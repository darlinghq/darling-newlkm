/*
Copyright (c) 2014-2017, Wenqi Chen

Shanghai Mifu Infotech Co., Ltd
B112-113, IF Industrial Park, 508 Chunking Road, Shanghai 201103, China

Copyright (C) 2017 Lubos Dolezel

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
#include <darling/debug_print.h>
#include "duct_pre_xnu.h"
#include "duct_vm_map.h"
#include "duct_kern_kalloc.h"

#include <vm/vm_map.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,10,0)
#include "access_process_vm.h"
#endif

#undef kfree
extern void kfree (const void * objp);


static zone_t    vm_map_zone;        /* zone for vm_map structures */
// static zone_t    vm_map_entry_zone;    /* zone for vm_map_entry structures */
// static zone_t    vm_map_entry_reserved_zone;    /* zone with reserve for non-blocking
//                      * allocations */
static zone_t    vm_map_copy_zone;    /* zone for vm_map_copy structures */
extern vm_size_t msg_ool_size_small;
extern vm_map_t kernel_map;

extern int duct_print_map (struct mm_struct * mm, unsigned long start, size_t len);
extern kern_return_t vm_deallocate(vm_map_t map, mach_vm_offset_t start, mach_vm_size_t size);

void duct_vm_map_init (void)
{
#if defined (__DARLING__)
#else
    vm_size_t entry_zone_alloc_size;
    const char *mez_name = "VM map entries";
#endif

    vm_map_zone = zinit((vm_map_size_t) sizeof(struct _vm_map), 40*1024,
                PAGE_SIZE, "maps");
    zone_change(vm_map_zone, Z_NOENCRYPT, TRUE);

    vm_map_copy_zone = duct_zinit ((vm_map_size_t) sizeof(struct vm_map_copy),
                 16 * 1024, PAGE_SIZE, "VM_map_copies");
    duct_zone_change (vm_map_copy_zone, Z_NOENCRYPT, TRUE);

    kernel_map = duct_vm_map_create(NULL);

#if defined (__DARLING__)
#else
// #if    defined(__LP64__)
//     entry_zone_alloc_size = PAGE_SIZE * 5;
// #else
//     entry_zone_alloc_size = PAGE_SIZE * 6;
// #endif
//     vm_map_entry_zone = zinit((vm_map_size_t) sizeof(struct vm_map_entry),
//                   1024*1024, entry_zone_alloc_size,
//                   mez_name);
//     zone_change(vm_map_entry_zone, Z_NOENCRYPT, TRUE);
//     zone_change(vm_map_entry_zone, Z_NOCALLOUT, TRUE);
//     zone_change(vm_map_entry_zone, Z_GZALLOC_EXEMPT, TRUE);
//
//     vm_map_entry_reserved_zone = zinit((vm_map_size_t) sizeof(struct vm_map_entry),
//                    kentry_data_size * 64, kentry_data_size,
//                    "Reserved VM map entries");
//     zone_change(vm_map_entry_reserved_zone, Z_NOENCRYPT, TRUE);
//
//     vm_map_copy_zone = zinit((vm_map_size_t) sizeof(struct vm_map_copy),
//                  16*1024, PAGE_SIZE, "VM map copies");
//     zone_change(vm_map_copy_zone, Z_NOENCRYPT, TRUE);
//
//     /*
//      *    Cram the map and kentry zones with initial data.
//      *    Set reserved_zone non-collectible to aid zone_gc().
//      */
//     zone_change(vm_map_zone, Z_COLLECT, FALSE);
//
//     zone_change(vm_map_entry_reserved_zone, Z_COLLECT, FALSE);
//     zone_change(vm_map_entry_reserved_zone, Z_EXPAND, FALSE);
//     zone_change(vm_map_entry_reserved_zone, Z_FOREIGN, TRUE);
//     zone_change(vm_map_entry_reserved_zone, Z_NOCALLOUT, TRUE);
//     zone_change(vm_map_entry_reserved_zone, Z_CALLERACCT, FALSE); /* don't charge caller */
//     zone_change(vm_map_copy_zone, Z_CALLERACCT, FALSE); /* don't charge caller */
//     zone_change(vm_map_entry_reserved_zone, Z_GZALLOC_EXEMPT, TRUE);
//
//     zcram(vm_map_zone, (vm_offset_t)map_data, map_data_size);
//     zcram(vm_map_entry_reserved_zone, (vm_offset_t)kentry_data, kentry_data_size);
//
//     lck_grp_attr_setdefault(&vm_map_lck_grp_attr);
//     lck_grp_init(&vm_map_lck_grp, "vm_map", &vm_map_lck_grp_attr);
//     lck_attr_setdefault(&vm_map_lck_attr);
//
// #if CONFIG_FREEZE
//     default_freezer_init();
// #endif /* CONFIG_FREEZE */
#endif
}

void duct_vm_map_deallocate(vm_map_t map)
{
	if (map != NULL)
	{
		duct_zfree(vm_map_zone, map);
		if (map->linux_task != NULL)
			put_task_struct(map->linux_task);
	}
}

os_refgrp_decl(static, map_refgrp, "vm_map", NULL);

// Calling with a NULL task makes other funcs consider the map a kernel map
vm_map_t duct_vm_map_create (struct task_struct* linux_task)
{
#if defined (__DARLING__)
#else
        // static int        color_seed = 0;
#endif
        register vm_map_t    result;

        result = (vm_map_t) zalloc(vm_map_zone);
        if (result == VM_MAP_NULL)
            panic("vm_map_create");


#if defined (__DARLING__)
#else
    //     vm_map_first_entry(result) = vm_map_to_entry(result);
    //     vm_map_last_entry(result)  = vm_map_to_entry(result);
    //     result->hdr.nentries = 0;
    //     result->hdr.entries_pageable = pageable;
    //
    //     vm_map_store_init( &(result->hdr) );
    //
    //     result->size = 0;
    //     result->user_wire_limit = MACH_VM_MAX_ADDRESS;    /* default limit is unlimited */
    //     result->user_wire_size  = 0;
    //     result->ref_count = 1;
    // #if    TASK_SWAPPER
    //     result->res_count = 1;
    //     result->sw_state = MAP_SW_IN;
    // #endif    /* TASK_SWAPPER */
    //     result->pmap = pmap;
    //     result->min_offset = min;
    //     result->max_offset = max;
    //     result->wiring_required = FALSE;
    //     result->no_zero_fill = FALSE;
    //     result->mapped_in_other_pmaps = FALSE;
    //     result->wait_for_space = FALSE;
    //     result->switch_protect = FALSE;
    //     result->disable_vmentry_reuse = FALSE;
    //     result->map_disallow_data_exec = FALSE;
    //     result->highest_entry_end = 0;
    //     result->first_free = vm_map_to_entry(result);
    //     result->hint = vm_map_to_entry(result);
    //     result->color_rr = (color_seed++) & vm_color_mask;
    //      result->jit_entry_exists = FALSE;
    // #if CONFIG_FREEZE
    //     result->default_freezer_handle = NULL;
    // #endif
    //     vm_map_lock_init(result);
    //     lck_mtx_init_ext(&result->s_lock, &result->s_lock_ext, &vm_map_lck_grp, &vm_map_lck_attr);
#endif

#if defined (__DARLING__)
	if (linux_task)
		get_task_struct(linux_task);
	result->linux_task = linux_task;
    result->max_offset = darling_is_task_64bit() ? 0x7fffffffffffull : VM_MAX_ADDRESS;
	os_ref_init_count(&result->map_refcnt, &map_refgrp, 1);
    result->hdr.page_shift = PAGE_SHIFT;
#endif

        return(result);
}


// #undef vm_map_copyin
// kern_return_t duct_vm_map_copyin (vm_map_t            src_map,
//     vm_map_address_t src_addr,
//     vm_map_size_t len,
//     boolean_t src_destroy,
//     vm_map_copy_t * copy_result)    /* OUT */
// {
//         return
//         vm_map_copyin_common (src_map, src_addr, len, src_destroy, FALSE, copy_result, FALSE);
// }

extern vm_map_t kernel_map, ipc_kernel_map;

boolean_t vm_kernel_map_is_kernel(vm_map_t map)
{
    return map == kernel_map || map == ipc_kernel_map;
}

kern_return_t duct_vm_map_copyin_common ( vm_map_t src_map, vm_map_address_t src_addr, vm_map_size_t len,
                                          boolean_t src_destroy, boolean_t src_volatile,
                                          vm_map_copy_t * copy_result, boolean_t use_maxprot )
{
        debug_msg( "- vm_map_copyin_common to current_map: %p from src_map: %p, addr: %p, len: 0x%llx destroy: %d\n",
                 current_map (), src_map, (void*) src_addr, len, src_destroy );

        // dump_stack ();

        vm_map_copy_t       copy;        /* Resulting copy */

        if (len == 0) {
                *copy_result    = VM_MAP_COPY_NULL;
                return KERN_SUCCESS;
        }

        // assert (src_map == current_map ());
        // assert (len > msg_ool_size_small);
        //copy            = (vm_map_copy_t) duct_zalloc (vm_map_copy_zone);
		copy = (vm_map_copy_t) vmalloc_user(sizeof(struct vm_map_copy) + len);

        if (copy == NULL) {
                return KERN_RESOURCE_SHORTAGE;
        }

        copy->type      = VM_MAP_COPY_KERNEL_BUFFER;
        // copy->map       = src_map;
        // copy->offset    = src_start;
        copy->size      = len;
        copy->offset    = 0;

        switch (copy->type) {
                // TODO: this could hopefully be optimized by calling find_vma
                // and transferring the pages
                case VM_MAP_COPY_KERNEL_BUFFER:
                {
                        //copy->cpy_kdata = vmalloc_user(len);
                        //copy->cpy_kalloc_size = len;

                        if (src_map->linux_task != NULL) { // if this is not a kernel map
                            if (src_addr) {

                                struct task_struct* ltask = src_map->linux_task;
                                int rd;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
                                rd = access_process_vm(ltask, src_addr, copy->cpy_kdata, len, 0);
#else
                                // Older kernels don't export access_process_vm(),
                                // so we need to fall back to our own copy in access_process_vm.h
                                rd = __access_process_vm(ltask, src_addr, copy->cpy_kdata, len, 0);
#endif
                                if (rd != len) {
                                        debug_msg("Failed to copy memory in duct_vm_map_copyin_common()\n");
                                        // vfree (copy->cpy_kdata);
                                        // duct_zfree (vm_map_copy_zone, copy);
                                        return KERN_NO_SPACE;
                                }
                            }

                            // FIXME: This disregards src_map!
                            if (src_destroy && src_addr)
                                vm_munmap(src_addr, len);
                        }
                        else if (vm_kernel_map_is_kernel(src_map)) { // it is a kernel map
                            if (src_addr) {
                                memcpy(copy->cpy_kdata, (void *) src_addr, len);

                                if (src_destroy)
                                    vm_deallocate(src_map, src_addr, len);
                            }
                        }
                        else
                            return KERN_FAILURE;

                        debug_msg("- copying OK\n");
                        *copy_result = copy;
                        return KERN_SUCCESS;
                }
                default:
                {
                    printk(KERN_ERR "Unknown copyin type %d\n", copy->type);
                    return KERN_FAILURE;
                }
        }
}

void duct_vm_map_copy_discard (vm_map_copy_t copy)
{
        debug_msg("vm_map_copy_discard\n");
        if (copy == VM_MAP_COPY_NULL)   return;

        switch (copy->type) {
                case VM_MAP_COPY_KERNEL_BUFFER:
                {
                    // vfree(copy->cpy_kdata);
                    break;
                }
        }

        //duct_zfree (vm_map_copy_zone, copy);
		vfree(copy);
}

kern_return_t
vm_map_copy_overwrite(
	vm_map_t	dst_map,
	vm_map_offset_t	dst_addr,
	vm_map_copy_t	copy,
	boolean_t	interruptible)
{
	struct task_struct* ltask = dst_map->linux_task;

	switch (copy->type)
	{
		case VM_MAP_COPY_KERNEL_BUFFER:
		{
			int wr;

			// We use FOLL_FORCE because we cannot reasonably implement mach_vm_protect().
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
			wr = access_process_vm(ltask, dst_addr, copy->cpy_kdata, copy->size, FOLL_WRITE | FOLL_FORCE);
#else
			// Older kernels don't export access_process_vm(),
			// so we need to fall back to our own copy in access_process_vm.h
			wr = __access_process_vm(ltask, dst_addr, copy->cpy_kdata, copy->size, FOLL_WRITE | FOLL_FORCE);
#endif
			if (wr != copy->size) {
					printk(KERN_ERR "Failed to copy memory\n");
					return KERN_INVALID_ADDRESS;
			}

            vfree (copy);

			return KERN_SUCCESS;
		}
		default:
			return KERN_FAILURE;
	}
}

kern_return_t duct_vm_map_copyout (vm_map_t dst_map, vm_map_address_t * dst_addr, vm_map_copy_t copy)
{
    return vm_map_copyout_size(dst_map, dst_addr, copy, copy->size);
}

kern_return_t vm_map_copyout_size(vm_map_t dst_map, vm_map_address_t* dst_addr, vm_map_copy_t copy, vm_map_size_t copy_size)
{
        debug_msg( "- vm_map_copyout (current_map: 0x%p) to dst_map: 0x%p, addr: 0x%p, copy: 0x%p\n",
                 current_map (), dst_map, (void*) dst_addr, copy );
        switch (copy->type)
        {
            case VM_MAP_COPY_KERNEL_BUFFER:
            {
                int rv;
                if (copy_size > copy->size)
                    copy_size = copy->size;

                unsigned long addr = vm_mmap(NULL, *dst_addr, copy_size, PROT_READ | PROT_WRITE,
                                             MAP_ANONYMOUS | MAP_PRIVATE, 0);

                if (IS_ERR((void*) addr))
                {
                    printk(KERN_ERR "Cannot vm_mmap %llu bytes\n", copy->size);
                    return KERN_NO_SPACE;
                }

                /*
                 * If we could do split_vma in a kernel module,
                 * we could use remap_vmalloc_range to avoid a copy. Sigh.

                struct vm_area_struct* vma;

                down_write (&linux_current->mm->mmap_sem);
                vma = find_vma(linux_current->mm, addr);
                if (vma == NULL)
                {
                    up_write (&linux_current->mm->mmap_sem);
                    vm_munmap(addr, copy->size);
                    return KERN_NO_SPACE;
                }
                */

                if (copy_to_user((void *) addr, copy->cpy_kdata, copy_size))
                {
                    printk(KERN_ERR "Cannot copy_to_user() to a newly allocated anon range!\n");
                    vm_munmap(addr, copy->size);
                    return KERN_FAILURE;
                }
                *dst_addr = addr;
				vfree (copy);

                return KERN_SUCCESS;
            }
            default:
                return KERN_FAILURE;
        }
}

boolean_t vm_map_copy_validate_size(vm_map_t dst_map, vm_map_copy_t copy, vm_map_size_t* size)
{
    if (copy == VM_MAP_COPY_NULL)
        return FALSE;
    switch (copy->type)
    {
        case VM_MAP_COPY_KERNEL_BUFFER:
            return *size == copy->size;
    }
    return FALSE;
}


int darling_is_task_64bit(void)
{
#if __x86_64__ || __arm64__
    return !test_thread_flag(TIF_IA32);
#else
    return 0;
#endif
}

// why is this in `osfmk/vm/vm_map.c`? it's declared in `osfmk/kern/debug.h`; you'd think it'd be in `osfmk/kern/debug.c`
unsigned int not_in_kdp = 1;
