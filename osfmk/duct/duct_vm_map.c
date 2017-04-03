/*
Copyright (c) 2014-2017, Wenqi Chen

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
        printk ( KERN_NOTICE "- BUG: vm_map_copyin_common to current_map: 0x%p from src_map: 0x%p, addr: 0x%x, len: 0x%x destroy: %d\n",
                 current_map (), src_map, src_addr, len, src_destroy );

        // dump_stack ();

        vm_map_copy_t       copy;        /* Resulting copy */

        if (len == 0) {
                *copy_result    = VM_MAP_COPY_NULL;
                return KERN_SUCCESS;
        }

        // printk (KERN_NOTICE "- msg_ool_size_small: 0x%x\n", msg_ool_size_small);

        //
        // /*
        //  * If the copy is sufficiently small, use a kernel buffer instead
        //  * of making a virtual copy.  The theory being that the cost of
        //  * setting up VM (and taking C-O-W faults) dominates the copy costs
        //  * for small regions.
        //  */
        // if ((len < msg_ool_size_small) && !use_maxprot)
        //     return vm_map_copyin_kernel_buffer(src_map, src_addr, len,
        //                        src_destroy, copy_result);
        //

        assert (src_map == current_map ());
        assert (len > msg_ool_size_small);
        copy            = (vm_map_copy_t) duct_zalloc (vm_map_copy_zone);

        if (copy == NULL) {
                return KERN_RESOURCE_SHORTAGE;
        }

        copy->type      = VM_MAP_COPY_LINUX_VMA_LIST;
        // copy->map       = src_map;
        // copy->offset    = src_start;
        copy->size      = len;
        // printk ( KERN_NOTICE "- copy: 0x%p, sizeof vm_area_struct: %d\n",
        //          copy, sizeof(struct vm_area_struct) );

        switch (copy->type) {
            #if defined (__DARLING__)
            #else
                case VM_MAP_COPY_ENTRY_LIST:
                case VM_MAP_COPY_OBJECT:
                case VM_MAP_COPY_KERNEL_BUFFER:
                        // printk (KERN_NOTICE "- copy->type not support\n");
                        copy->cpy_kdata = duct_kalloc(len);

                        if (copy_from_user(copy->cpy_kdata, (void __user *) src_addr, len)) {
                                duct_kfree (copy->cpy_kdata);
                                duct_zfree (vm_map_copy_zone, copy);
                                return KERN_NO_SPACE;
                        }
                        return KERN_SUCCESS;
            #endif

                case VM_MAP_COPY_LINUX_VMA_LIST:
                {
                        // WC assert that we destroy source so that we don't
                        // need to write_protect pages now
                        assert (src_destroy == TRUE);

                        struct vm_area_struct     * vma     = NULL;
                        // CPH, cut to get vma (in our case, there is only one vma)
                        down_write (&linux_current->mm->mmap_sem);

                        cut_clean_vma (linux_current->mm, src_addr, len, &vma);

                        // up_write (&linux_current->mm->mmap_sem);

                        assert (vma->vm_file == NULL);
                        assert (vma->vm_start == src_addr);
                        assert (vma->vm_end == src_addr + len);

                        int              nr_pages    = vma_pages (vma);
                        struct page   ** pages       = duct_kalloc (sizeof(struct page *) * nr_pages);

                        // down_write (&linux_current->mm->mmap_sem);

                        int              nr_pages_ret   =
                        get_user_pages (NULL, linux_current->mm, vma->vm_start, nr_pages, 0, 0, pages, NULL);

                        assert (nr_pages_ret == nr_pages);
						copy->c_u.c_p.pages = pages;
						copy->c_u.c_p.page_count = nr_pages;

						atomic_set(&copy->c_u.c_p.mapping_refcount, 1);

                        // CPH, follow_page() doesn't work without get_user_pages aboe
                        // anon_vma_prepare (vma);

                        // int             nr              = 0;
                        // unsigned long   scan_addr       = src_addr;
                        // while (scan_addr < vma->vm_end) {
                        //         struct page       * page;
                        //         page    = follow_page (vma, scan_addr, FOLL_GET);
                        //
                        //         if (IS_ERR_OR_NULL (page)) {
                        //                 scan_addr  += LINUX_PAGE_SIZE;
                        //                 nr         ++;
                        //                 continue;
                        //         }
                        //
                        //         if (PageAnon (page) /* || page_trans_compound_anon (page) */) {
                        //                 // printk (KERN_NOTICE "- PageAnon\n");
                        //                 flush_anon_page (vma, page, scan_addr);
                        //                 flush_dcache_page (page);
                        //         }
                        //
                        //         if (nr < 20) {
                        //                 printk (KERN_NOTICE "- page[%d]: 0x%p\n", nr, page);
                        //         }
                        //
                        //         put_page (page);
                        //
                        //         scan_addr  += LINUX_PAGE_SIZE;
                        //         nr         ++;
                        // }

                        up_write (&linux_current->mm->mmap_sem);

                        // if (src_destroy) {
                        //         do_munmap(copy->map->linux_mm, src_start, len);
                        // }

                        printk (KERN_NOTICE "- returned copy: 0x%p\n", copy);
                        *copy_result    = copy;
                        return KERN_SUCCESS;
                }

        }

        // initialise list head
        // vm_map_copy_first_entry (copy)  = vm_map_copy_to_entry (copy);
        // vm_map_copy_last_entry (copy)   = vm_map_copy_to_entry (copy);
        //
        // copy->cpy_hdr.nentries          = 0;
        // copy->cpy_hdr.entries_pageable  = TRUE;

        // WC - todo
        // vm_map_store_init (&(copy->cpy_hdr));

        // down_write (&linux_mm->mmap_sem);
        // vma     = find_vma (mm, copy->offset);
        // assert (vma != 0);
        // assert (vma->vm_file == 0);

        // up_write (&linux_mm->mmap_sem);


        return 0;
}

void duct_vm_map_copy_discard (vm_map_copy_t copy)
{
        if (copy == VM_MAP_COPY_NULL)   return;

        switch (copy->type) {
                case VM_MAP_COPY_LINUX_VMA_LIST:
                {
                        // vm_size_t i;
                        // for (i = 0; i < copy->c_u.c_p.page_count; i++) {
                        //         put_page(copy->c_u.c_p.pages[i]);
                        // }
                        //
                        // duct_kfree (copy->c_u.c_p.pages);
                        // break;
                }

            #if defined (__DARLING__)
				// TODO: atomic_dec on mapping_refcount, free if zero
            #else
                // case VM_MAP_COPY_ENTRY_LIST:
                //         while (vm_map_copy_first_entry(copy) !=
                //                vm_map_copy_to_entry(copy)) {
                //                    vm_map_entry_t    entry = vm_map_copy_first_entry(copy);
                //
                //                    vm_map_copy_entry_unlink(copy, entry);
                //                    vm_object_deallocate(entry->object.vm_object);
                //                    vm_map_copy_entry_dispose(copy, entry);
                //         }
                //         break;

                // case VM_MAP_COPY_OBJECT:
                //         vm_object_deallocate(copy->cpy_object);
                //         break;
                //
                // case VM_MAP_COPY_KERNEL_BUFFER:
                //
                //         /*
                //          * The vm_map_copy_t and possibly the data buffer were
                //          * allocated by a single call to kalloc(), i.e. the
                //          * vm_map_copy_t was not allocated out of the zone.
                //          */
                //         kfree(copy, copy->cpy_kalloc_size);
                //         return;
            #endif
        }

        duct_zfree (vm_map_copy_zone, copy);
}

static int special_mapping_fault(struct vm_area_struct *vma,
                struct vm_fault *vmf)
{
    pgoff_t pgoff;
    struct page **pages;

    /*
     * special mappings have no vm_file, and in that case, the mm
     * uses vm_pgoff internally. So we have to subtract it from here.
     * We are allowed to do this because we are the mm; do not copy
     * this code into drivers!
     */
    pgoff = vmf->pgoff - vma->vm_pgoff;

    for (pages = vma->vm_private_data; pgoff && *pages; ++pages)
        pgoff--;

    if (*pages) {
        struct page *page = *pages;
        get_page(page);
        vmf->page = page;
        return 0;
    }

    return VM_FAULT_SIGBUS;
}

static void special_mapping_close(struct vm_area_struct *vma)
{
    printk (KERN_NOTICE "duct_vm_map / special_mapping_open()");
	vm_map_copy_t copy = (vm_map_copy_t) vma->vm_private_data;
	if (!atomic_dec_and_test(&copy->c_u.c_p.mapping_refcount))
	{
		// TODO: free pages
	}
}

static void special_mapping_open(struct vm_area_struct *vma)
{
    printk (KERN_NOTICE "duct_vm_map / special_mapping_open()");
	vm_map_copy_t copy = (vm_map_copy_t) vma->vm_private_data;
	atomic_inc(&copy->c_u.c_p.mapping_refcount);
}

static const struct vm_operations_struct special_mapping_vmops = {
	.open = special_mapping_open,
    .close = special_mapping_close,
    .fault = special_mapping_fault,
};


kern_return_t duct_vm_map_copyout (vm_map_t dst_map, vm_map_address_t * dst_addr, vm_map_copy_t copy)
{
        printk ( KERN_NOTICE "- BUG: vm_map_copyout (current_map: 0x%p) to dst_map: 0x%p, addr: 0x%p, copy: 0x%p\n",
                 current_map (), dst_map, dst_addr, copy );
		down_write (&linux_current->mm->mmap_sem);


        int ret;
        struct vm_area_struct *vma;
        struct mm_struct *mm = linux_current->mm;
        unsigned long addr = *dst_addr;
        struct page **pages = copy->c_u.c_p.pages;
        unsigned long len = copy->size;
        unsigned long vm_flags = VM_READ | VM_MAYWRITE;

        vma = kmem_cache_zalloc(vm_area_cachep, GFP_KERNEL);
        if (unlikely(vma == NULL))
            return KERN_RESOURCE_SHORTAGE;

        INIT_LIST_HEAD(&vma->anon_vma_chain);
        vma->vm_mm = mm;
        vma->vm_start = addr;
        vma->vm_end = addr + len;

        vma->vm_flags = vm_flags | mm->def_flags | VM_DONTEXPAND;
        vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);

        vma->vm_ops = &special_mapping_vmops;
        vma->vm_private_data = pages;

        ret = security_file_mmap(NULL, 0, 0, 0, vma->vm_start, 1);
        if (ret)
            goto out;

        ret = insert_vm_struct(mm, vma);
        if (ret)
            goto out;

        mm->total_vm += len >> PAGE_SHIFT;

        perf_event_mmap(vma);
		up_write (&linux_current->mm->mmap_sem);

        return 0;

out:
		up_write (&linux_current->mm->mmap_sem);
        kmem_cache_free(vm_area_cachep, vma);
        return ret;
}
