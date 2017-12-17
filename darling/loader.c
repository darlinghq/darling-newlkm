
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
	uintptr_t mmapSize = 0;
	bool pie = false;
	uint32_t fat_offset;
	int err = 0;
	uint32_t i, p;

	fat_offset = farch ? farch->offset : 0;

	if (kernel_read(file, fat_offset, (char*) &header, sizeof(header)) != sizeof(header))
		return -EIO;

	if (header.filetype != (expect_dylinker ? MH_DYLINKER : MH_EXECUTE))
		return -ENOEXEC;

	cmds = (uint8_t*) kmalloc(header.sizeofcmds, GFP_KERNEL);
	if (!cmds)
		return -ENOMEM;

	if (kernel_read(file, fat_offset + sizeof(header), cmds, header.sizeofcmds) != header.sizeofcmds)
	{
		kfree(cmds);
		return -EIO;
	}

	if ((header.filetype == MH_EXECUTE && header.flags & MH_PIE) || header.filetype == MH_DYLINKER)
	{
		uintptr_t base = -1;

		// Go through all SEGMENT_COMMAND commands to get the total continuous range required.
		for (i = 0, p = 0; i < header.ncmds; i++)
		{
			struct SEGMENT_STRUCT* seg = (struct SEGMENT_STRUCT*) &cmds[p];

			// Load commands are always sorted, so this will get us the maximum address.
			if (seg->cmd == SEGMENT_COMMAND)
			{
				if (base == -1)
				{
					base = seg->vmaddr;
					if (base != 0 && header.filetype == MH_DYLINKER)
						goto no_slide;
				}
				mmapSize = seg->vmaddr + seg->vmsize - base;
			}

			p += seg->cmdsize;
		}

		slide = vm_mmap(NULL, base, mmapSize, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_EXTRA, 0);
		if (BAD_ADDR(slide))
		{
			err = -ENOMEM;
			goto out;
		}

		pie = true;
	}
no_slide:

	for (i = 0, p = 0; i < header.ncmds; i++)
	{
		struct load_command* lc;

		lc = (struct load_command*) &cmds[p];

		switch (lc->cmd)
		{
			case SEGMENT_COMMAND:
			{
				struct SEGMENT_STRUCT* seg = (struct SEGMENT_STRUCT*) lc;
				void* rv;

				// TODO: can we get rid of this special handling?
				if (memcmp(SEG_PAGEZERO, seg->segname, sizeof(SEG_PAGEZERO)) == 0)
				{
					unsigned long map_addr;

					map_addr = vm_mmap(NULL, 0, PAGE_SIZE, PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, 0);

					if (map_addr != 0)
					{
						// TODO: print a warning
					}

					p += lc->cmdsize;
					continue;
				}

				if (seg->filesize < seg->vmsize)
				{
					if (slide != 0)
					{
						// Some segments' filesize != vmsize, thus this mprotect().
						unsigned long map_addr;

						map_addr = vm_mmap(NULL, (seg->vmaddr + slide), seg->vmsize, native_prot(seg->maxprot),
								MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, 0);

						if (BAD_ADDR(map_addr))
						{
							err = -ENOMEM;
							goto out;
						}
					}
					else
					{
						size_t size = seg->vmsize - seg->filesize;
						unsigned long map_addr;

						map_addr = vm_mmap(NULL, PAGE_ALIGN(seg->vmaddr + seg->vmsize - size), PAGE_ROUNDUP(size), native_prot(seg->maxprot),
								MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, 0);
						if (BAD_ADDR(map_addr))
						{
							err = -ENOMEM;
							goto out;
						}
					}
				}

				if (seg->filesize > 0)
				{
					unsigned long map_addr;
					map_addr = vm_mmap(file, (seg->vmaddr + slide), seg->filesize, native_prot(seg->maxprot),
							MAP_FIXED | MAP_PRIVATE, seg->fileoff + fat_offset);
					if (BAD_ADDR(map_addr))
					{
						err = -ENOMEM;
						goto out;
					}
				
					if (seg->fileoff == 0)
						mappedHeader = (struct MACH_HEADER_STRUCT*) (seg->vmaddr + slide);
				}

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
				size_t length = dy->cmdsize - dy->name.offset;

				if (length > PAGE_SIZE-1)
				{
					err = -EIO;
					goto out;
				}

				path = kmalloc(length+1, GFP_KERNEL);

				memcpy(path, ((char*) dy) + dy->name.offset, length);
				path[length] = '\0';

				struct file* dylinker = open_exec(path);
				kfree(path);

				if (IS_ERR(dylinker))
				{
					err = PTR_ERR(dylinker);
					goto out;
				}
				if (kernel_read(dylinker, 0, bprm->buf, sizeof(bprm->buf)) != sizeof(bprm->buf))
				{
					err = -EIO;
					fput(dylinker);
					goto out;
				}
				err = load(bprm, dylinker, NULL, header.cputype, lr);

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
	lr->entry_point = entryPoint;

out:
	kfree(cmds);

	return err;
}


#undef FUNCTION_NAME
#undef SEGMENT_STRUCT
#undef SEGMENT_COMMAND
#undef MACH_HEADER_STRUCT
#undef SECTION_STRUCT
#undef MAP_EXTRA

