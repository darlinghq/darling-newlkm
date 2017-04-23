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
#include "duct_pre_xnu.h"
#include "duct_vm_map.h"
#include "duct_kern_kalloc.h"

#include <vm/vm_map.h>

#undef kfree
extern void kfree (const void * objp);


static zone_t    vm_map_zone;        /* zone for vm_map structures */
// static zone_t    vm_map_entry_zone;    /* zone for vm_map_entry structures */
// static zone_t    vm_map_entry_reserved_zone;    /* zone with reserve for non-blocking
//                      * allocations */
static zone_t    vm_map_copy_zone;    /* zone for vm_map_copy structures */
extern vm_size_t msg_ool_size_small;

extern int duct_print_map (struct mm_struct * mm, unsigned long start, size_t len);

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
                 16 * 1024, PAGE_SIZE, "VM map copies");
    duct_zone_change (vm_map_copy_zone, Z_NOENCRYPT, TRUE);

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

vm_map_t duct_vm_map_create (struct mm_struct * linux_mm)
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
        // CPH, as linux will flush task_struct->mm in execve(), don't save linux_mm here
        // Reference: macho_load_binary() -> flush_old_exec(bprm) in bsd/compat/binfmt_macho.c
        // and flush_old_exec() and exec_mmap() in fs/exec.c
        // result->linux_mm   = linux_mm;
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


kern_return_t duct_vm_map_copyin_common ( vm_map_t src_map, vm_map_address_t src_addr, vm_map_size_t len,
                                          boolean_t src_destroy, boolean_t src_volatile,
                                          vm_map_copy_t * copy_result, boolean_t use_maxprot )
{
        printk ( KERN_NOTICE "- vm_map_copyin_common to current_map: %p from src_map: %p, addr: %p, len: 0x%llx destroy: %d\n",
                 current_map (), src_map, (void*) src_addr, len, src_destroy );

        // dump_stack ();

        vm_map_copy_t       copy;        /* Resulting copy */

        if (len == 0) {
                *copy_result    = VM_MAP_COPY_NULL;
                return KERN_SUCCESS;
        }

        // assert (src_map == current_map ());
        // assert (len > msg_ool_size_small);
        copy            = (vm_map_copy_t) duct_zalloc (vm_map_copy_zone);

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
                        copy->cpy_kdata = vmalloc_user(len);
                        copy->cpy_kalloc_size = len;

                        if (src_addr) {
                            if (copy_from_user(copy->cpy_kdata, (void __user *) src_addr, len)) {
                                    printk(KERN_ERR "Failed to copy memory\n");
                                    vfree (copy->cpy_kdata);
                                    duct_zfree (vm_map_copy_zone, copy);
                                    return KERN_NO_SPACE;
                            }
                        }

                        if (src_destroy && src_addr)
                            vm_munmap(src_addr, len);

                        printk(KERN_NOTICE "- copying OK\n");
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
        printk(KERN_NOTICE "vm_map_copy_discard\n");
        if (copy == VM_MAP_COPY_NULL)   return;

        switch (copy->type) {
                case VM_MAP_COPY_KERNEL_BUFFER:
                {
                    vfree(copy->cpy_kdata);
                    break;
                }
        }

        duct_zfree (vm_map_copy_zone, copy);
}


kern_return_t duct_vm_map_copyout (vm_map_t dst_map, vm_map_address_t * dst_addr, vm_map_copy_t copy)
{
        printk ( KERN_NOTICE "- vm_map_copyout (current_map: 0x%p) to dst_map: 0x%p, addr: 0x%p, copy: 0x%p\n",
                 current_map (), dst_map, (void*) dst_addr, copy );
        switch (copy->type)
        {
            case VM_MAP_COPY_KERNEL_BUFFER:
            {
                int rv;
                unsigned long addr = vm_mmap(NULL, *dst_addr, copy->size, PROT_READ | PROT_WRITE,
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

                if (copy_to_user((void *) addr, copy->cpy_kdata, copy->size))
                {
                    printk(KERN_ERR "Cannot copy_to_user() to a newly allocated anon range!\n");
                    vm_munmap(addr, copy->size);
                    return KERN_FAILURE;
                }
                *dst_addr = addr;

                return KERN_SUCCESS;
            }
            default:
                return KERN_FAILURE;
        }
}

int darling_is_task_64bit(void)
{
#if __x86_64__ || __arm64__
    return !test_thread_flag(TIF_IA32);
#else
    return 0;
#endif
}

