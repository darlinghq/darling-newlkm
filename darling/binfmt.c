/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2017 Lubos Dolezel
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
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <asm/mman.h>
#include <asm/elf.h>
#include <linux/ptrace.h>
#include "debug_print.h"
#include "task_registry.h"

struct load_results
{
	unsigned long mh;
	unsigned long entry_point;
	unsigned long stack_size;
	unsigned long dyld_all_image_location;
	unsigned long dyld_all_image_size;
	uint8_t uuid[16];

	unsigned long vm_addr_max;
};

extern struct file* commpage_install(void);

static int macho_load(struct linux_binprm* bprm);
static int test_load(struct linux_binprm* bprm);
static int test_load_fat(struct linux_binprm* bprm);
static int load_fat(struct linux_binprm* bprm, struct file* file, uint32_t bprefs[4], uint32_t arch, struct load_results* lr);
static int load32(struct linux_binprm* bprm, struct file* file, struct fat_arch* farch, bool expect_dylinker, struct load_results* lr);
static int load64(struct linux_binprm* bprm, struct file* file, struct fat_arch* farch, bool expect_dylinker, struct load_results* lr);
static int load(struct linux_binprm* bprm, struct file* file, uint32_t *bprefs, uint32_t arch, struct load_results* lr);
static int native_prot(int prot);
static int setup_stack(struct linux_binprm* bprm, struct load_results* lr);

// #define PAGE_ALIGN(x) ((x) & ~(PAGE_SIZE-1))
#define PAGE_ROUNDUP(x) (((((x)-1) / PAGE_SIZE)+1) * PAGE_SIZE)

struct linux_binfmt macho_format = {
	.module = THIS_MODULE,
	.load_binary = macho_load,
	.load_shlib = NULL,
	.core_dump = NULL, // TODO: We will want this eventually
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
	uint32_t bprefs[4] = { 0, 0, 0, 0 };
	int err;
	struct load_results lr;
	struct pt_regs* regs = current_pt_regs();

	// TODO: parse binprefs out of env
	
	// Do quick checks on the executable
	err = test_load(bprm);
	if (err)
		goto out;
	
	// Remove the running executable
	err = flush_old_exec(bprm);
	if (err)
		goto out;

	setup_new_exec(bprm);
	install_exec_creds(bprm);

	err = setup_arg_pages(bprm, STACK_TOP, false);
	if (err)
		goto out;

	current->mm->start_stack = bprm->p;
	
	memset(&lr, 0, sizeof(lr));
	err = load(bprm, bprm->file, bprefs, 0, &lr);

	if (err)
	{
		debug_msg("Binary failed to load: %d\n", err);
		goto out;
	}

	set_binfmt(&macho_format);

	current->mm->start_brk = lr.vm_addr_max;
	current->mm->brk = lr.vm_addr_max;
	current->mm->start_stack = bprm->p;

	// TODO: fill in start_code, end_code, start_data, end_data

	// Setup the stack
	setup_stack(bprm, &lr);

	// Map commpage
	struct file* file = commpage_install();
	if (IS_ERR(file))
	{
		err = PTR_ERR(file);
		send_sig(SIGKILL, current, 0);
		return err;
	}
	
	fput(file);

	// Set DYLD_INFO
	darling_task_set_dyld_info(lr.dyld_all_image_location, lr.dyld_all_image_size);

	debug_msg("Entry point: %p\n", (void*) lr.entry_point);
	start_thread(regs, lr.entry_point, bprm->p);
out:
	return err;
}

static const char EXECUTABLE_PATH[] = "executable_path=";
int setup_stack(struct linux_binprm* bprm, struct load_results* lr)
{
	int err = 0;
	// unsigned char rand_bytes[16];
	char *executable_path, *executable_buf;
	unsigned long __user* argv;
	unsigned long __user* envp;
	unsigned long __user* applep;
	unsigned long __user* sp;
	char __user* exepath_user;
	size_t exepath_len;

	// Produce executable_path=... for applep
	executable_buf = kmalloc(4096, GFP_KERNEL);
	if (!executable_buf)
		return -ENOMEM;

	executable_path = d_path(&bprm->file->f_path, executable_buf, 4095);
	if (IS_ERR(executable_path))
	{
		err = -ENAMETOOLONG;
		goto out;
	}

	exepath_len = strlen(executable_path);
	sp = (unsigned long*) bprm->p;
	sp -= bprm->argc + bprm->envc + 40 + exepath_len;
	exepath_user = (char __user*) bprm->p - exepath_len - sizeof(EXECUTABLE_PATH);

	if (!find_extend_vma(current->mm, (unsigned long) sp))
	{
		err = -EFAULT;
		goto out;
	}

	if (copy_to_user(exepath_user, EXECUTABLE_PATH, sizeof(EXECUTABLE_PATH)-1))
	{
		err = -EFAULT;
		goto out;
	}
	if (copy_to_user(exepath_user + sizeof(EXECUTABLE_PATH)-1, executable_path, exepath_len + 1))
	{
		err = -EFAULT;
		goto out;
	}

	kfree(executable_buf);
	executable_buf = NULL;
	bprm->p = (unsigned long) sp;

	// XXX: skip this for static executables, but we don't support them anyway...
	if (__put_user(lr->mh, sp++))
	{
		err = -EFAULT;
		goto out;
	}
	if (__put_user(bprm->argc, sp++))
	{
		err = -EFAULT;
		goto out;
	}

	unsigned long p = current->mm->arg_start;
	int argc = bprm->argc;

	argv = sp;
	envp = argv + argc + 1;
	applep = envp + bprm->envc + 1;

	// Fill in argv pointers
	while (argc--)
	{
		if (__put_user(p, argv++))
		{
			err = -EFAULT;
			goto out;
		}

		size_t len = strnlen_user((void __user*) p, MAX_ARG_STRLEN);
		if (!len || len > MAX_ARG_STRLEN)
		{
			err = -EINVAL;
			goto out;
		}

		p += len;
	}
	if (__put_user(0, argv++))
	{
		err = -EFAULT;
		goto out;
	}
	current->mm->arg_end = current->mm->env_start = p;

	// Fill in envp pointers
	int envc = bprm->envc;
	while (envc--)
	{
		if (__put_user(p, envp++))
		{
			err = -EFAULT;
			goto out;
		}

		size_t len = strnlen_user((void __user*) p, MAX_ARG_STRLEN);
		if (!len || len > MAX_ARG_STRLEN)
		{
			err = -EINVAL;
			goto out;
		}

		p += len;
	}
	if (__put_user(0, envp++))
	{
		err = -EFAULT;
		goto out;
	}
	current->mm->env_end = p;

	if (__put_user(exepath_user, applep++))
	{
		err = -EFAULT;
		goto out;
	}
	if (__put_user(0, applep++))
	{
		err = -EFAULT;
		goto out;
	}

	// get_random_bytes(rand_bytes, sizeof(rand_bytes));

	// TODO: produce stack_guard, e.g. stack_guard=0xcdd5c48c061b00fd (must contain 00 somewhere!)
	// TODO: produce malloc_entropy, e.g. malloc_entropy=0x9536cc569d9595cf,0x831942e402da316b
	// TODO: produce main_stack?
	
out:
	if (executable_buf)
		kfree(executable_buf);
	return err;
}

int load(struct linux_binprm* bprm,
		struct file* file,
		uint32_t *bprefs,
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
		return load_fat(bprm, file, bprefs, arch, lr);
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
		uint32_t bprefs[4],
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
				if (bprefs[j] && arch->cputype == bprefs[j])
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
#include "loader.c"
#undef GEN_64BIT

#define GEN_32BIT
#include "loader.c"
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
	start_thread_common(regs, new_ip, new_sp,
			    __USER_CS, __USER_DS, 0);
}
#endif

