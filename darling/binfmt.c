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
#include <mach-o/loader.h>
#include <mach-o/fat.h>

static int macho_load(struct linux_binprm* bprm);
static int load_fat(struct linux_binprm* bprm, uint32_t bprefs[4]);
static int load32(struct linux_binprm* bprm, struct fat_arch* farch);
static int load64(struct linux_binprm* bprm, struct fat_arch* farch);

static struct linux_binfmt macho_format = {
	.module = THIS_MODULE,
	.load_binary = macho_load,
	.load_shlib = NULL,
	.core_dump = NULL,
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
	uint32_t magic;
	uint32_t bprefs[4] = { 0, 0, 0, 0 };

	// TODO: parse binprefs out of env
	
	magic = *(uint32_t*) bprm->buf;

	if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64)
	{
		return load64(bprm, NULL);
	}
	else if (magic == MH_MAGIC || magic == MH_CIGAM)
	{
		// TODO: make process 32-bit
		return load32(bprm, NULL);
	}
	else if (magic == FAT_MAGIC || magic == FAT_CIGAM)
	{
		return load_fat(bprm, bprefs);
	}
	else
		return -ENOEXEC;
}

int load_fat(struct linux_binprm* bprm, uint32_t bprefs[4])
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

	if (best_arch == NULL)
		return -ENOEXEC;

	if (best_arch->cputype & CPU_ARCH_ABI64)
		return load64(bprm, best_arch);
	else
		return load32(bprm, best_arch);
}

#define GEN_64BIT
#include "loader.c"
#undef GEN_64BIT

#define GEN_32BIT
#include "loader.c"
#undef GEN_32BIT

