/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2017-2018 Lubos Dolezel
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "binfmt.h"
#undef PAGE_MASK
#undef PAGE_SHIFT
#undef PAGE_SIZE
#include <mach-o/loader.h>
#include <mach-o/fat.h>
#undef __unused

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
#	include <linux/sched/task_stack.h>
#endif

#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <asm/mman.h>
#include <asm/elf.h>
#include <linux/ptrace.h>
#include <linux/version.h>
#include <linux/coredump.h>
#include <linux/highmem.h>
#include <linux/mount.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,9)
#include <linux/sched/task_stack.h>
#endif
#include "debug_print.h"
#include "task_registry.h"
#include "commpage.h"

// To get LINUX_SIGRTMIN
#include <rtsig.h>

struct load_results
{
	unsigned long mh;
	unsigned long entry_point;
	unsigned long stack_size;
	unsigned long dyld_all_image_location;
	unsigned long dyld_all_image_size;
	uint8_t uuid[16];

	unsigned long vm_addr_max;
	bool _32on64;
	int kernfd;
	unsigned long base;
	uint32_t bprefs[4];
	char* root_path;
};

extern struct file* xnu_task_setup(void);
extern int commpage_install(struct file* xnu_task);

static int macho_load(struct linux_binprm* bprm);
static int macho_coredump(struct coredump_params* cprm);
static int test_load(struct linux_binprm* bprm);
static int test_load_fat(struct linux_binprm* bprm);
static int load_fat(struct linux_binprm* bprm, struct file* file, uint32_t arch, struct load_results* lr);
static int load32(struct linux_binprm* bprm, struct file* file, struct fat_arch* farch, bool expect_dylinker, struct load_results* lr);
static int load64(struct linux_binprm* bprm, struct file* file, struct fat_arch* farch, bool expect_dylinker, struct load_results* lr);
static int load(struct linux_binprm* bprm, struct file* file, uint32_t arch, struct load_results* lr);
static int native_prot(int prot);
static int setup_stack64(struct linux_binprm* bprm, struct load_results* lr);
static int setup_stack32(struct linux_binprm* bprm, struct load_results* lr);
static int setup_space(struct linux_binprm* bprm, struct load_results* lr);
static void process_special_env(struct linux_binprm* bprm, struct load_results* lr);
static void vchroot_detect(struct load_results* lr);

// #define PAGE_ALIGN(x) ((x) & ~(PAGE_SIZE-1))
#define PAGE_ROUNDUP(x) (((((x)-1) / PAGE_SIZE)+1) * PAGE_SIZE)

struct linux_binfmt macho_format = {
	.module = THIS_MODULE,
	.load_binary = macho_load,
	.load_shlib = NULL,
#ifdef CONFIG_COREDUMP
	.core_dump = macho_coredump,
#endif
	.min_coredump = PAGE_SIZE
};

void macho_binfmt_init(void)
{
	register_binfmt(&macho_format);
}

void macho_binfmt_exit(void)
{
	unregister_binfmt(&macho_format);
}

int macho_load(struct linux_binprm* bprm)
{
	int err;
	struct load_results lr;
	struct pt_regs* regs = current_pt_regs();
	struct file* xnu_task;

	// Do quick checks on the executable
	err = test_load(bprm);
	if (err)
		goto out;

	// Setup a new XNU task
	xnu_task = xnu_task_setup();
	if (IS_ERR(xnu_task))
	{
		err = PTR_ERR(xnu_task);
		goto out;
	}

	// Block SIGNAL_SIGEXC_TOGGLE and SIGNAL_SIGEXC_THUPDATE.
	// See sigexc.c in libsystem_kernel.
	sigaddset(&current->blocked, LINUX_SIGRTMIN);
	sigaddset(&current->blocked, LINUX_SIGRTMIN+1);
	
	// Remove the running executable
	// This is the point of no return.
	err = flush_old_exec(bprm);
	if (err)
		goto out;

	memset(&lr, 0, sizeof(lr));
	err = load(bprm, bprm->file, 0, &lr);

	if (err)
	{
		fput(xnu_task);
		debug_msg("Binary failed to load: %d\n", err);
		goto out;
	}

	set_binfmt(&macho_format);

	current->mm->start_brk = current->mm->brk = PAGE_ALIGN(lr.vm_addr_max);
	current->mm->start_stack = bprm->p;

	// TODO: fill in start_code, end_code, start_data, end_data

	// Map commpage
	err = commpage_install(xnu_task);

	lr.kernfd = get_unused_fd_flags(O_RDWR | O_CLOEXEC);
	
	if (lr.kernfd >= 0)
		fd_install(lr.kernfd, xnu_task);
	else
		err = lr.kernfd;

	// The ref to the task is now held by the commpage mapping
	// fput(xnu_task);

	if (err != 0)
	{
		debug_msg("Failed to install commpage: %d\n", err);
		send_sig(SIGKILL, current, 0);
		return err;
	}

	// Setup the stack
	if (lr._32on64)
		setup_stack32(bprm, &lr);
	else
		setup_stack64(bprm, &lr);

	// Set DYLD_INFO
	darling_task_set_dyld_info(lr.dyld_all_image_location, lr.dyld_all_image_size);

	// debug_msg("Entry point: %lx, stack: %lx, mh: %lx\n", (void*) lr.entry_point, (void*) bprm->p, (void*) lr.mh);

	//unsigned int* pp = (unsigned int*)bprm->p;
	//int i;
	//for (i = 0; i < 30; i++)
	//{
	//	debug_msg("sp @%p: 0x%x\n", pp, *pp);
	//	pp++;
	//}
	
	start_thread(regs, lr.entry_point, bprm->p);
out:
	if (lr.root_path)
		kfree(lr.root_path);

	return err;
}

int setup_space(struct linux_binprm* bprm, struct load_results* lr)
{
	int err;

	// Explanation:
	// Using STACK_TOP would cause the stack to be placed just above the commpage
	// and would collide with it eventually.
	unsigned long stackAddr = commpage_address(!test_thread_flag(TIF_IA32));

	setup_new_exec(bprm);
	install_exec_creds(bprm);

	// TODO: Mach-O supports executable stacks

	err = setup_arg_pages(bprm, stackAddr, EXSTACK_DISABLE_X);
	if (err != 0)
		return err;

	// If vchroot is active in the current process, respect it
	vchroot_detect(lr);
	// Parse binprefs out of env, evaluate DYLD_ROOT_PATH
	process_special_env(bprm, lr);

	return 0;
}

static const char EXECUTABLE_PATH[] = "executable_path=";

int load(struct linux_binprm* bprm,
		struct file* file,
		uint32_t arch,
		struct load_results* lr)
{
	uint32_t magic = *(uint32_t*)bprm->buf;

	if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64)
	{
		// Make sure the loader has the right cputype
		if (arch && ((struct mach_header*) bprm->buf)->cputype != arch)
			return -ENOEXEC;

		return load64(bprm, file, NULL, false, lr);
	}
	else if (magic == MH_MAGIC || magic == MH_CIGAM)
	{
		// Make sure the loader has the right cputype
		if (arch && ((struct mach_header*) bprm->buf)->cputype != arch)
			return -ENOEXEC;

		// TODO: make process 32-bit
		return load32(bprm, file, NULL, false, lr);
	}
	else if (magic == FAT_MAGIC || magic == FAT_CIGAM)
	{
		return load_fat(bprm, file, arch, lr);
	}
	else
		return -ENOEXEC;
}

int test_load(struct linux_binprm* bprm)
{
	uint32_t magic = *(uint32_t*)bprm->buf;

	// TODO: This function should check if the dynamic loader is present and valid

	if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64)
	{
		struct mach_header_64* mh = (struct mach_header_64*) bprm->buf;

		uint32_t filetype = mh->filetype;
		// if (magic == MH_CIGAM_64)
		//	be32_to_cpus(&filetype);

		if (filetype != MH_EXECUTE)
			return -ENOEXEC;

#ifdef __x86_64__
		uint32_t cputype = mh->cputype;
		// if (magic == MH_CIGAM_64)
		// 	be32_to_cpus(&cputype);

		if ((cputype & ~CPU_ARCH_MASK) != CPU_TYPE_X86)
			return -ENOEXEC;
#endif
		return 0;
	}
	else if (magic == MH_MAGIC || magic == MH_CIGAM)
	{
		struct mach_header_64* mh = (struct mach_header_64*) bprm->buf;

		if (mh->filetype != MH_EXECUTE)
			return -ENOEXEC;

#ifdef __x86_64__
		if ((mh->cputype & ~CPU_ARCH_MASK) != CPU_TYPE_X86)
			return -ENOEXEC;
#endif

		return 0;
	}
	else if (magic == FAT_MAGIC || magic == FAT_CIGAM)
	{
		return test_load_fat(bprm);
	}
	else
		return -ENOEXEC;
}

int test_load_fat(struct linux_binprm* bprm)
{
	struct fat_header* fhdr = (struct fat_header*) bprm->buf;
	const bool swap = fhdr->magic == FAT_CIGAM;
	u32 narch = fhdr->nfat_arch;
	bool found_usable = false;

	if (swap)
		be32_to_cpus(&narch);

	if (sizeof(*fhdr) + narch * sizeof(struct fat_arch) > sizeof(bprm->buf))
		return -ENOEXEC;

	uint32_t i;
	for (i = 0; i < narch; i++)
	{
		struct fat_arch* arch;
		u32 cputype;

		arch = ((struct fat_arch*)(fhdr+1)) + i;

		cputype = arch->cputype;
		if (swap)
			be32_to_cpus(&cputype);

#ifdef __x86_64__
		if ((cputype & ~CPU_ARCH_MASK) == CPU_TYPE_X86)
		{
			found_usable = true;
			break;
		}
#endif
	}

	if (!found_usable)
		return -ENOEXEC;

	return 0;
}

int load_fat(struct linux_binprm* bprm,
		struct file* file,
		uint32_t forced_arch,
		struct load_results* lr)
{
	struct fat_header* fhdr = (struct fat_header*) bprm->buf;
	const bool swap = fhdr->magic == FAT_CIGAM;
	struct fat_arch* best_arch = NULL;
	int bpref_index = -1;

	// Here we assume that our current endianess is LE
	// which is actually true for all of Darling's supported archs.
#define SWAP32(x) be32_to_cpus((u32*) &(x))

	if (swap)
		SWAP32(fhdr->nfat_arch);

	if (sizeof(*fhdr) + fhdr->nfat_arch * sizeof(struct fat_arch) > sizeof(bprm->buf))
		return -ENOEXEC;

	uint32_t i;
	for (i = 0; i < fhdr->nfat_arch; i++)
	{
		struct fat_arch* arch;

		arch = ((struct fat_arch*)(fhdr+1)) + i;

		if (swap)
		{
			SWAP32(arch->cputype);
			SWAP32(arch->cpusubtype);
			SWAP32(arch->offset);
			SWAP32(arch->size);
			SWAP32(arch->align);
		}

		if (!forced_arch)
		{
			int j;
			for (j = 0; j < 4; j++)
			{
				if (lr->bprefs[j] && arch->cputype == lr->bprefs[j])
				{
					if (bpref_index == -1 || bpref_index > j)
					{
						best_arch = arch;
						bpref_index = j;
						break;
					}
				}
			}

			if (bpref_index == -1)
			{
#if defined(__x86_64__)
				if (arch->cputype == CPU_TYPE_X86_64)
					best_arch = arch;
				else if (best_arch == NULL && arch->cputype == CPU_TYPE_X86)
					best_arch = arch;
#elif defined (__aarch64__)
#warning TODO: arm
#else
#error Unsupported CPU architecture
#endif
			}
		}
		else
		{
			if (arch->cputype == forced_arch)
				best_arch = arch;
		}
	}

	if (best_arch == NULL)
		return -ENOEXEC;

	if (best_arch->cputype & CPU_ARCH_ABI64)
		return load64(bprm, file, best_arch, forced_arch != 0, lr);
	else
		return load32(bprm, file, best_arch, forced_arch != 0, lr);
}

#define GEN_64BIT
#include "binfmt_loader.c"
#include "binfmt_stack.c"
#undef GEN_64BIT

#define GEN_32BIT
#include "binfmt_loader.c"
#include "binfmt_stack.c"
#undef GEN_32BIT

int native_prot(int prot)
{
	int protOut = 0;

	if (prot & VM_PROT_READ)
		protOut |= PROT_READ;
	if (prot & VM_PROT_WRITE)
		protOut |= PROT_WRITE;
	if (prot & VM_PROT_EXECUTE)
		protOut |= PROT_EXEC;

	return protOut;
}

extern struct vfsmount* task_get_vchroot(task_t t);
void vchroot_detect(struct load_results* lr)
{
	struct vfsmount* vchroot;
	struct path path;

	// Find out if the current process has an associated XNU task_t
	task_t task = darling_task_get_current();

	if (!task)
		return;

	vchroot = task_get_vchroot(task);
	if (!vchroot)
		return;

	// Get the path as a string into load_results
	char* buf = (char*) __get_free_page(GFP_KERNEL);
	char* p = dentry_path_raw(vchroot->mnt_root, buf, 1023);

	if (IS_ERR(p))
		goto out;

	lr->root_path = kmalloc(strlen(p) + 1, GFP_KERNEL);
	strcpy(lr->root_path, p);
out:
	free_page((unsigned long) buf);
	mntput(vchroot);
}

// Given that there's no proper way of passing special parameters to the binary loader
// via execve(), we must do this via env variables
void process_special_env(struct linux_binprm* bprm, struct load_results* lr)
{
	unsigned long p = current->mm->arg_start;

	// Get past argv strings
	int argc = bprm->argc;
	while (argc--)
	{
		size_t len = strnlen_user((void __user*) p, MAX_ARG_STRLEN);
		if (!len || len > MAX_ARG_STRLEN)
			return;

		p += len;
	}

	int envc = bprm->envc;
	char* env_value = (char*) kmalloc(MAX_ARG_STRLEN, GFP_KERNEL);

	while (envc--)
	{
		size_t len = strnlen_user((void __user*) p, MAX_ARG_STRLEN);
		if (!len || len > MAX_ARG_STRLEN)
			break;

		if (copy_from_user(env_value, (void __user*) p, len) == 0)
		{
			printk(KERN_NOTICE "env var: %s\n", env_value);

			if (strncmp(env_value, "__mldr_bprefs=", 14) == 0)
			{
				// NOTE: This env var will not be passed to userland, see setup_stack32/64
				sscanf(env_value+14, "%x,%x,%x,%x", &lr->bprefs[0], &lr->bprefs[1], &lr->bprefs[2], &lr->bprefs[3]);
			}
			else if (strncmp(env_value, "DYLD_ROOT_PATH=", 15) == 0)
			{
				if (lr->root_path == NULL)
				{
					// len already includes NUL
					lr->root_path = (char*) kmalloc(len - 15, GFP_KERNEL);
					if (lr->root_path)
						strcpy(lr->root_path, env_value + 15);
				}
			}
		}
		else
			printk(KERN_NOTICE "Cannot get env var value\n");
		p += len;
	}

	kfree(env_value);
}

// Copied from arch/x86/kernel/process_64.c
// Why on earth isn't this exported?!
#ifdef __x86_64__
static void
start_thread_common(struct pt_regs *regs, unsigned long new_ip,
		    unsigned long new_sp,
		    unsigned int _cs, unsigned int _ss, unsigned int _ds)
{
	loadsegment(fs, 0);
	loadsegment(es, _ds);
	loadsegment(ds, _ds);
	load_gs_index(0);
	regs->ip		= new_ip;
	regs->sp		= new_sp;
	regs->cs		= _cs;
	regs->ss		= _ss;
	regs->flags		= X86_EFLAGS_IF;
	force_iret();
}

void
start_thread(struct pt_regs *regs, unsigned long new_ip, unsigned long new_sp)
{
	bool ia32 = test_thread_flag(TIF_IA32);
	start_thread_common(regs, new_ip, new_sp,
			ia32 ? __USER32_CS : __USER_CS,
			__USER_DS,
			ia32 ? __USER_DS : 0);
}
#endif


//////////////////////////////////////////////////////////////////////////////
// CORE DUMPING SUPPORT                                                     //
//////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_COREDUMP

// Copied and adapted from mm/gup.c from get_dump_page() (not exported for LKMs)
static
struct page *macho_get_dump_page(unsigned long addr)
{
	struct vm_area_struct *vma;
	struct page *page;

	if (get_user_pages(addr, 1, FOLL_FORCE | FOLL_DUMP | FOLL_GET, &page, &vma) < 1)
		return NULL;
	flush_cache_page(vma, addr, page_to_pfn(page));
	return page;
}

struct thread_flavor
{
	// preceded by struct thread_command and other flavors
	uint32_t flavor;
	uint32_t count;
	char state[0];
	// followed by x86_thread_state32_t, for example
};

static
void fill_thread_state32(x86_thread_state32_t* state, struct task_struct* task)
{
	const struct pt_regs* regs = task_pt_regs(task);

	state->eax = regs->ax;
	state->ebx = regs->bx;
	state->ecx = regs->cx;
	state->edx = regs->dx;
	state->esi = regs->si;
	state->edi = regs->di;
	state->eip = regs->ip;
	state->ebp = regs->bp;
	state->esp = regs->sp;
	state->ss = regs->ss;
	state->eflags = regs->flags;
	state->cs = regs->cs;
}

static
void fill_float_state32(x86_float_state32_t* state, struct task_struct* task)
{
	// TODO
	memset(state, 0, sizeof(*state));
}

static
void fill_thread_state64(x86_thread_state64_t* state, struct task_struct* task)
{
	const struct pt_regs* regs = task_pt_regs(task);

	state->rax = regs->ax;
	state->rbx = regs->bx;
	state->rcx = regs->cx;
	state->rdx = regs->dx;
	state->rdi = regs->di;
	state->rsi = regs->si;
	state->rbp = regs->bp;
	state->rsp = regs->sp;
	state->r8 = regs->r8;
	state->r9 = regs->r9;
	state->r10 = regs->r10;
	state->r11 = regs->r11;
	state->r12 = regs->r12;
	state->r13 = regs->r13;
	state->r14 = regs->r14;
	state->r15 = regs->r15;
	state->rflags = regs->flags;
	state->cs = regs->cs;
	state->rip = regs->ip;
}

static
void fill_float_state64(x86_float_state64_t* state, struct task_struct* task)
{
	// TODO
	memset(state, 0, sizeof(*state));
}

static
bool macho_dump_headers32(struct coredump_params* cprm)
{
	// Count memory segments and threads
	unsigned int segs = current->mm->map_count;
	unsigned int threads = 0; // = atomic_read(&current->mm->core_state->nr_threads); // doesn't seem to work?
	struct core_thread* ct;

	for (ct = &current->mm->core_state->dumper; ct != NULL; ct = ct->next)
		threads++;

	struct mach_header mh;

	mh.magic = MH_MAGIC;
#ifdef __x86_64__
	mh.cputype = CPU_TYPE_X86;
	mh.cpusubtype = CPU_SUBTYPE_X86_ALL;
#else
#warning Missing code for this arch
#endif
	mh.filetype = MH_CORE;
	mh.ncmds = segs + threads;

	const int statesize = sizeof(x86_thread_state32_t) + sizeof(x86_float_state32_t) + sizeof(struct thread_flavor)*2;

	mh.sizeofcmds = segs * sizeof(struct segment_command) + threads * (sizeof(struct thread_command) + statesize);

	if (!dump_emit(cprm, &mh, sizeof(mh)))
		goto fail;

	struct vm_area_struct* vma;
	uint32_t file_offset = mh.sizeofcmds + sizeof(mh);

	for (vma = current->mm->mmap; vma != NULL; vma = vma->vm_next)
	{
		struct segment_command sc;

		sc.cmd = LC_SEGMENT;
		sc.cmdsize = sizeof(sc);
		sc.segname[0] = 0;
		sc.nsects = 0;
		sc.flags = 0;
		sc.vmaddr = vma->vm_start;
		sc.vmsize = vma->vm_end - vma->vm_start;
		sc.fileoff = file_offset;
		
		if (sc.vmaddr > 0) // avoid dumping the __PAGEZERO segment which may be really large
			sc.filesize = sc.vmsize;
		else
			sc.filesize = 0;
		sc.initprot = 0;

		if (vma->vm_flags & VM_READ)
			sc.initprot |= VM_PROT_READ;
		if (vma->vm_flags & VM_WRITE)
			sc.initprot |= VM_PROT_WRITE;
		if (vma->vm_flags & VM_EXEC)
			sc.initprot |= VM_PROT_EXECUTE;
		sc.maxprot = sc.initprot;

		if (!dump_emit(cprm, &sc, sizeof(sc)))
			goto fail;

		file_offset += sc.filesize;
	}

	const int memsize = sizeof(struct thread_command) + statesize;
	uint8_t* buffer = kmalloc(memsize, GFP_KERNEL);

	for (ct = &current->mm->core_state->dumper; ct != NULL; ct = ct->next)
	{
		struct thread_command* tc = (struct thread_command*) buffer;
		struct thread_flavor* tf = (struct thread_flavor*)(tc+1);

		tc->cmd = LC_THREAD;
		tc->cmdsize = memsize;

		// General registers
		tf->flavor = x86_THREAD_STATE32;
		tf->count = x86_THREAD_STATE32_COUNT;

		fill_thread_state32((x86_thread_state32_t*) tf->state, ct->task);

		// Float registers
		tf = (struct thread_flavor*) (((char*) tf) + sizeof(x86_thread_state32_t));
		tf->flavor = x86_FLOAT_STATE32;
		tf->count = x86_FLOAT_STATE32_COUNT;

		fill_float_state32((x86_float_state32_t*) tf->state, ct->task);

		if (!dump_emit(cprm, buffer, memsize))
		{
			kfree(buffer);
			goto fail;
		}
	}
	kfree(buffer);

	return true;
fail:
	return false;
}

static
bool macho_dump_headers64(struct coredump_params* cprm)
{
	// Count memory segments and threads
	unsigned int segs = current->mm->map_count;
	unsigned int threads = 0; // = atomic_read(&current->mm->core_state->nr_threads); // doesn't seem to work?
	struct core_thread* ct;
	struct mach_header_64 mh;

	for (ct = &current->mm->core_state->dumper; ct != NULL; ct = ct->next)
		threads++;

	mh.magic = MH_MAGIC_64;
#ifdef __x86_64__
	mh.cputype = CPU_TYPE_X86_64;
	mh.cpusubtype = CPU_SUBTYPE_X86_64_ALL;
#else
#warning Missing code for this arch
#endif
	mh.filetype = MH_CORE;
	mh.ncmds = segs + threads;

	debug_msg("CORE: threads: %d\n", threads);

	const int statesize = sizeof(x86_thread_state32_t) + sizeof(x86_float_state64_t) + sizeof(struct thread_flavor)*2;
	mh.sizeofcmds = segs * sizeof(struct segment_command_64) + threads * (sizeof(struct thread_command) + statesize);
	mh.reserved = 0;

	if (!dump_emit(cprm, &mh, sizeof(mh)))
		goto fail;

	struct vm_area_struct* vma;
	uint32_t file_offset = mh.sizeofcmds + sizeof(mh);

	for (vma = current->mm->mmap; vma != NULL; vma = vma->vm_next)
	{
		struct segment_command_64 sc;

		sc.cmd = LC_SEGMENT_64;
		sc.cmdsize = sizeof(sc);
		sc.segname[0] = 0;
		sc.nsects = 0;
		sc.flags = 0;
		sc.vmaddr = vma->vm_start;
		sc.vmsize = vma->vm_end - vma->vm_start;
		sc.fileoff = file_offset;
		
		if (sc.vmaddr > 0) // avoid dumping the __PAGEZERO segment which may be really large
			sc.filesize = sc.vmsize;
		else
			sc.filesize = 0;
		sc.initprot = 0;

		if (vma->vm_flags & VM_READ)
			sc.initprot |= VM_PROT_READ;
		if (vma->vm_flags & VM_WRITE)
			sc.initprot |= VM_PROT_WRITE;
		if (vma->vm_flags & VM_EXEC)
			sc.initprot |= VM_PROT_EXECUTE;
		sc.maxprot = sc.initprot;

		if (!dump_emit(cprm, &sc, sizeof(sc)))
			goto fail;

		file_offset += sc.filesize;
	}

	const int memsize = sizeof(struct thread_command) + statesize;
	uint8_t* buffer = kmalloc(memsize, GFP_KERNEL);

	for (ct = &current->mm->core_state->dumper; ct != NULL; ct = ct->next)
	{

		struct thread_command* tc = (struct thread_command*) buffer;
		struct thread_flavor* tf = (struct thread_flavor*)(tc+1);

		tc->cmd = LC_THREAD;
		tc->cmdsize = memsize;

		// General registers
		tf->flavor = x86_THREAD_STATE64;
		tf->count = x86_THREAD_STATE64_COUNT;

		fill_thread_state64((x86_thread_state64_t*) tf->state, ct->task);

		// Float registers
		tf = (struct thread_flavor*) (tf->state + sizeof(x86_thread_state64_t));
		tf->flavor = x86_FLOAT_STATE64;
		tf->count = x86_FLOAT_STATE64_COUNT;

		fill_float_state64((x86_float_state64_t*) tf->state, ct->task);

		if (!dump_emit(cprm, buffer, memsize))
		{
			kfree(buffer);
			goto fail;
		}
	}
	kfree(buffer);

	return true;
fail:
	return false;
}

int macho_coredump(struct coredump_params* cprm)
{
	mm_segment_t fs = get_fs();
	set_fs(KERNEL_DS);

	// Write the Mach-O header and loader commands
	if (test_thread_flag(TIF_IA32))
	{
		// 32-bit executables
		if (!macho_dump_headers32(cprm))
			goto fail;
	}
	else
	{
		// 64-bit executables
		if (!macho_dump_headers64(cprm))
			goto fail;
	}

	// Dump memory contents
	struct vm_area_struct* vma;

	// Inspired by elf_core_dump()
	for (vma = current->mm->mmap; vma != NULL; vma = vma->vm_next)
	{
		unsigned long addr;

		if (vma->vm_start == 0)
			continue; // skip __PAGEZERO dumping

		for (addr = vma->vm_start; addr < vma->vm_end; addr += PAGE_SIZE)
		{
			struct page* page;
			bool stop;

			page = macho_get_dump_page(addr);

			if (page)
			{
				void* kaddr = kmap(page);
				stop = !dump_emit(cprm, kaddr, PAGE_SIZE);
				kunmap(page);
				put_page(page);
			}
			else
				stop = !dump_skip(cprm, PAGE_SIZE);

			if (stop)
				goto fail;
		}
	}

	dump_truncate(cprm);

fail:
	set_fs(fs);
	return cprm->written > 0;
}

#else
#warning Core dumping not allowed by kernel config
#endif

