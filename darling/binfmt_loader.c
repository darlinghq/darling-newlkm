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


// Definitions:
// FUNCTION_NAME (load32/load64)
// SEGMENT_STRUCT (segment_command/SEGMENT_STRUCT)
// SEGMENT_COMMAND (LC_SEGMENT/SEGMENT_COMMAND)
// MACH_HEADER_STRUCT (mach_header/MACH_HEADER_STRUCT)
// SECTION_STRUCT (section/SECTION_STRUCT)

#if defined(GEN_64BIT)
#   define FUNCTION_NAME load64
#   define SEGMENT_STRUCT segment_command_64
#   define SEGMENT_COMMAND LC_SEGMENT_64
#   define MACH_HEADER_STRUCT mach_header_64
#   define SECTION_STRUCT section_64
#	define MAP_EXTRA 0
#elif defined(GEN_32BIT)
#   define FUNCTION_NAME load32
#   define SEGMENT_STRUCT segment_command
#   define SEGMENT_COMMAND LC_SEGMENT
#   define MACH_HEADER_STRUCT mach_header
#   define SECTION_STRUCT section
#	define MAP_EXTRA MAP_32BIT
#else
#   error See above
#endif

#ifndef BAD_ADDR
#	define BAD_ADDR(x) ((unsigned long)(x) >= TASK_SIZE)
#endif

int FUNCTION_NAME(struct linux_binprm* bprm,
		struct file* file,
		struct fat_arch* farch,
		bool expect_dylinker,
		struct load_results* lr)
{
	struct MACH_HEADER_STRUCT header;
	uint8_t* cmds;
	uintptr_t entryPoint = 0;
	struct MACH_HEADER_STRUCT* mappedHeader = NULL;
	uintptr_t slide = 0;
	bool pie = false;
	uint32_t fat_offset;
	int err = 0;
	uint32_t i, p;
	loff_t pos;

	struct cred *override_cred = NULL;
	const struct cred *old_cred;

	if (!expect_dylinker)
	{
#ifdef GEN_32BIT
		lr->_32on64 = true;
		set_personality_ia32(false);
#endif
		setup_space(bprm, lr);
	}

	fat_offset = pos = farch ? farch->offset : 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	if (kernel_read(file, (char*) &header, sizeof(header), &pos) != sizeof(header))
#else
	if (kernel_read(file, fat_offset, (char*) &header, sizeof(header)) != sizeof(header))
#endif
		return -EIO;

	if (header.filetype != (expect_dylinker ? MH_DYLINKER : MH_EXECUTE))
		return -ENOEXEC;

	cmds = (uint8_t*) kmalloc(header.sizeofcmds, GFP_KERNEL);
	if (!cmds)
		return -ENOMEM;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
	if (kernel_read(file, cmds, header.sizeofcmds, &pos) != header.sizeofcmds)
#else
	if (kernel_read(file, fat_offset + sizeof(header), cmds, header.sizeofcmds) != header.sizeofcmds)
#endif
	{
		debug_msg("Binary failed to load %d bytes\n", header.sizeofcmds);
		kfree(cmds);
		return -EIO;
	}

	if ((header.filetype == MH_EXECUTE && (header.flags & MH_PIE)) || header.filetype == MH_DYLINKER)
	{
		uintptr_t base = -1;
		uintptr_t mmapSize = 0;

		// Go through all SEGMENT_COMMAND commands to get the total continuous range required.
		for (i = 0, p = 0; i < header.ncmds; i++)
		{
			struct SEGMENT_STRUCT* seg = (struct SEGMENT_STRUCT*) &cmds[p];

			// Load commands are always sorted, so this will get us the maximum address.
			if (seg->cmd == SEGMENT_COMMAND && strcmp(seg->segname, "__PAGEZERO") != 0)
			{
				if (base == -1)
				{
					base = seg->vmaddr;
					//if (base != 0 && header.filetype == MH_DYLINKER)
					//	goto no_slide;
				}
				mmapSize = seg->vmaddr + seg->vmsize - base;
			}

			p += seg->cmdsize;
		}

		// printk(KERN_NOTICE "mmapSize=0x%llx, base=0x%llx\n", mmapSize, lr->base);
		slide = vm_mmap(NULL, lr->base, mmapSize, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_EXTRA, 0);
		// printk(KERN_NOTICE "mmap result=0x%lx (suggested 0x%lx)\n", slide, lr->base);
		if (BAD_ADDR(slide))
		{
			err = -ENOMEM;
			goto out;
		}
		if (slide + mmapSize > lr->vm_addr_max)
			lr->vm_addr_max = lr->base = slide + mmapSize;
		slide -= base;

		pie = true;
	}
no_slide:

	// We need to have CAP_SYS_RAWIO to be able to map __PAGEZERO
	// and to be able to run pretty much any 32-bit binary on systems
	// that set mmap_min_addr as high as 64K.
	override_cred = prepare_creds();
	cap_raise(override_cred->cap_effective, CAP_SYS_RAWIO);
	old_cred = override_creds(override_cred);

	for (i = 0, p = 0; i < header.ncmds && p < header.sizeofcmds; i++)
	{
		struct load_command* lc;

		lc = (struct load_command*) &cmds[p];
		if (lc->cmdsize > PAGE_SIZE)
		{
			debug_msg("Broken Mach-O file, cmdsize = %d\n", lc->cmdsize);
			err = -ENOEXEC;
			goto out;
		}

		switch (lc->cmd)
		{
			case SEGMENT_COMMAND:
			{
				struct SEGMENT_STRUCT* seg = (struct SEGMENT_STRUCT*) lc;
				void* rv;

				// This logic is wrong and made up. But it's the only combination where
				// some apps stop crashing (TBD why) and LLDB recognized the memory layout
				// of processes started as suspended.
				int maxprot = native_prot(seg->maxprot);
				int initprot = native_prot(seg->initprot);
				int useprot = (initprot & PROT_EXEC) ? maxprot : initprot;

				if (seg->filesize < seg->vmsize)
				{
					unsigned long map_addr;
					if (slide != 0)
					{
						unsigned long addr = seg->vmaddr;

						if (addr != 0)
							addr += slide;

						// printk(KERN_NOTICE "map to addr=0x%lx, size=0x%lx\n", addr, seg->vmsize);
						map_addr = vm_mmap(NULL, addr, seg->vmsize, useprot,
								MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, 0);

						if (BAD_ADDR(map_addr))
						{
							err = (int) map_addr;
							goto out;
						}
					}
					else
					{
						size_t size = seg->vmsize - seg->filesize;

						// printk(KERN_NOTICE "map to addr=0x%lx, size=0x%lx #2\n", PAGE_ALIGN(seg->vmaddr + seg->vmsize - size), PAGE_ROUNDUP(size));
						map_addr = vm_mmap(NULL, PAGE_ALIGN(seg->vmaddr + seg->vmsize - size), PAGE_ROUNDUP(size), useprot,
								MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, 0);
						if (BAD_ADDR(map_addr))
						{
							err = (int) map_addr;
							goto out;
						}
					}
				}

				if (seg->filesize > 0)
				{
					unsigned long map_addr;
					// printk(KERN_NOTICE "map to addr=0x%lx, size=0x%lx #3\n", seg->vmaddr+slide, seg->filesize);
					map_addr = vm_mmap(file, (seg->vmaddr + slide), seg->filesize, useprot,
							MAP_FIXED | MAP_PRIVATE, seg->fileoff + fat_offset);
					if (BAD_ADDR(map_addr))
					{
						err = -ENOMEM;
						goto out;
					}
				
					if (seg->fileoff == 0)
						mappedHeader = (struct MACH_HEADER_STRUCT*) (seg->vmaddr + slide);
				}

				if (seg->vmaddr + slide + seg->vmsize > lr->vm_addr_max)
					lr->vm_addr_max = seg->vmaddr + slide + seg->vmsize;

				if (strcmp(SEG_DATA, seg->segname) == 0)
				{
					// Look for section named __all_image_info for GDB/LLDB integration
					struct SECTION_STRUCT* sect = (struct SECTION_STRUCT*) (seg+1);
					struct SECTION_STRUCT* end = (struct SECTION_STRUCT*) (&cmds[p + lc->cmdsize]);

					while (sect < end)
					{
						if (strncmp(sect->sectname, "__all_image_info", 16) == 0)
						{
							lr->dyld_all_image_location = slide + sect->addr;
							lr->dyld_all_image_size = sect->size;
							break;
						}
						sect++;
					}
				}
				break;
			}
			case LC_UNIXTHREAD:
			{
#ifdef GEN_64BIT
				entryPoint = ((uint64_t*) lc)[18];
#endif
#ifdef GEN_32BIT
				entryPoint = ((uint32_t*) lc)[14];
#endif
				entryPoint += slide;

				break;
			}
			case LC_LOAD_DYLINKER:
			{
				if (header.filetype != MH_EXECUTE)
				{
					// dylinker can't reference another dylinker
					err = -ENOEXEC;
					goto out;
				}

				struct dylinker_command* dy = (struct dylinker_command*) lc;
				char* path;
				size_t length;
				struct file* dylinker = NULL;

				if (lr->root_path != NULL)
				{
					const size_t root_len = strlen(lr->root_path);
					const size_t linker_len = dy->cmdsize - dy->name.offset;

					length = linker_len + root_len;
					path = kmalloc(length+1, GFP_KERNEL);

					// Concat root path and linker path
					memcpy(path, lr->root_path, root_len);
					memcpy(path + root_len, ((char*) dy) + dy->name.offset, linker_len);
					path[length] = '\0';

					dylinker = open_exec(path);

					if (IS_ERR(dylinker))
					{
						err = PTR_ERR(dylinker);
						debug_msg("Failed to load dynamic linker (%s) for executable, error %d\n", path, err);
						dylinker = NULL;
					}
					else
						debug_msg("Loaded chrooted dyld at %s\n", path);
					kfree(path);
				}

				if (dylinker == NULL)
				{
					length = dy->cmdsize - dy->name.offset;
					path = kmalloc(length+1, GFP_KERNEL);

					memcpy(path, ((char*) dy) + dy->name.offset, length);
					path[length] = '\0';

					dylinker = open_exec(path);

					if (IS_ERR(dylinker))
					{
						err = PTR_ERR(dylinker);
						debug_msg("Failed to load dynamic linker (%s) for executable, error %d\n", path, err);
						kfree(path);
						goto out;
					}
					kfree(path);
				}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
				pos = 0;
				if (kernel_read(dylinker, bprm->buf, sizeof(bprm->buf), &pos) != sizeof(bprm->buf))
#else
				if (kernel_read(dylinker, 0, bprm->buf, sizeof(bprm->buf)) != sizeof(bprm->buf))
#endif
				{
					err = -EIO;
					fput(dylinker);
					goto out;
				}
				err = load(bprm, dylinker, header.cputype, lr);

				fput(dylinker);

				if (err != 0)
					goto out;

				break;
			}
			case LC_MAIN:
			{
				struct entry_point_command* ee = (struct entry_point_command*) lc;
				if (ee->stacksize > lr->stack_size)
					lr->stack_size = ee->stacksize;
				break;
			}
			case LC_UUID:
			{
				if (header.filetype == MH_EXECUTE)
				{
					struct uuid_command* ue = (struct uuid_command*) lc;
					memcpy(lr->uuid, ue->uuid, sizeof(ue->uuid));
				}
				break;
			}
		}

		p += lc->cmdsize;
	}

	if (header.filetype == MH_EXECUTE)
		lr->mh = (uintptr_t) mappedHeader;
	if (entryPoint && !lr->entry_point)
		lr->entry_point = entryPoint;

out:
	kfree(cmds);

	if (override_cred != NULL)
	{
		revert_creds(old_cred);
		put_cred(override_cred);
	}

	return err;
}


#undef FUNCTION_NAME
#undef SEGMENT_STRUCT
#undef SEGMENT_COMMAND
#undef MACH_HEADER_STRUCT
#undef SECTION_STRUCT
#undef MAP_EXTRA

