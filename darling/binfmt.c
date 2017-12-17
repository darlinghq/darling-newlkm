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
#include <asm/mman.h>

struct load_results
{
	unsigned long mh;
	unsigned long entry_point;
	unsigned long stack_size;
	unsigned long dyld_all_image_location;
	unsigned long dyld_all_image_size;
	uint8_t uuid[16];
};

static int macho_load(struct linux_binprm* bprm);
static int load_fat(struct linux_binprm* bprm, struct file* file, uint32_t bprefs[4], uint32_t arch, struct load_results* lr);
static int load32(struct linux_binprm* bprm, struct file* file, struct fat_arch* farch, bool expect_dylinker, struct load_results* lr);
static int load64(struct linux_binprm* bprm, struct file* file, struct fat_arch* farch, bool expect_dylinker, struct load_results* lr);
static int load(struct linux_binprm* bprm, struct file* file, uint32_t *bprefs, uint32_t arch, struct load_results* lr);
static int native_prot(int prot);

#define PAGE_ALIGN(x) ((x) & ~(PAGE_SIZE-1))
#define PAGE_ROUNDUP(x) (((((x)-1) / PAGE_SIZE)+1) * PAGE_SIZE)

static struct linux_binfmt macho_format = {
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

	// TODO: parse binprefs out of env
	
	memset(&lr, 0, sizeof(lr));
	err = load(bprm, bprm->file, bprefs, 0, &lr);

	if (err)
		goto out;

	// TODO: Map commpage
	// TODO: Set DYLD_INFO
	// TODO: Map pagezero
	// TODO: prng seed in apple[]

out:
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

