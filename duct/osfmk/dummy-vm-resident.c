/*
 * Copyright (c) 2000-2009 Apple Inc. All rights reserved.
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
 *  File:   vm/vm_page.c
 *  Author: Avadis Tevanian, Jr., Michael Wayne Young
 *
 *  Resident memory management module.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <debug.h>
#include <libkern/OSAtomic.h>

#include <mach/clock_types.h>
#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <mach/sdt.h>
#include <kern/counters.h>
#include <kern/sched_prim.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/kalloc.h>
#include <kern/zalloc.h>
#include <kern/xpr.h>
#include <vm/pmap.h>
#include <vm/vm_init.h>
#include <vm/vm_map.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/vm_kern.h>         /* kernel_memory_allocate() */
#include <kern/misc_protos.h>
#include <zone_debug.h>
#include <vm/cpm.h>
#include <pexpert/pexpert.h>

#include <vm/vm_protos.h>
#include <vm/memory_object.h>
#include <vm/vm_purgeable_internal.h>

// #include <IOKit/IOHibernatePrivate.h>

#include <sys/kdebug.h>

#if defined(__arm__)
#include <arm/mp.h>
#endif

boolean_t   hibernate_cleaning_in_progress = FALSE;
boolean_t   vm_page_free_verify = TRUE;

uint32_t    vm_lopage_free_count = 0;
uint32_t    vm_lopage_free_limit = 0;
uint32_t    vm_lopage_lowater    = 0;
boolean_t   vm_lopage_refill = FALSE;
boolean_t   vm_lopage_needed = FALSE;

lck_mtx_ext_t   vm_page_queue_lock_ext;
lck_mtx_ext_t   vm_page_queue_free_lock_ext;
lck_mtx_ext_t   vm_purgeable_queue_lock_ext;

int     speculative_age_index = 0;
int     speculative_steal_index = 0;
struct vm_speculative_age_q vm_page_queue_speculative[VM_PAGE_MAX_SPECULATIVE_AGE_Q + 1];


/* __private_extern__ */ extern void        vm_page_init_lck_grp(void);

// static void     vm_page_free_prepare(vm_page_t  page);
// static vm_page_t    vm_page_grab_fictitious_common(ppnum_t phys_addr);




/*
 *  Associated with page of user-allocatable memory is a
 *  page structure.
 */

/*
 *  These variables record the values returned by vm_page_bootstrap,
 *  for debugging purposes.  The implementation of pmap_steal_memory
 *  and pmap_startup here also uses them internally.
 */

vm_offset_t virtual_space_start;
vm_offset_t virtual_space_end;
uint32_t    vm_page_pages;

/*
 *  The vm_page_lookup() routine, which provides for fast
 *  (virtual memory object, offset) to page lookup, employs
 *  the following hash table.  The vm_page_{insert,remove}
 *  routines install and remove associations in the table.
 *  [This table is often called the virtual-to-physical,
 *  or VP, table.]
 */
typedef struct {
    vm_page_t   pages;
#if MACH_PAGE_HASH_STATS
    int     cur_count;      /* current count */
    int     hi_count;       /* high water mark */
#endif /* MACH_PAGE_HASH_STATS */
} vm_page_bucket_t;


#define BUCKETS_PER_LOCK    16

vm_page_bucket_t *vm_page_buckets;      /* Array of buckets */
unsigned int    vm_page_bucket_count = 0;   /* How big is array? */
unsigned int    vm_page_hash_mask;      /* Mask for hash function */
unsigned int    vm_page_hash_shift;     /* Shift for hash function */
uint32_t    vm_page_bucket_hash;        /* Basic bucket hash */
unsigned int    vm_page_bucket_lock_count = 0;      /* How big is array of locks? */

lck_spin_t  *vm_page_bucket_locks;


#if MACH_PAGE_HASH_STATS
/* This routine is only for debug.  It is intended to be called by
 * hand by a developer using a kernel debugger.  This routine prints
 * out vm_page_hash table statistics to the kernel debug console.
 */
void
hash_debug(void)
{
        kprintf("not implemented: task_policy_set()\n");
        return 0;
}
#endif /* MACH_PAGE_HASH_STATS */

/*
 *  The virtual page size is currently implemented as a runtime
 *  variable, but is constant once initialized using vm_set_page_size.
 *  This initialization must be done in the machine-dependent
 *  bootstrap sequence, before calling other machine-independent
 *  initializations.
 *
 *  All references to the virtual page size outside this
 *  module must use the PAGE_SIZE, PAGE_MASK and PAGE_SHIFT
 *  constants.
 */
vm_size_t   page_size  = PAGE_SIZE;
vm_size_t   page_mask  = PAGE_MASK;
int     page_shift = PAGE_SHIFT;

/*
 *  Resident page structures are initialized from
 *  a template (see vm_page_alloc).
 *
 *  When adding a new field to the virtual memory
 *  object structure, be sure to add initialization
 *  (see vm_page_bootstrap).
 */
struct vm_page  vm_page_template;

vm_page_t   vm_pages = VM_PAGE_NULL;
unsigned int    vm_pages_count = 0;
ppnum_t     vm_page_lowest = 0;

/*
 *  Resident pages that represent real memory
 *  are allocated from a set of free lists,
 *  one per color.
 */
unsigned int    vm_colors;
unsigned int    vm_color_mask;          /* mask is == (vm_colors-1) */
unsigned int    vm_cache_geometry_colors = 0;   /* set by hw dependent code during startup */
queue_head_t    vm_page_queue_free[MAX_COLORS];
unsigned int    vm_page_free_wanted;
unsigned int    vm_page_free_wanted_privileged;
unsigned int    vm_page_free_count;
unsigned int    vm_page_fictitious_count;

unsigned int    vm_page_free_count_minimum; /* debugging */

/*
 *  Occasionally, the virtual memory system uses
 *  resident page structures that do not refer to
 *  real pages, for example to leave a page with
 *  important state information in the VP table.
 *
 *  These page structures are allocated the way
 *  most other kernel structures are.
 */
zone_t  vm_page_zone;
vm_locks_array_t vm_page_locks;
decl_lck_mtx_data(,vm_page_alloc_lock)
lck_mtx_ext_t vm_page_alloc_lock_ext;

unsigned int io_throttle_zero_fill;

unsigned int    vm_page_local_q_count = 0;
unsigned int    vm_page_local_q_soft_limit = 250;
unsigned int    vm_page_local_q_hard_limit = 500;
struct vplq     *vm_page_local_q = NULL;

/* N.B. Guard and fictitious pages must not
 * be assigned a zero phys_page value.
 */
/*
 *  Fictitious pages don't have a physical address,
 *  but we must initialize phys_page to something.
 *  For debugging, this should be a strange value
 *  that the pmap module can recognize in assertions.
 */
ppnum_t vm_page_fictitious_addr = (ppnum_t) -1;

/*
 *  Guard pages are not accessible so they don't
 *  need a physical address, but we need to enter
 *  one in the pmap.
 *  Let's make it recognizable and make sure that
 *  we don't use a real physical page with that
 *  physical address.
 */
ppnum_t vm_page_guard_addr = (ppnum_t) -2;

/*
 *  Resident page structures are also chained on
 *  queues that are used by the page replacement
 *  system (pageout daemon).  These queues are
 *  defined here, but are shared by the pageout
 *  module.  The inactive queue is broken into
 *  inactive and zf for convenience as the
 *  pageout daemon often assignes a higher
 *  affinity to zf pages
 */

vm_page_queue_head_t    vm_page_queue_active;
vm_page_queue_head_t    vm_page_queue_inactive;
vm_page_queue_head_t    vm_page_queue_anonymous;    /* inactive memory queue for anonymous pages */
vm_page_queue_head_t    vm_page_queue_throttled;

unsigned int    vm_page_active_count;
unsigned int    vm_page_inactive_count;
unsigned int    vm_page_anonymous_count;
unsigned int    vm_page_throttled_count;
unsigned int    vm_page_speculative_count;
unsigned int    vm_page_wire_count = 0;
unsigned int    vm_page_wire_count_initial;
unsigned int    vm_page_gobble_count = 0;
unsigned int    vm_page_wire_count_warning = 0;
unsigned int    vm_page_gobble_count_warning = 0;

unsigned int    vm_page_purgeable_count = 0; /* # of pages purgeable now */
unsigned int    vm_page_purgeable_wired_count = 0; /* # of purgeable pages that are wired now */
uint64_t    vm_page_purged_count = 0;    /* total count of purged pages */

#if DEVELOPMENT || DEBUG
unsigned int    vm_page_speculative_recreated = 0;
unsigned int    vm_page_speculative_created = 0;
unsigned int    vm_page_speculative_used = 0;
#endif

vm_page_queue_head_t    vm_page_queue_cleaned;

unsigned int    vm_page_cleaned_count = 0;
unsigned int    vm_pageout_enqueued_cleaned = 0;

uint64_t    max_valid_dma_address = 0xffffffffffffffffULL;
ppnum_t     max_valid_low_ppnum = 0xffffffff;


/*
 *  Several page replacement parameters are also
 *  shared with this module, so that page allocation
 *  (done here in vm_page_alloc) can trigger the
 *  pageout daemon.
 */
unsigned int    vm_page_free_target = 0;
unsigned int    vm_page_free_min = 0;
unsigned int    vm_page_throttle_limit = 0;
uint32_t    vm_page_creation_throttle = 0;
unsigned int    vm_page_inactive_target = 0;
unsigned int   vm_page_anonymous_min = 0;
unsigned int    vm_page_inactive_min = 0;
unsigned int    vm_page_free_reserved = 0;
unsigned int    vm_page_throttle_count = 0;


/*
 *  The VM system has a couple of heuristics for deciding
 *  that pages are "uninteresting" and should be placed
 *  on the inactive queue as likely candidates for replacement.
 *  These variables let the heuristics be controlled at run-time
 *  to make experimentation easier.
 */

boolean_t vm_page_deactivate_hint = TRUE;

struct vm_page_stats_reusable vm_page_stats_reusable;

/*
 *  vm_set_page_size:
 *
 *  Sets the page size, perhaps based upon the memory
 *  size.  Must be called before any use of page-size
 *  dependent functions.
 *
 *  Sets page_shift and page_mask from page_size.
 */
void
vm_set_page_size(void)
{
        kprintf("not implemented: vm_set_page_size()\n");
}


/* Called once during statup, once the cache geometry is known.
 */
// static void
// vm_page_set_colors( void )
// {
//     ;
// }


lck_grp_t       vm_page_lck_grp_free;
lck_grp_t       vm_page_lck_grp_queue;
lck_grp_t       vm_page_lck_grp_local;
lck_grp_t       vm_page_lck_grp_purge;
lck_grp_t       vm_page_lck_grp_alloc;
lck_grp_t       vm_page_lck_grp_bucket;
lck_grp_attr_t      vm_page_lck_grp_attr;
lck_attr_t      vm_page_lck_attr;


// /* __private_extern__ */ extern void
// vm_page_init_lck_grp(void)
// {
//     ;
// }

// void
// vm_page_init_local_q()
// {
//     ;
// }


/*
 *  vm_page_bootstrap:
 *
 *  Initializes the resident memory module.
 *
 *  Allocates memory for the page cells, and
 *  for the object/offset-to-page hash table headers.
 *  Each page cell is initialized and placed on the free list.
 *  Returns the range of available kernel virtual memory.
 */

// void
// vm_page_bootstrap(
//     vm_offset_t     *startp,
//     vm_offset_t     *endp)
// {
//     ;
// }

#ifndef MACHINE_PAGES
/*
 *  We implement pmap_steal_memory and pmap_startup with the help
 *  of two simpler functions, pmap_virtual_space and pmap_next_page.
 */

void *
pmap_steal_memory(
    vm_size_t size)
{
        kprintf("not implemented: pmap_steal_memory()\n");
        return 0;
}

void
pmap_startup(
    vm_offset_t *startp,
    vm_offset_t *endp)
{
        kprintf("not implemented: pmap_startup()\n");
}
#endif  /* MACHINE_PAGES */

/*
 *  Routine:    vm_page_module_init
 *  Purpose:
 *      Second initialization pass, to be done after
 *      the basic VM system is ready.
 */
// void
// vm_page_module_init(void)
// {
//     ;
// }

/*
 *  Routine:    vm_page_create
 *  Purpose:
 *      After the VM system is up, machine-dependent code
 *      may stumble across more physical memory.  For example,
 *      memory that it was reserving for a frame buffer.
 *      vm_page_create turns this memory into available pages.
 */

void
vm_page_create(
    ppnum_t start,
    ppnum_t end)
{
        kprintf("not implemented: vm_page_create()\n");
}

/*
 *  vm_page_hash:
 *
 *  Distributes the object/offset key pair among hash buckets.
 *
 *  NOTE:   The bucket count must be a power of 2
 */
#define vm_page_hash(object, offset) (\
    ( (natural_t)((uintptr_t)object * vm_page_bucket_hash) + ((uint32_t)atop_64(offset) ^ vm_page_bucket_hash))\
     & vm_page_hash_mask)


/*
 *  vm_page_insert:     [ internal use only ]
 *
 *  Inserts the given mem entry into the object/object-page
 *  table and object list.
 *
 *  The object must be locked.
 */
void
vm_page_insert(
    vm_page_t       mem,
    vm_object_t     object,
    vm_object_offset_t  offset)
{
        kprintf("not implemented: vm_page_insert()\n");
}

void		vm_page_insert_internal(
					vm_page_t		page,
					vm_object_t		object,
					vm_object_offset_t	offset,
					vm_tag_t                tag,
					boolean_t		queues_lock_held,
					boolean_t		insert_in_hash,
					boolean_t		batch_pmap_op,
					boolean_t               delayed_accounting,
					uint64_t		*delayed_ledger_update)
{
        kprintf("not implemented: vm_page_insert_internal()\n");
}

/*
 *  vm_page_replace:
 *
 *  Exactly like vm_page_insert, except that we first
 *  remove any existing page at the given offset in object.
 *
 *  The object must be locked.
 */
void
vm_page_replace(
    register vm_page_t      mem,
    register vm_object_t        object,
    register vm_object_offset_t offset)
{
        kprintf("not implemented: vm_page_replace()\n");
}

/*
 *  vm_page_remove:     [ internal use only ]
 *
 *  Removes the given mem entry from the object/offset-page
 *  table and the object page list.
 *
 *  The object must be locked.
 */

void
vm_page_remove(
    vm_page_t   mem,
    boolean_t   remove_from_hash)
{
        kprintf("not implemented: vm_page_remove()\n");
}


/*
 *  vm_page_lookup:
 *
 *  Returns the page associated with the object/offset
 *  pair specified; if none is found, VM_PAGE_NULL is returned.
 *
 *  The object must be locked.  No side effects.
 */

unsigned long vm_page_lookup_hint = 0;
unsigned long vm_page_lookup_hint_next = 0;
unsigned long vm_page_lookup_hint_prev = 0;
unsigned long vm_page_lookup_hint_miss = 0;
unsigned long vm_page_lookup_bucket_NULL = 0;
unsigned long vm_page_lookup_miss = 0;


vm_page_t
vm_page_lookup(
    vm_object_t     object,
    vm_object_offset_t  offset)
{
        kprintf("not implemented: vm_page_lookup()\n");
        return 0;
}


/*
 *  vm_page_rename:
 *
 *  Move the given memory entry from its
 *  current object to the specified target object/offset.
 *
 *  The object must be locked.
 */
void
vm_page_rename(
    register vm_page_t      mem,
    register vm_object_t        new_object,
    vm_object_offset_t      new_offset,
    boolean_t           encrypted_ok)
{
        kprintf("not implemented: vm_page_rename()\n");
}

/*
 *  vm_page_init:
 *
 *  Initialize the fields in a new page.
 *  This takes a structure with random values and initializes it
 *  so that it can be given to vm_page_release or vm_page_insert.
 */
// void
// vm_page_init(
//     vm_page_t   mem,
//     ppnum_t     phys_page,
//     boolean_t   lopage)
// {
//     ;
// }

/*
 *  vm_page_grab_fictitious:
 *
 *  Remove a fictitious page from the free list.
 *  Returns VM_PAGE_NULL if there are no free pages.
 */
int c_vm_page_grab_fictitious = 0;
int c_vm_page_grab_fictitious_failed = 0;
int c_vm_page_release_fictitious = 0;
int c_vm_page_more_fictitious = 0;

vm_page_t
vm_page_grab_fictitious_common(
    ppnum_t phys_addr)
{
        kprintf("not implemented: vm_page_grab_fictitious_common()\n");
        return 0;
}

vm_page_t
vm_page_grab_fictitious(void)
{
        kprintf("not implemented: vm_page_grab_fictitious()\n");
        return 0;
}

vm_page_t
vm_page_grab_guard(void)
{
        kprintf("not implemented: vm_page_grab_guard()\n");
        return 0;
}


/*
 *  vm_page_release_fictitious:
 *
 *  Release a fictitious page to the zone pool
 */
void
vm_page_release_fictitious(
    vm_page_t m)
{
        kprintf("not implemented: vm_page_release_fictitious()\n");
}

/*
 *  vm_page_more_fictitious:
 *
 *  Add more fictitious pages to the zone.
 *  Allowed to block. This routine is way intimate
 *  with the zones code, for several reasons:
 *  1. we need to carve some page structures out of physical
 *     memory before zones work, so they _cannot_ come from
 *     the zone_map.
 *  2. the zone needs to be collectable in order to prevent
 *     growth without bound. These structures are used by
 *     the device pager (by the hundreds and thousands), as
 *     private pages for pageout, and as blocking pages for
 *     pagein. Temporary bursts in demand should not result in
 *     permanent allocation of a resource.
 *  3. To smooth allocation humps, we allocate single pages
 *     with kernel_memory_allocate(), and cram them into the
 *     zone.
 */

void vm_page_more_fictitious(void)
{
        kprintf("not implemented: vm_page_more_fictitious()\n");
}


/*
 *  vm_pool_low():
 *
 *  Return true if it is not likely that a non-vm_privileged thread
 *  can get memory without blocking.  Advisory only, since the
 *  situation may change under us.
 */
int
vm_pool_low(void)
{
        kprintf("not implemented: vm_pool_low()\n");
        return 0;
}



/*
 * this is an interface to support bring-up of drivers
 * on platforms with physical memory > 4G...
 */
int     vm_himemory_mode = 0;


/*
 * this interface exists to support hardware controllers
 * incapable of generating DMAs with more than 32 bits
 * of address on platforms with physical memory > 4G...
 */
unsigned int    vm_lopages_allocated_q = 0;
unsigned int    vm_lopages_allocated_cpm_success = 0;
unsigned int    vm_lopages_allocated_cpm_failed = 0;
vm_page_queue_head_t    vm_lopage_queue_free;

vm_page_t
vm_page_grablo(void)
{
        kprintf("not implemented: vm_page_grablo()\n");
        return 0;
}


/*
 *  vm_page_grab:
 *
 *  first try to grab a page from the per-cpu free list...
 *  this must be done while pre-emption is disabled... if
 *  a page is available, we're done...
 *  if no page is available, grab the vm_page_queue_free_lock
 *  and see if current number of free pages would allow us
 *  to grab at least 1... if not, return VM_PAGE_NULL as before...
 *  if there are pages available, disable preemption and
 *  recheck the state of the per-cpu free list... we could
 *  have been preempted and moved to a different cpu, or
 *  some other thread could have re-filled it... if still
 *  empty, figure out how many pages we can steal from the
 *  global free queue and move to the per-cpu queue...
 *  return 1 of these pages when done... only wakeup the
 *  pageout_scan thread if we moved pages from the global
 *  list... no need for the wakeup if we've satisfied the
 *  request from the per-cpu queue.
 */

#define COLOR_GROUPS_TO_STEAL   4


vm_page_t
vm_page_grab( void )
{
        kprintf("not implemented: vm_page_grab()\n");
        return 0;
}

/*
 *  vm_page_release:
 *
 *  Return a page to the free list.
 */

extern void		vm_page_release(
	vm_page_t	page,
	boolean_t	page_queues_locked)
{
        kprintf("not implemented: vm_page_release()\n");
}

/*
 *  vm_page_wait:
 *
 *  Wait for a page to become available.
 *  If there are plenty of free pages, then we don't sleep.
 *
 *  Returns:
 *      TRUE:  There may be another page, try again
 *      FALSE: We were interrupted out of our wait, don't try again
 */

boolean_t
vm_page_wait(
    int interruptible )
{
        kprintf("not implemented: vm_page_wait()\n");
        return 0;
}

/*
 *  vm_page_alloc:
 *
 *  Allocate and return a memory cell associated
 *  with this VM object/offset pair.
 *
 *  Object must be locked.
 */

vm_page_t
vm_page_alloc(
    vm_object_t     object,
    vm_object_offset_t  offset)
{
        kprintf("not implemented: vm_page_alloc()\n");
        return 0;
}

vm_page_t
vm_page_alloclo(
    vm_object_t     object,
    vm_object_offset_t  offset)
{
        kprintf("not implemented: vm_page_alloclo()\n");
        return 0;
}


/*
 *  vm_page_alloc_guard:
 *
 *  Allocate a fictitious page which will be used
 *  as a guard page.  The page will be inserted into
 *  the object and returned to the caller.
 */

vm_page_t
vm_page_alloc_guard(
    vm_object_t     object,
    vm_object_offset_t  offset)
{
        kprintf("not implemented: vm_page_alloc_guard()\n");
        return 0;
}


counter(unsigned int c_laundry_pages_freed = 0;)

/*
 *  vm_page_free_prepare:
 *
 *  Removes page from any queue it may be on
 *  and disassociates it from its VM object.
 *
 *  Object and page queues must be locked prior to entry.
 */
// static void
// vm_page_free_prepare(
//     vm_page_t   mem)
// {
//     ;
// }


void
vm_page_free_prepare_queues(
    vm_page_t   mem)
{
        kprintf("not implemented: vm_page_free_prepare_queues()\n");

}


void
vm_page_free_prepare_object(
    vm_page_t   mem,
    boolean_t   remove_from_hash)
{
        kprintf("not implemented: vm_page_free_prepare_object()\n");
}


/*
 *  vm_page_free:
 *
 *  Returns the given page to the free list,
 *  disassociating it with any VM object.
 *
 *  Object and page queues must be locked prior to entry.
 */
void
vm_page_free(
    vm_page_t   mem)
{
        kprintf("not implemented: vm_page_free()\n");
}


void
vm_page_free_unlocked(
    vm_page_t   mem,
    boolean_t   remove_from_hash)
{
        kprintf("not implemented: vm_page_free_unlocked()\n");
}


/*
 * Free a list of pages.  The list can be up to several hundred pages,
 * as blocked up by vm_pageout_scan().
 * The big win is not having to take the free list lock once
 * per page.
 */
void
vm_page_free_list(
    vm_page_t   freeq,
    boolean_t   prepare_object)
{
        kprintf("not implemented: vm_page_free_list()\n");
}


/*
 *  vm_page_wire:
 *
 *  Mark this page as wired down by yet
 *  another map, removing it from paging queues
 *  as necessary.
 *
 *  The page's object and the page queues must be locked.
 */
void		vm_page_wire(
					vm_page_t	page,
					vm_tag_t        tag,
					boolean_t	check_memorystatus)
{
        kprintf("not implemented: vm_page_wire()\n");
}

/*
 *      vm_page_gobble:
 *
 *      Mark this page as consumed by the vm/ipc/xmm subsystems.
 *
 *      Called only for freshly vm_page_grab()ed pages - w/ nothing locked.
 */
void
vm_page_gobble(
        register vm_page_t      mem)
{
        kprintf("not implemented: vm_page_gobble()\n");
}

/*
 *  vm_page_unwire:
 *
 *  Release one wiring of this page, potentially
 *  enabling it to be paged again.
 *
 *  The page's object and the page queues must be locked.
 */
void
vm_page_unwire(
    vm_page_t   mem,
    boolean_t   queueit)
{
        kprintf("not implemented: vm_page_unwire()\n");
}

/*
 *  vm_page_deactivate:
 *
 *  Returns the given page to the inactive list,
 *  indicating that no physical maps have access
 *  to this page.  [Used by the physical mapping system.]
 *
 *  The page queues must be locked.
 */
void
vm_page_deactivate(
    vm_page_t   m)
{
        kprintf("not implemented: vm_page_deactivate()\n");
}


void
vm_page_deactivate_internal(
    vm_page_t   m,
    boolean_t   clear_hw_reference)
{
        kprintf("not implemented: vm_page_deactivate_internal()\n");
}

/*
 * vm_page_enqueue_cleaned
 *
 * Put the page on the cleaned queue, mark it cleaned, etc.
 * Being on the cleaned queue (and having m->clean_queue set)
 * does ** NOT ** guarantee that the page is clean!
 *
 * Call with the queues lock held.
 */

void vm_page_enqueue_cleaned(vm_page_t m)
{
        kprintf("not implemented: vm_page_enqueue_cleaned()\n");
}

/*
 *  vm_page_activate:
 *
 *  Put the specified page on the active list (if appropriate).
 *
 *  The page queues must be locked.
 */

void
vm_page_activate(
    register vm_page_t  m)
{
        kprintf("not implemented: vm_page_activate()\n");
}


/*
 *      vm_page_speculate:
 *
 *      Put the specified page on the speculative list (if appropriate).
 *
 *      The page queues must be locked.
 */
void
vm_page_speculate(
    vm_page_t   m,
    boolean_t   new)
{
        kprintf("not implemented: vm_page_speculate()\n");
}


/*
 * move pages from the specified aging bin to
 * the speculative bin that pageout_scan claims from
 *
 *      The page queues must be locked.
 */
void
vm_page_speculate_ageit(struct vm_speculative_age_q *aq)
{
        kprintf("not implemented: vm_page_speculate_ageit()\n");
}


void
vm_page_lru(
    vm_page_t   m)
{
        kprintf("not implemented: vm_page_lru()\n");
}


void
vm_page_reactivate_all_throttled(void)
{
        kprintf("not implemented: vm_page_reactivate_all_throttled()\n");
}


/*
 * move pages from the indicated local queue to the global active queue
 * its ok to fail if we're below the hard limit and force == FALSE
 * the nolocks == TRUE case is to allow this function to be run on
 * the hibernate path
 */

void
vm_page_reactivate_local(uint32_t lid, boolean_t force, boolean_t nolocks)
{
        kprintf("not implemented: vm_page_reactivate_local()\n");
}

/*
 *  vm_page_part_zero_fill:
 *
 *  Zero-fill a part of the page.
 */
void
vm_page_part_zero_fill(
    vm_page_t   m,
    vm_offset_t m_pa,
    vm_size_t   len)
{
        kprintf("not implemented: vm_page_part_zero_fill()\n");
}

/*
 *  vm_page_part_copy:
 *
 *  copy part of one page to another
 */

void
vm_page_part_copy(
    vm_page_t   src_m,
    vm_offset_t src_pa,
    vm_page_t   dst_m,
    vm_offset_t dst_pa,
    vm_size_t   len)
{
        kprintf("not implemented: vm_page_part_copy()\n");
}

/*
 *  vm_page_copy:
 *
 *  Copy one page to another
 *
 * ENCRYPTED SWAP:
 * The source page should not be encrypted.  The caller should
 * make sure the page is decrypted first, if necessary.
 */

int vm_page_copy_cs_validations = 0;
int vm_page_copy_cs_tainted = 0;

void
vm_page_copy(
    vm_page_t   src_m,
    vm_page_t   dest_m)
{
        kprintf("not implemented: vm_page_copy()\n");
}

#if MACH_ASSERT
// static void
// _vm_page_print(
//     vm_page_t   p)
// {
//     ;
// }

/*
 *  Check that the list of pages is ordered by
 *  ascending physical address and has no holes.
 */
// static int
// vm_page_verify_contiguous(
//     vm_page_t   pages,
//     unsigned int    npages)
// {
//     return 0;
// }


/*
 *  Check the free lists for proper length etc.
 */
// static unsigned int
// vm_page_verify_free_list(
//     queue_head_t    *vm_page_queue,
//     unsigned int    color,
//     vm_page_t   look_for_page,
//     boolean_t   expect_page)
// {
//     return 0;
// }

static boolean_t vm_page_verify_free_lists_enabled = FALSE;
// static void
// vm_page_verify_free_lists( void )
// {
//     ;
// }

void
vm_page_queues_assert(
    vm_page_t   mem,
    int     val)
{
        kprintf("not implemented: vm_page_queues_assert()\n");
}
#endif  /* MACH_ASSERT */


/*
 *  CONTIGUOUS PAGE ALLOCATION
 *
 *  Find a region large enough to contain at least n pages
 *  of contiguous physical memory.
 *
 *  This is done by traversing the vm_page_t array in a linear fashion
 *  we assume that the vm_page_t array has the avaiable physical pages in an
 *  ordered, ascending list... this is currently true of all our implementations
 *  and must remain so... there can be 'holes' in the array...  we also can
 *  no longer tolerate the vm_page_t's in the list being 'freed' and reclaimed
 *  which use to happen via 'vm_page_convert'... that function was no longer
 *  being called and was removed...
 *
 *  The basic flow consists of stabilizing some of the interesting state of
 *  a vm_page_t behind the vm_page_queue and vm_page_free locks... we start our
 *  sweep at the beginning of the array looking for pages that meet our criterea
 *  for a 'stealable' page... currently we are pretty conservative... if the page
 *  meets this criterea and is physically contiguous to the previous page in the 'run'
 *  we keep developing it.  If we hit a page that doesn't fit, we reset our state
 *  and start to develop a new run... if at this point we've already considered
 *  at least MAX_CONSIDERED_BEFORE_YIELD pages, we'll drop the 2 locks we hold,
 *  and mutex_pause (which will yield the processor), to keep the latency low w/r
 *  to other threads trying to acquire free pages (or move pages from q to q),
 *  and then continue from the spot we left off... we only make 1 pass through the
 *  array.  Once we have a 'run' that is long enough, we'll go into the loop which
 *  which steals the pages from the queues they're currently on... pages on the free
 *  queue can be stolen directly... pages that are on any of the other queues
 *  must be removed from the object they are tabled on... this requires taking the
 *  object lock... we do this as a 'try' to prevent deadlocks... if the 'try' fails
 *  or if the state of the page behind the vm_object lock is no longer viable, we'll
 *  dump the pages we've currently stolen back to the free list, and pick up our
 *  scan from the point where we aborted the 'current' run.
 *
 *
 *  Requirements:
 *      - neither vm_page_queue nor vm_free_list lock can be held on entry
 *
 *  Returns a pointer to a list of gobbled/wired pages or VM_PAGE_NULL.
 *
 * Algorithm:
 */

#define MAX_CONSIDERED_BEFORE_YIELD 1000


#define RESET_STATE_OF_RUN()    \
    MACRO_BEGIN     \
    prevcontaddr = -2;  \
    start_pnum = -1;    \
    free_considered = 0;    \
    substitute_needed = 0;  \
    npages = 0;     \
    MACRO_END

/*
 * Can we steal in-use (i.e. not free) pages when searching for
 * physically-contiguous pages ?
 */
#define VM_PAGE_FIND_CONTIGUOUS_CAN_STEAL 1

static unsigned int vm_page_find_contiguous_last_idx = 0,  vm_page_lomem_find_contiguous_last_idx = 0;
#if DEBUG
int vm_page_find_contig_debug = 0;
#endif

// static vm_page_t
// vm_page_find_contiguous(
//     unsigned int    contig_pages,
//     ppnum_t     max_pnum,
//     ppnum_t     pnum_mask,
//     boolean_t   wire,
//     int     flags)
// {
//     return 0;
// }

/*
 *  Allocate a list of contiguous, wired pages.
 */
kern_return_t
cpm_allocate(
    vm_size_t   size,
    vm_page_t   *list,
    ppnum_t     max_pnum,
    ppnum_t     pnum_mask,
    boolean_t   wire,
    int     flags)
{
        kprintf("not implemented: cpm_allocate()\n");
        return 0;
}


unsigned int vm_max_delayed_work_limit = DEFAULT_DELAYED_WORK_LIMIT;

/*
 * when working on a 'run' of pages, it is necessary to hold
 * the vm_page_queue_lock (a hot global lock) for certain operations
 * on the page... however, the majority of the work can be done
 * while merely holding the object lock... in fact there are certain
 * collections of pages that don't require any work brokered by the
 * vm_page_queue_lock... to mitigate the time spent behind the global
 * lock, go to a 2 pass algorithm... collect pages up to DELAYED_WORK_LIMIT
 * while doing all of the work that doesn't require the vm_page_queue_lock...
 * then call vm_page_do_delayed_work to acquire the vm_page_queue_lock and do the
 * necessary work for each page... we will grab the busy bit on the page
 * if it's not already held so that vm_page_do_delayed_work can drop the object lock
 * if it can't immediately take the vm_page_queue_lock in order to compete
 * for the locks in the same order that vm_pageout_scan takes them.
 * the operation names are modeled after the names of the routines that
 * need to be called in order to make the changes very obvious in the
 * original loop
 */

void vm_page_do_delayed_work(vm_object_t object, vm_tag_t tag, struct vm_page_delayed_work *dwp, int dw_count)
{
        kprintf("not implemented: vm_page_do_delayed_work()\n");
}

kern_return_t
vm_page_alloc_list(
    int page_count,
    int flags,
    vm_page_t *list)
{
        kprintf("not implemented: vm_page_alloc_list()\n");
        return 0;
}

void
vm_page_set_offset(vm_page_t page, vm_object_offset_t offset)
{
        kprintf("not implemented: vm_page_set_offset()\n");
}

vm_page_t
vm_page_get_next(vm_page_t page)
{
        kprintf("not implemented: vm_page_get_next()\n");
        return 0;
}

vm_object_offset_t
vm_page_get_offset(vm_page_t page)
{
        kprintf("not implemented: vm_page_get_offset()\n");
        return 0;
}

ppnum_t
vm_page_get_phys_page(vm_page_t page)
{
        kprintf("not implemented: vm_page_get_phys_page()\n");
        return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if HIBERNATION

static vm_page_t hibernate_gobble_queue;

extern boolean_t (* volatile consider_buffer_cache_collect)(int);

// static int  hibernate_drain_pageout_queue(struct vm_pageout_queue *);
// static int  hibernate_flush_dirty_pages(void);
// static int  hibernate_flush_queue(queue_head_t *, int);

void hibernate_flush_wait(void);
void hibernate_mark_in_progress(void);
void hibernate_clear_in_progress(void);


struct hibernate_statistics {
    int hibernate_considered;
    int hibernate_reentered_on_q;
    int hibernate_found_dirty;
    int hibernate_skipped_cleaning;
    int hibernate_skipped_transient;
    int hibernate_skipped_precious;
    int hibernate_queue_nolock;
    int hibernate_queue_paused;
    int hibernate_throttled;
    int hibernate_throttle_timeout;
    int hibernate_drained;
    int hibernate_drain_timeout;
    int cd_lock_failed;
    int cd_found_precious;
    int cd_found_wired;
    int cd_found_busy;
    int cd_found_unusual;
    int cd_found_cleaning;
    int cd_found_laundry;
    int cd_found_dirty;
    int cd_local_free;
    int cd_total_free;
    int cd_vm_page_wire_count;
    int cd_pages;
    int cd_discarded;
    int cd_count_wire;
} hibernate_stats;



// static int
// hibernate_drain_pageout_queue(struct vm_pageout_queue *q)
// {
//     return 0;
// }


// static int
// hibernate_flush_queue(queue_head_t *q, int qcount)
// {
//     return 0;
// }


// static int
// hibernate_flush_dirty_pages()
// {
//     return 0;
// }


extern void IOSleep(unsigned int);
extern int sync_internal(void);

int
hibernate_flush_memory()
{
        kprintf("not implemented: hibernate_flush_memory()\n");
        return 0;
}


// static void
// hibernate_page_list_zero(hibernate_page_list_t *list)
// {
//     ;
// }

void
hibernate_gobble_pages(uint32_t gobble_count, uint32_t free_page_time)
{
        kprintf("not implemented: hibernate_gobble_pages()\n");
}

void
hibernate_free_gobble_pages(void)
{
        kprintf("not implemented: hibernate_free_gobble_pages()\n");
}

// static boolean_t
// hibernate_consider_discard(vm_page_t m, boolean_t preflight)
// {
//     return 0;
// }


// static void
// hibernate_discard_page(vm_page_t m)
// {
//     ;
// }

/*
 Grab locks for hibernate_page_list_setall()
*/
void
hibernate_vm_lock_queues(void)
{
        kprintf("not implemented: hibernate_vm_lock_queues()\n");
}

void
hibernate_vm_unlock_queues(void)
{
        kprintf("not implemented: hibernate_vm_unlock_queues()\n");
}

/*
 Bits zero in the bitmaps => page needs to be saved. All pages default to be saved,
 pages known to VM to not need saving are subtracted.
 Wired pages to be saved are present in page_list_wired, pageable in page_list.
*/

// void
// hibernate_page_list_setall(hibernate_page_list_t * page_list,
//                hibernate_page_list_t * page_list_wired,
//                hibernate_page_list_t * page_list_pal,
//                boolean_t preflight,
//                uint32_t * pagesOut)
// {
//     ;
// }

// void
// hibernate_page_list_discard(hibernate_page_list_t * page_list)
// {
//     ;
// }

#endif /* HIBERNATION */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <mach_vm_debug.h>
#if MACH_VM_DEBUG

#include <mach_debug/hash_info.h>
#include <vm/vm_debug.h>

/*
 *  Routine:    vm_page_info
 *  Purpose:
 *      Return information about the global VP table.
 *      Fills the buffer with as much information as possible
 *      and returns the desired size of the buffer.
 *  Conditions:
 *      Nothing locked.  The caller should provide
 *      possibly-pageable memory.
 */

unsigned int
vm_page_info(
    hash_info_bucket_t *info,
    unsigned int count)
{
        kprintf("not implemented: vm_page_info()\n");
        return 0;
}
#endif  /* MACH_VM_DEBUG */
