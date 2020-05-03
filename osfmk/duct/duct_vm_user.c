/*
Copyright (c) 2014-2017, Wenqi Chen
Copyright (c) 2017-2020, Lubos Dolezel

Shanghai Mifu Infotech Co., Ltd
B112-113, IF Industrial Park, 508 Chunking Road, Shanghai 201103, China


All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


*/

#include "duct.h"
#include "duct_pre_xnu.h"
#include "duct_vm_user.h"
#include "duct_kern_printf.h"
#include <ipc/ipc_port.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0)
#include "access_process_vm.h"
#endif

#include <vm/vm_map.h>

#define current linux_current

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
#include <linux/sched/mm.h>
#endif

#define BAD_ADDR(x)     ((unsigned long)(x) >= TASK_SIZE)

#undef vmalloc
#undef vfree
#undef kfree

kern_return_t duct_mach_vm_allocate (vm_map_t map, mach_vm_offset_t * addr, mach_vm_size_t size, int flags)
{
        // assert (current_map () == map);
        vm_map_offset_t map_addr;
        vm_map_size_t    map_size;
        kern_return_t    result;
        boolean_t    anywhere;

        if (map->linux_task == NULL) { // kernel allocation
            *addr = (mach_vm_offset_t) vmalloc(size);
            return (*addr) ? KERN_SUCCESS : KERN_NO_SPACE;
        }

        /* filter out any kernel-only flags */
           if (flags & ~VM_FLAGS_USER_ALLOCATE)
            return KERN_INVALID_ARGUMENT;

#if defined (__DARLING__)
#else
        if (map == VM_MAP_NULL)
            return(KERN_INVALID_ARGUMENT);
#endif

        if (size == 0) {
            *addr = 0;
            return(KERN_SUCCESS);
        }

        anywhere = ((VM_FLAGS_ANYWHERE & flags) != 0);

        if (anywhere) {
            /*
             * No specific address requested, so start candidate address
             * search at the minimum address in the map.  However, if that
             * minimum is 0, bump it up by PAGE_SIZE.  We want to limit
             * allocations of PAGEZERO to explicit requests since its
             * normal use is to catch dereferences of NULL and many
             * applications also treat pointers with a value of 0 as
             * special and suddenly having address 0 contain useable
             * memory would tend to confuse those applications.
             */
#if defined (__DARLING__)
            map_addr    = *addr;
#else
            map_addr = vm_map_min(map);
#endif
            if (map_addr == 0)
                map_addr += PAGE_SIZE;

        } else
            map_addr = vm_map_trunc_page(*addr, PAGE_MASK);

        map_size = vm_map_round_page(size, PAGE_MASK);
        if (map_size == 0) {
          return(KERN_INVALID_ARGUMENT);
        }

#if defined (__DARLING__)

        vm_prot_t       vm_prot         = VM_PROT_DEFAULT;

        unsigned long   map_prot        = 0;

        /* standard conversion */
        if (vm_prot & VM_PROT_READ)         map_prot   |= PROT_READ;
        if (vm_prot & VM_PROT_WRITE)        map_prot   |= PROT_WRITE;
        if (vm_prot & VM_PROT_EXECUTE)      map_prot   |= PROT_EXEC;

        int             map_flags       = MAP_PRIVATE | MAP_ANONYMOUS;

        if (! flags & VM_FLAGS_ANYWHERE)    map_flags  |= MAP_FIXED;

        // down_write (&linux_current->mm->mmap_sem);

        /* WC - todo: should potentially be just a vm_brk with prot flags */
        // printk (KERN_NOTICE "mmap: requested addr: 0x%x\n", map_addr);

        map_addr    =
        vm_mmap (NULL, map_addr, map_size, map_prot, map_flags, 0);

        // printk ( KERN_NOTICE "mmap: returned addr: 0x%x, bad: %d\n", 
        //          map_addr, BAD_ADDR (map_addr) ? 1 : 0 );

        result      = BAD_ADDR (map_addr) ? KERN_NO_SPACE : KERN_SUCCESS;

        // printk (KERN_NOTICE "mmap: result: 0x%x\n", result);


        // up_write (&linux_current->mm->mmap_sem);


#else
        result = vm_map_enter(
                map,
                &map_addr,
                map_size,
                (vm_map_offset_t)0,
                flags,
                VM_OBJECT_NULL,
                (vm_object_offset_t)0,
                FALSE,
                VM_PROT_DEFAULT,
                VM_PROT_ALL,
                VM_INHERIT_DEFAULT);
#endif

        *addr = map_addr;
        return(result);
}

kern_return_t
mach_vm_deallocate(
	vm_map_t		map,
	mach_vm_offset_t	start,
	mach_vm_size_t	size)
{
    if (map->linux_task == NULL) { // kernel allocation
        vfree((void *) start);
        return KERN_SUCCESS;
    }

    vm_munmap(start, size);
    return KERN_SUCCESS;
}

/*
 * mach_vm_copy -
 * Overwrite one range of the specified map with the contents of
 * another range within that same map (i.e. both address ranges
 * are "over there").
 */
kern_return_t
mach_vm_copy(
	vm_map_t		map,
	mach_vm_address_t	source_address,
	mach_vm_size_t	size,
	mach_vm_address_t	dest_address)
{
	kern_return_t rv = KERN_SUCCESS;
	int done;
	void* mem;
	struct task_struct* ltask = map->linux_task;

	if (size > 100*1024*1024)
	{
		printf("BUG: Refusing to vm_copy %d bytes\n", size);
		return KERN_FAILURE;
	}

	mem = vmalloc(size);
	if (mem == NULL)
	{
		printf("out of memory in mach_vm_copy\n");
		return KERN_NO_SPACE;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
	done = access_process_vm(ltask, source_address, mem, size, 0);
#else
	// Older kernels don't export access_process_vm(),
	// so we need to fall back to our own copy in access_process_vm.h
	done = __access_process_vm(ltask, source_address, mem, size, 0);
#endif

	if (done != size)
	{
		printf("mach_vm_copy(): only read %d bytes, %d expected\n", done, size);

		rv = KERN_NO_SPACE;
		goto failure;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
	done = access_process_vm(ltask, dest_address, mem, size, 1);
#else
	done = __access_process_vm(ltask, dest_address, mem, size, 1);
#endif

	if (done != size)
	{
		printf("mach_vm_copy(): only wrote %d bytes, %d expected\n", done, size);
		rv = KERN_NO_SPACE;
	}

failure:
	vfree(mem);
	return rv;
}

kern_return_t
vm_copy(
	vm_map_t	map,
	vm_address_t	source_address,
	vm_size_t	size,
	vm_address_t	dest_address)
{
	return mach_vm_copy(map, source_address, size, dest_address);
}

kern_return_t
mach_vm_region(
    vm_map_t         map,
    mach_vm_offset_t    *address,       /* IN/OUT */
    mach_vm_size_t  *size,          /* OUT */
    vm_region_flavor_t   flavor,        /* IN */
    vm_region_info_t     info,          /* OUT */
    mach_msg_type_number_t  *count,         /* IN/OUT */
    mach_port_t     *object_name)       /* OUT */
{
	switch (flavor)
	{
		case VM_REGION_BASIC_INFO:
		case VM_REGION_BASIC_INFO_64:
		{
			kern_return_t rv = KERN_SUCCESS;
			struct mm_struct* mm;
			struct vm_area_struct* vma;
			struct task_struct* ltask = map->linux_task;

			mm = ltask->mm;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
			if (!mm || !mmget_not_zero(mm))
#else
			if (!mm || !atomic_inc_not_zero(&mm->mm_users))
#endif
				return KERN_FAILURE;

			down_read(&mm->mmap_sem);

			vma = find_vma(mm, *address);
			if (vma == NULL)
			{
				rv = KERN_INVALID_ADDRESS;
				goto out;
			}

			*address = vma->vm_start;
			*size = vma->vm_end - vma->vm_start;

			if (flavor == VM_REGION_BASIC_INFO_64)
			{
				vm_region_basic_info_64_t out = (vm_region_basic_info_64_t) info;

				if (*count < VM_REGION_BASIC_INFO_COUNT_64)
				{
					rv = KERN_INVALID_ARGUMENT;
					goto out;
				}
				*count = VM_REGION_BASIC_INFO_COUNT_64;

				out->protection = 0;

				if (vma->vm_flags & VM_READ)
					out->protection |= VM_PROT_READ;
				if (vma->vm_flags & VM_WRITE)
					out->protection |= VM_PROT_WRITE;
				// This is a special hack for LLDB. For processes started as suspended, with two RX segments.
				// However, in order to avoid failures, they are actually mapped as RWX and are to be changed to RX later by dyld.
				if (vma->vm_flags & VM_EXEC)
					//out->protection |= VM_PROT_EXECUTE;
					out->protection = VM_PROT_EXECUTE | VM_PROT_READ;

				out->offset = vma->vm_pgoff * PAGE_SIZE;
				out->shared = !!(vma->vm_flags & VM_MAYSHARE);
				out->behavior = VM_BEHAVIOR_DEFAULT;
				out->user_wired_count = 0;
				out->inheritance = 0;
				out->max_protection = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
				out->reserved = FALSE;
			}
			else
			{
				vm_region_basic_info_t out = (vm_region_basic_info_t) info;

				if (*count < VM_REGION_BASIC_INFO_COUNT)
				{
					rv = KERN_INVALID_ARGUMENT;
					goto out;
				}
				*count = VM_REGION_BASIC_INFO_COUNT;

				out->protection = 0;

				if (vma->vm_flags & VM_READ)
					out->protection |= VM_PROT_READ;
				if (vma->vm_flags & VM_WRITE)
					out->protection |= VM_PROT_WRITE;
				// This is a special hack for LLDB. For processes started as suspended, with two RX segments.
				// However, in order to avoid failures, they are actually mapped as RWX and are to be changed to RX later by dyld.
				if (vma->vm_flags & VM_EXEC)
					out->protection = VM_PROT_EXECUTE | VM_PROT_READ;

				out->offset = vma->vm_pgoff * PAGE_SIZE;
				out->shared = !!(vma->vm_flags & VM_MAYSHARE);
				out->behavior = VM_BEHAVIOR_DEFAULT;
				out->user_wired_count = 0;
				out->inheritance = 0;
				out->max_protection = VM_READ | VM_WRITE | VM_EXEC;
				out->reserved = FALSE;
			}

out:
			up_read(&mm->mmap_sem);
			mmput(mm);

			if (object_name)
				*object_name = IP_NULL;

			return rv;
		}
		default:
			return KERN_INVALID_ARGUMENT;
	}
}


kern_return_t
mach_vm_region_recurse(
    vm_map_t            map,
    mach_vm_address_t       *address,
    mach_vm_size_t      *size,
    uint32_t            *depth,
    vm_region_recurse_info_t    info,
    mach_msg_type_number_t  *infoCnt)
{
	if (depth != NULL)
		*depth = 0;

	kern_return_t rv = KERN_SUCCESS;
	struct mm_struct* mm;
	struct vm_area_struct* vma;
	struct task_struct* ltask = map->linux_task;

	mm = ltask->mm;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
	if (!mm || !mmget_not_zero(mm))
#else
	if (!mm || !atomic_inc_not_zero(&mm->mm_users))
#endif
		return KERN_FAILURE;

	down_read(&mm->mmap_sem);

	vma = find_vma(mm, *address);
	if (vma == NULL)
	{
		rv = KERN_INVALID_ADDRESS;
		goto out;
	}

	*address = vma->vm_start;
	*size = vma->vm_end - vma->vm_start;

	if (*infoCnt == VM_REGION_SUBMAP_SHORT_INFO_COUNT_64)
	{
		vm_region_submap_info_64_t out = (vm_region_submap_info_64_t) info;

		memset(out, 0, sizeof(*out));
		out->protection = 0;

		if (vma->vm_flags & VM_READ)
			out->protection |= VM_PROT_READ;
		if (vma->vm_flags & VM_WRITE)
			out->protection |= VM_PROT_WRITE;
		if (vma->vm_flags & VM_EXEC)
			out->protection |= VM_PROT_EXECUTE;

		out->offset = vma->vm_pgoff * PAGE_SIZE;
		out->share_mode = (vma->vm_flags & VM_MAYSHARE) ? SM_SHARED : SM_PRIVATE;
		out->max_protection = VM_READ | VM_WRITE | VM_EXEC;
	}
	else if (*infoCnt == VM_REGION_SUBMAP_SHORT_INFO_COUNT_64)
	{
		vm_region_submap_short_info_64_t out = (vm_region_submap_short_info_64_t) info;

		memset(out, 0, sizeof(*out));
		out->protection = 0;

		if (vma->vm_flags & VM_READ)
			out->protection |= VM_PROT_READ;
		if (vma->vm_flags & VM_WRITE)
			out->protection |= VM_PROT_WRITE;
		if (vma->vm_flags & VM_EXEC)
			out->protection |= VM_PROT_EXECUTE;

		out->offset = vma->vm_pgoff * PAGE_SIZE;
		out->share_mode = (vma->vm_flags & VM_MAYSHARE) ? SM_SHARED : SM_PRIVATE;
		out->max_protection = VM_READ | VM_WRITE | VM_EXEC;
	}
	else
		rv = KERN_INVALID_ARGUMENT;

out:
	up_read(&mm->mmap_sem);
	mmput(mm);

	return rv;
}

static int remap_release(struct inode* inode, struct file* file);
static int remap_mmap(struct file *filp, struct vm_area_struct *vma);
static const struct file_operations remap_ops =
{
	.owner = THIS_MODULE,
	.release = remap_release,
	.mmap = remap_mmap,
};

struct remap_data
{
	struct page** pages;
	unsigned long page_count;
};

// This symbol is not exported from Linux
void mm_trace_rss_stat(struct mm_struct *mm, int member, long count) {}

kern_return_t
mach_vm_remap(
	vm_map_t		target_map,
	mach_vm_offset_t	*address,
	mach_vm_size_t	size,
	mach_vm_offset_t	mask,
	int			flags,
	vm_map_t		src_map,
	mach_vm_offset_t	memory_address,
	boolean_t		copy,
	vm_prot_t		*cur_protection,
	vm_prot_t		*max_protection,
	vm_inherit_t		inheritance)
{
	kern_return_t ret = KERN_SUCCESS;

	if (src_map->linux_task == NULL || target_map->linux_task == NULL)
		return KERN_FAILURE;
	if (target_map != current_task()->map)
		return KERN_NOT_SUPPORTED;

	unsigned int gup_flags = FOLL_WRITE | FOLL_POPULATE;
	unsigned int map_prot = PROT_READ | PROT_WRITE;

	down_read(&src_map->linux_task->mm->mmap_sem);

	unsigned long page_start = memory_address & LINUX_PAGE_MASK;
	unsigned long nr_pages = ((memory_address + size + PAGE_SIZE - page_start - 1) >> PAGE_SHIFT);

	printk(KERN_NOTICE "mach_vm_remap(): addr 0x%lx, page_start: 0x%lx, nr_pages: %d\n", memory_address, page_start, nr_pages);

	struct page** pages = (struct page**) kmalloc(sizeof(struct page*) * nr_pages, GFP_KERNEL);
	long got_pages;

	got_pages = get_user_pages_remote(NULL, src_map->linux_task->mm, page_start, nr_pages, gup_flags,
		pages, NULL, NULL);

	if (got_pages == -LINUX_EFAULT)
	{
		// retry R/O
		gup_flags &= ~FOLL_WRITE;
		map_prot &= ~PROT_WRITE;

		got_pages = get_user_pages_remote(NULL, src_map->linux_task->mm, page_start, nr_pages, gup_flags,
			pages, NULL, NULL);
	}

	up_read(&src_map->linux_task->mm->mmap_sem);

	if (got_pages != nr_pages)
	{
		if (got_pages >= 0)
			printk(KERN_WARNING "mach_vm_remap(): wanted %d pages, got %d pages\n", nr_pages, got_pages);
		else
			printk(KERN_WARNING "mach_vm_remap(): get_user_pages_remote() failed with %ld\n", got_pages);

		ret = KERN_MEMORY_ERROR;
		goto err;
	}
	
	unsigned int map_flags = 0;

     if (!(flags & VM_FLAGS_ANYWHERE))
	 	map_flags |= MAP_FIXED;

	unsigned long map_size = nr_pages * PAGE_SIZE;
	unsigned long map_addr = *address;
	
	if (copy)
	{
		map_flags |= MAP_PRIVATE;
		for (long i = 0; i < got_pages; i++)
		{
			struct page* copied = alloc_page(GFP_USER);
			copy_page(page_address(copied), page_address(pages[i]));
			put_page(pages[i]);
			pages[i] = copied;
		}
	}
	else
		map_flags |= MAP_SHARED;

	struct remap_data* remap = kmalloc(sizeof(struct remap_data), GFP_KERNEL);
	remap->pages = pages;
	remap->page_count = nr_pages;

	struct file* f = anon_inode_getfile("mach_vm_remap", &remap_ops, remap, O_RDWR);
	if (IS_ERR(f))
	{
		kfree(remap);
		printk(KERN_WARNING "mach_vm_remap(): anon_inode_getfile() failed: %ld\n", PTR_ERR(f));
		ret = KERN_FAILURE;
		goto err;
	}
	pages = NULL;

	map_addr = vm_mmap(f, map_addr, map_size, map_prot, map_flags, 0);
	fput(f);

	if (BAD_ADDR(map_addr))
	{
		printk(KERN_WARNING "mach_vm_remap(): vm_mmap() failed: %ld\n", map_addr);
		ret = KERN_NO_SPACE;
		goto err;
	}

	if (!copy)
	{
		// Fixup page accounting to avoid warnings
		for (long i = 0; i < got_pages; i++)
		{
			dec_mm_counter(linux_current->mm, MM_SHMEMPAGES);
			inc_mm_counter(linux_current->mm, mm_counter(remap->pages[i]));
		}
	}

	*cur_protection = VM_PROT_READ;
	if (map_prot & PROT_WRITE)
		*cur_protection |= VM_PROT_WRITE;

	*max_protection = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;

	*address = map_addr;
	if (flags & VM_FLAGS_RETURN_DATA_ADDR)
		*address += memory_address & (PAGE_SIZE - 1);
	
err:
	if (pages != NULL)
	{
		for (long i = 0; i < got_pages; i++)
			put_page(pages[i]);

		kfree(pages);
	}
	return ret;
}

static int remap_release(struct inode* inode, struct file* file)
{
	struct remap_data* data = (struct remap_data*) file->private_data;
	for (long i = 0; i < data->page_count; i++)
		put_page(data->pages[i]);

	kfree(data->pages);
	kfree(data);

	return 0;
}

static vm_fault_t remap_vm_fault(struct vm_fault* vmf)
{
	struct remap_data* data = (struct remap_data*) vmf->vma->vm_private_data;
	vmf->page = data->pages[vmf->pgoff];
	get_page(vmf->page);
	return 0;
}

static struct vm_operations_struct remap_vm_ops = {
	.fault = remap_vm_fault,
};

static int remap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	vma->vm_ops = &remap_vm_ops;
	vma->vm_private_data = filp->private_data;
	vma->vm_flags |= VM_DONTEXPAND;
	
	return 0;
}

