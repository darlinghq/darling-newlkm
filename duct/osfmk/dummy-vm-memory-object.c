/*
 * Copyright (c) 2000-2008 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */
/*
 *  File:   vm/memory_object.c
 *  Author: Michael Wayne Young
 *
 *  External memory management interface control functions.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <advisory_pageout.h>

/*
 *  Interface dependencies:
 */

#include <mach/std_types.h> /* For pointer_t */
#include <mach/mach_types.h>

#include <mach/mig.h>
#include <mach/kern_return.h>
#include <mach/memory_object.h>
#include <mach/memory_object_default.h>
#include <mach/memory_object_control_server.h>
#include <mach/host_priv_server.h>
#include <mach/boolean.h>
#include <mach/vm_prot.h>
#include <mach/message.h>

/*
 *  Implementation dependencies:
 */
#include <string.h>     /* For memcpy() */

#include <kern/xpr.h>
#include <kern/host.h>
#include <kern/thread.h>    /* For current_thread() */
#include <kern/ipc_mig.h>
#include <kern/misc_protos.h>

#include <vm/vm_object.h>
#include <vm/vm_fault.h>
#include <vm/memory_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/pmap.h>        /* For pmap_clear_modify */
#include <vm/vm_kern.h>     /* For kernel_map, vm_move */
#include <vm/vm_map.h>      /* For vm_map_pageable */
#include <vm/vm_purgeable_internal.h>   /* Needed by some vm_page.h macros */
#include <vm/vm_shared_region.h>

#if MACH_PAGEMAP
#include <vm/vm_external.h>
#endif  /* MACH_PAGEMAP */

#include <vm/vm_protos.h>


memory_object_default_t memory_manager_default = MEMORY_OBJECT_DEFAULT_NULL;
decl_lck_mtx_data(, memory_manager_default_lock)


/*
 *  Routine:    memory_object_should_return_page
 *
 *  Description:
 *      Determine whether the given page should be returned,
 *      based on the page's state and on the given return policy.
 *
 *      We should return the page if one of the following is true:
 *
 *      1. Page is dirty and should_return is not RETURN_NONE.
 *      2. Page is precious and should_return is RETURN_ALL.
 *      3. Should_return is RETURN_ANYTHING.
 *
 *      As a side effect, m->dirty will be made consistent
 *      with pmap_is_modified(m), if should_return is not
 *      MEMORY_OBJECT_RETURN_NONE.
 */

#define memory_object_should_return_page(m, should_return) \
    (should_return != MEMORY_OBJECT_RETURN_NONE && \
     (((m)->dirty || ((m)->dirty = pmap_is_modified((m)->phys_page))) || \
      ((m)->precious && (should_return) == MEMORY_OBJECT_RETURN_ALL) || \
      (should_return) == MEMORY_OBJECT_RETURN_ANYTHING))

typedef int memory_object_lock_result_t;

#define MEMORY_OBJECT_LOCK_RESULT_DONE              0
#define MEMORY_OBJECT_LOCK_RESULT_MUST_BLOCK        1
#define MEMORY_OBJECT_LOCK_RESULT_MUST_RETURN       2
#define MEMORY_OBJECT_LOCK_RESULT_MUST_FREE     3

memory_object_lock_result_t memory_object_lock_page(
                vm_page_t       m,
                memory_object_return_t  should_return,
                boolean_t       should_flush,
                vm_prot_t       prot);

/*
 *  Routine:    memory_object_lock_page
 *
 *  Description:
 *      Perform the appropriate lock operations on the
 *      given page.  See the description of
 *      "memory_object_lock_request" for the meanings
 *      of the arguments.
 *
 *      Returns an indication that the operation
 *      completed, blocked, or that the page must
 *      be cleaned.
 */
memory_object_lock_result_t
memory_object_lock_page(
    vm_page_t       m,
    memory_object_return_t  should_return,
    boolean_t       should_flush,
    vm_prot_t       prot)
{
        kprintf("not implemented: memory_object_lock_page()\n");
        return 0;
}



/*
 *  Routine:    memory_object_lock_request [user interface]
 *
 *  Description:
 *      Control use of the data associated with the given
 *      memory object.  For each page in the given range,
 *      perform the following operations, in order:
 *          1)  restrict access to the page (disallow
 *              forms specified by "prot");
 *          2)  return data to the manager (if "should_return"
 *              is RETURN_DIRTY and the page is dirty, or
 *              "should_return" is RETURN_ALL and the page
 *              is either dirty or precious); and,
 *          3)  flush the cached copy (if "should_flush"
 *              is asserted).
 *      The set of pages is defined by a starting offset
 *      ("offset") and size ("size").  Only pages with the
 *      same page alignment as the starting offset are
 *      considered.
 *
 *      A single acknowledgement is sent (to the "reply_to"
 *      port) when these actions are complete.  If successful,
 *      the naked send right for reply_to is consumed.
 */

kern_return_t
memory_object_lock_request(
    memory_object_control_t     control,
    memory_object_offset_t      offset,
    memory_object_size_t        size,
    memory_object_offset_t  *   resid_offset,
    int         *   io_errno,
    memory_object_return_t      should_return,
    int             flags,
    vm_prot_t           prot)
{
        kprintf("not implemented: memory_object_lock_request()\n");
        return 0;
}

/*
 *  memory_object_release_name:  [interface]
 *
 *  Enforces name semantic on memory_object reference count decrement
 *  This routine should not be called unless the caller holds a name
 *  reference gained through the memory_object_named_create or the
 *  memory_object_rename call.
 *  If the TERMINATE_IDLE flag is set, the call will return if the
 *  reference count is not 1. i.e. idle with the only remaining reference
 *  being the name.
 *  If the decision is made to proceed the name field flag is set to
 *  false and the reference count is decremented.  If the RESPECT_CACHE
 *  flag is set and the reference count has gone to zero, the
 *  memory_object is checked to see if it is cacheable otherwise when
 *  the reference count is zero, it is simply terminated.
 */

kern_return_t
memory_object_release_name(
    memory_object_control_t control,
    int             flags)
{
        kprintf("not implemented: memory_object_release_name()\n");
        return 0;
}



/*
 *  Routine:    memory_object_destroy [user interface]
 *  Purpose:
 *      Shut down a memory object, despite the
 *      presence of address map (or other) references
 *      to the vm_object.
 */
kern_return_t
memory_object_destroy(
    memory_object_control_t control,
    kern_return_t       reason)
{
        kprintf("not implemented: memory_object_destroy()\n");
        return 0;
}

/*
 *  Routine:    vm_object_sync
 *
 *  Kernel internal function to synch out pages in a given
 *  range within an object to its memory manager.  Much the
 *  same as memory_object_lock_request but page protection
 *  is not changed.
 *
 *  If the should_flush and should_return flags are true pages
 *  are flushed, that is dirty & precious pages are written to
 *  the memory manager and then discarded.  If should_return
 *  is false, only precious pages are returned to the memory
 *  manager.
 *
 *  If should flush is false and should_return true, the memory
 *  manager's copy of the pages is updated.  If should_return
 *  is also false, only the precious pages are updated.  This
 *  last option is of limited utility.
 *
 *  Returns:
 *  FALSE       if no pages were returned to the pager
 *  TRUE        otherwise.
 */

boolean_t
vm_object_sync(
    vm_object_t     object,
    vm_object_offset_t  offset,
    vm_object_size_t    size,
    boolean_t       should_flush,
    boolean_t       should_return,
    boolean_t       should_iosync)
{
        kprintf("not implemented: vm_object_sync()\n");
        return 0;
}



#define LIST_REQ_PAGEOUT_PAGES(object, data_cnt, po, ro, ioerr, iosync)    \
MACRO_BEGIN                             \
                                    \
        int         upl_flags;                              \
    memory_object_t     pager;                  \
                                    \
    if (object == slide_info.slide_object) {                    \
        panic("Objects with slid pages not allowed\n");     \
    }                               \
                                            \
    if ((pager = (object)->pager) != MEMORY_OBJECT_NULL) {      \
        vm_object_paging_begin(object);             \
        vm_object_unlock(object);               \
                                    \
                if (iosync)                                         \
                        upl_flags = UPL_MSYNC | UPL_IOSYNC;         \
                else                                                \
                        upl_flags = UPL_MSYNC;                      \
                                            \
        (void) memory_object_data_return(pager,         \
            po,                     \
            (memory_object_cluster_size_t)data_cnt,     \
                    ro,                                             \
                    ioerr,                                          \
            FALSE,                      \
            FALSE,                                      \
            upl_flags);                                     \
                                    \
        vm_object_lock(object);                 \
        vm_object_paging_end(object);               \
    }                               \
MACRO_END



// static int
// vm_object_update_extent(
//         vm_object_t     object,
//         vm_object_offset_t  offset,
//     vm_object_offset_t  offset_end,
//     vm_object_offset_t  *offset_resid,
//     int         *io_errno,
//         boolean_t       should_flush,
//     memory_object_return_t  should_return,
//         boolean_t       should_iosync,
//         vm_prot_t       prot)
// {
//     return 0;
// }



/*
 *  Routine:    vm_object_update
 *  Description:
 *      Work function for m_o_lock_request(), vm_o_sync().
 *
 *      Called with object locked and paging ref taken.
 */
kern_return_t
vm_object_update(
    vm_object_t     object,
    vm_object_offset_t  offset,
    vm_object_size_t    size,
    vm_object_offset_t  *resid_offset,
    int         *io_errno,
    memory_object_return_t  should_return,
    int         flags,
    vm_prot_t       protection)
{
        kprintf("not implemented: vm_object_update()\n");
        return 0;
}


/*
 *  Routine:    memory_object_synchronize_completed [user interface]
 *
 *  Tell kernel that previously synchronized data
 *  (memory_object_synchronize) has been queue or placed on the
 *  backing storage.
 *
 *  Note: there may be multiple synchronize requests for a given
 *  memory object outstanding but they will not overlap.
 */

kern_return_t
memory_object_synchronize_completed(
    memory_object_control_t control,
    memory_object_offset_t  offset,
    memory_object_size_t    length)
{
        kprintf("not implemented: memory_object_synchronize_completed()\n");
        return 0;
}

// static kern_return_t
// vm_object_set_attributes_common(
//     vm_object_t object,
//     boolean_t   may_cache,
//     memory_object_copy_strategy_t copy_strategy,
//     boolean_t   temporary,
//         boolean_t   silent_overwrite,
//     boolean_t   advisory_pageout)
// {
//     return 0;
// }

/*
 *  Set the memory object attribute as provided.
 *
 *  XXX This routine cannot be completed until the vm_msync, clean
 *       in place, and cluster work is completed. See ifdef notyet
 *       below and note that vm_object_set_attributes_common()
 *       may have to be expanded.
 */
kern_return_t
memory_object_change_attributes(
    memory_object_control_t     control,
    memory_object_flavor_t      flavor,
    memory_object_info_t        attributes,
    mach_msg_type_number_t      count)
{
        kprintf("not implemented: memory_object_change_attributes()\n");
        return 0;
}

kern_return_t
memory_object_get_attributes(
        memory_object_control_t control,
        memory_object_flavor_t  flavor,
    memory_object_info_t    attributes, /* pointer to OUT array */
    mach_msg_type_number_t  *count)     /* IN/OUT */
{
        kprintf("not implemented: memory_object_get_attributes()\n");
        return 0;
}


kern_return_t
memory_object_iopl_request(
    ipc_port_t      port,
    memory_object_offset_t  offset,
    upl_size_t      *upl_size,
    upl_t           *upl_ptr,
    upl_page_info_array_t   user_page_list,
    unsigned int        *page_list_count,
    int         *flags)
{
        kprintf("not implemented: memory_object_iopl_request()\n");
        return 0;
}

/*
 *  Routine:    memory_object_upl_request [interface]
 *  Purpose:
 *      Cause the population of a portion of a vm_object.
 *      Depending on the nature of the request, the pages
 *      returned may be contain valid data or be uninitialized.
 *
 */

kern_return_t
memory_object_upl_request(
    memory_object_control_t control,
    memory_object_offset_t  offset,
    upl_size_t      size,
    upl_t           *upl_ptr,
    upl_page_info_array_t   user_page_list,
    unsigned int        *page_list_count,
    int         cntrl_flags)
{
        kprintf("not implemented: memory_object_upl_request()\n");
        return 0;
}

/*
 *  Routine:    memory_object_super_upl_request [interface]
 *  Purpose:
 *      Cause the population of a portion of a vm_object
 *      in much the same way as memory_object_upl_request.
 *      Depending on the nature of the request, the pages
 *      returned may be contain valid data or be uninitialized.
 *      However, the region may be expanded up to the super
 *      cluster size provided.
 */

kern_return_t
memory_object_super_upl_request(
    memory_object_control_t control,
    memory_object_offset_t  offset,
    upl_size_t      size,
    upl_size_t      super_cluster,
    upl_t           *upl,
    upl_page_info_t     *user_page_list,
    unsigned int        *page_list_count,
    int         cntrl_flags)
{
        kprintf("not implemented: memory_object_super_upl_request()\n");
        return 0;
}

kern_return_t
memory_object_cluster_size(memory_object_control_t control, memory_object_offset_t *start,
               vm_size_t *length, uint32_t *io_streaming, memory_object_fault_info_t fault_info)
{
        kprintf("not implemented: memory_object_cluster_size()\n");
        return 0;
}


int vm_stat_discard_cleared_reply = 0;
int vm_stat_discard_cleared_unset = 0;
int vm_stat_discard_cleared_too_late = 0;



/*
 *  Routine:    host_default_memory_manager [interface]
 *  Purpose:
 *      set/get the default memory manager port and default cluster
 *      size.
 *
 *      If successful, consumes the supplied naked send right.
 */
kern_return_t
host_default_memory_manager(
    host_priv_t     host_priv,
    memory_object_default_t *default_manager,
    __unused memory_object_cluster_size_t cluster_size)
{
        kprintf("not implemented: host_default_memory_manager()\n");
        return 0;
}

/*
 *  Routine:    memory_manager_default_reference
 *  Purpose:
 *      Returns a naked send right for the default
 *      memory manager.  The returned right is always
 *      valid (not IP_NULL or IP_DEAD).
 */

/* __private_extern__ */ extern memory_object_default_t
memory_manager_default_reference(void)
{
        kprintf("not implemented: memory_manager_default_reference()\n");
        return 0;
}

/*
 *  Routine:    memory_manager_default_check
 *
 *  Purpose:
 *      Check whether a default memory manager has been set
 *      up yet, or not. Returns KERN_SUCCESS if dmm exists,
 *      and KERN_FAILURE if dmm does not exist.
 *
 *      If there is no default memory manager, log an error,
 *      but only the first time.
 *
 */
/* __private_extern__ */ extern kern_return_t
memory_manager_default_check(void)
{
        kprintf("not implemented: memory_manager_default_check()\n");
        return 0;
}

/* __private_extern__ */ extern void
memory_manager_default_init(void)
{
        kprintf("not implemented: memory_manager_default_init()\n");
}



/* Allow manipulation of individual page state.  This is actually part of */
/* the UPL regimen but takes place on the object rather than on a UPL */

kern_return_t
memory_object_page_op(
    memory_object_control_t control,
    memory_object_offset_t  offset,
    int         ops,
    ppnum_t         *phys_entry,
    int         *flags)
{
        kprintf("not implemented: memory_object_page_op()\n");
        return 0;
}

/*
 * memory_object_range_op offers performance enhancement over
 * memory_object_page_op for page_op functions which do not require page
 * level state to be returned from the call.  Page_op was created to provide
 * a low-cost alternative to page manipulation via UPLs when only a single
 * page was involved.  The range_op call establishes the ability in the _op
 * family of functions to work on multiple pages where the lack of page level
 * state handling allows the caller to avoid the overhead of the upl structures.
 */

kern_return_t
memory_object_range_op(
    memory_object_control_t control,
    memory_object_offset_t  offset_beg,
    memory_object_offset_t  offset_end,
    int                     ops,
    int                     *range)
{
        kprintf("not implemented: memory_object_range_op()\n");
        return 0;
}


void
memory_object_mark_used(
        memory_object_control_t control)
{
        kprintf("not implemented: memory_object_mark_used()\n");
}


void
memory_object_mark_unused(
    memory_object_control_t control,
    __unused boolean_t  rage)
{
        kprintf("not implemented: memory_object_mark_unused()\n");
}


kern_return_t
memory_object_pages_resident(
    memory_object_control_t control,
    boolean_t           *   has_pages_resident)
{
        kprintf("not implemented: memory_object_pages_resident()\n");
        return 0;
}

kern_return_t
memory_object_signed(
    memory_object_control_t control,
    boolean_t       is_signed)
{
        kprintf("not implemented: memory_object_signed()\n");
        return 0;
}

boolean_t
memory_object_is_slid(
    memory_object_control_t control)
{
        kprintf("not implemented: memory_object_is_slid()\n");
        return 0;
}

static zone_t mem_obj_control_zone;

/* __private_extern__ */ extern void
memory_object_control_bootstrap(void)
{
        kprintf("not implemented: memory_object_control_bootstrap()\n");
}

/* __private_extern__ */ extern memory_object_control_t
memory_object_control_allocate(
    vm_object_t     object)
{
        kprintf("not implemented: memory_object_control_allocate()\n");
        return 0;
}

/* __private_extern__ */ extern void
memory_object_control_collapse(
    memory_object_control_t control,
    vm_object_t     object)
{
        kprintf("not implemented: memory_object_control_collapse()\n");
}

/* __private_extern__ */ extern vm_object_t
memory_object_control_to_vm_object(
    memory_object_control_t control)
{
        kprintf("not implemented: memory_object_control_to_vm_object()\n");
        return 0;
}

memory_object_control_t
convert_port_to_mo_control(
    __unused mach_port_t    port)
{
        kprintf("not implemented: convert_port_to_mo_control()\n");
        return 0;
}


mach_port_t
convert_mo_control_to_port(
    __unused memory_object_control_t    control)
{
        kprintf("not implemented: convert_mo_control_to_port()\n");
        return 0;
}

void
memory_object_control_reference(
    __unused memory_object_control_t    control)
{
        kprintf("not implemented: memory_object_control_reference()\n");
}

/*
 * We only every issue one of these references, so kill it
 * when that gets released (should switch the real reference
 * counting in true port-less EMMI).
 */
void
memory_object_control_deallocate(
    memory_object_control_t control)
{
        kprintf("not implemented: memory_object_control_deallocate()\n");
}

void
memory_object_control_disable(
    memory_object_control_t control)
{
        kprintf("not implemented: memory_object_control_disable()\n");
}

void
memory_object_default_reference(
    memory_object_default_t dmm)
{
        kprintf("not implemented: memory_object_default_reference()\n");
}

void
memory_object_default_deallocate(
    memory_object_default_t dmm)
{
        kprintf("not implemented: memory_object_default_deallocate()\n");
}

memory_object_t
convert_port_to_memory_object(
    __unused mach_port_t    port)
{
        kprintf("not implemented: convert_port_to_memory_object()\n");
        return 0;
}


mach_port_t
convert_memory_object_to_port(
    __unused memory_object_t    object)
{
        kprintf("not implemented: convert_memory_object_to_port()\n");
        return 0;
}


/* Routine memory_object_reference */
void memory_object_reference(
    memory_object_t memory_object)
{
        kprintf("not implemented: memory_object_reference()\n");
}

/* Routine memory_object_deallocate */
void memory_object_deallocate(
    memory_object_t memory_object)
{
        kprintf("not implemented: memory_object_deallocate()\n");
}


/* Routine memory_object_init */
kern_return_t memory_object_init
(
    memory_object_t memory_object,
    memory_object_control_t memory_control,
    memory_object_cluster_size_t memory_object_page_size
)
{
        kprintf("not implemented: memory_object_init()\n");
        return 0;
}

/* Routine memory_object_terminate */
kern_return_t memory_object_terminate
(
    memory_object_t memory_object
)
{
        kprintf("not implemented: memory_object_terminate()\n");
        return 0;
}

/* Routine memory_object_data_request */
kern_return_t memory_object_data_request
(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_cluster_size_t length,
    vm_prot_t desired_access,
    memory_object_fault_info_t fault_info
)
{
        kprintf("not implemented: memory_object_data_request()\n");
        return 0;
}

/* Routine memory_object_data_return */
kern_return_t memory_object_data_return
(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_cluster_size_t size,
    memory_object_offset_t *resid_offset,
    int *io_error,
    boolean_t dirty,
    boolean_t kernel_copy,
    int upl_flags
)
{
        kprintf("not implemented: memory_object_data_return()\n");
        return 0;
}

/* Routine memory_object_data_initialize */
kern_return_t memory_object_data_initialize
(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_cluster_size_t size
)
{
        kprintf("not implemented: memory_object_data_initialize()\n");
        return 0;
}

/* Routine memory_object_data_unlock */
kern_return_t memory_object_data_unlock
(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_size_t size,
    vm_prot_t desired_access
)
{
        kprintf("not implemented: memory_object_data_unlock()\n");
        return 0;
}

/* Routine memory_object_synchronize */
kern_return_t memory_object_synchronize
(
    memory_object_t memory_object,
    memory_object_offset_t offset,
    memory_object_size_t size,
    vm_sync_t sync_flags
)
{
        kprintf("not implemented: memory_object_synchronize()\n");
        return 0;
}


/*
 * memory_object_map() is called by VM (in vm_map_enter() and its variants)
 * each time a "named" VM object gets mapped directly or indirectly
 * (copy-on-write mapping).  A "named" VM object has an extra reference held
 * by the pager to keep it alive until the pager decides that the
 * memory object (and its VM object) can be reclaimed.
 * VM calls memory_object_last_unmap() (in vm_object_deallocate()) when all
 * the mappings of that memory object have been removed.
 *
 * For a given VM object, calls to memory_object_map() and memory_object_unmap()
 * are serialized (through object->mapping_in_progress), to ensure that the
 * pager gets a consistent view of the mapping status of the memory object.
 *
 * This allows the pager to keep track of how many times a memory object
 * has been mapped and with which protections, to decide when it can be
 * reclaimed.
 */

/* Routine memory_object_map */
kern_return_t memory_object_map
(
    memory_object_t memory_object,
    vm_prot_t prot
)
{
        kprintf("not implemented: memory_object_map()\n");
        return 0;
}

/* Routine memory_object_last_unmap */
kern_return_t memory_object_last_unmap
(
    memory_object_t memory_object
)
{
        kprintf("not implemented: memory_object_last_unmap()\n");
        return 0;
}

/* Routine memory_object_data_reclaim */
kern_return_t memory_object_data_reclaim
(
    memory_object_t memory_object,
    boolean_t   reclaim_backing_store
)
{
        kprintf("not implemented: memory_object_data_reclaim()\n");
        return 0;
}

/* Routine memory_object_create */
kern_return_t memory_object_create
(
    memory_object_default_t default_memory_manager,
    vm_size_t new_memory_object_size,
    memory_object_t *new_memory_object
)
{
        kprintf("not implemented: memory_object_create()\n");
        return 0;
}

upl_t
convert_port_to_upl(
    ipc_port_t  port)
{
        kprintf("not implemented: convert_port_to_upl()\n");
        return 0;
}

mach_port_t
convert_upl_to_port(
    __unused upl_t      upl)
{
        kprintf("not implemented: convert_upl_to_port()\n");
        return 0;
}

/* __private_extern__ */ extern void
upl_no_senders(
    __unused ipc_port_t             port,
    __unused mach_port_mscount_t    mscount)
{
        kprintf("not implemented: upl_no_senders()\n");
}
