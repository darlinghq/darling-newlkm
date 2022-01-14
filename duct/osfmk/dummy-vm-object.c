/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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
 *  File:   vm/vm_object.c
 *  Author: Avadis Tevanian, Jr., Michael Wayne Young
 *
 *  Virtual memory object module.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <debug.h>
#include <mach_pagemap.h>
#include <task_swapper.h>

#include <mach/mach_types.h>
#include <mach/memory_object.h>
#include <mach/memory_object_default.h>
#include <mach/memory_object_control_server.h>
#include <mach/vm_param.h>

#include <mach/sdt.h>

#include <ipc/ipc_types.h>
#include <ipc/ipc_port.h>

#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/lock.h>
#include <kern/queue.h>
#include <kern/xpr.h>
#include <kern/kalloc.h>
#include <kern/zalloc.h>
#include <kern/host.h>
#include <kern/host_statistics.h>
#include <kern/processor.h>
#include <kern/misc_protos.h>

#include <vm/memory_object.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>
#include <vm/vm_purgeable_internal.h>

/*
 *  Virtual memory objects maintain the actual data
 *  associated with allocated virtual memory.  A given
 *  page of memory exists within exactly one object.
 *
 *  An object is only deallocated when all "references"
 *  are given up.
 *
 *  Associated with each object is a list of all resident
 *  memory pages belonging to that object; this list is
 *  maintained by the "vm_page" module, but locked by the object's
 *  lock.
 *
 *  Each object also records the memory object reference
 *  that is used by the kernel to request and write
 *  back data (the memory object, field "pager"), etc...
 *
 *  Virtual memory objects are allocated to provide
 *  zero-filled memory (vm_allocate) or map a user-defined
 *  memory object into a virtual address space (vm_map).
 *
 *  Virtual memory objects that refer to a user-defined
 *  memory object are called "permanent", because all changes
 *  made in virtual memory are reflected back to the
 *  memory manager, which may then store it permanently.
 *  Other virtual memory objects are called "temporary",
 *  meaning that changes need be written back only when
 *  necessary to reclaim pages, and that storage associated
 *  with the object can be discarded once it is no longer
 *  mapped.
 *
 *  A permanent memory object may be mapped into more
 *  than one virtual address space.  Moreover, two threads
 *  may attempt to make the first mapping of a memory
 *  object concurrently.  Only one thread is allowed to
 *  complete this mapping; all others wait for the
 *  "pager_initialized" field is asserted, indicating
 *  that the first thread has initialized all of the
 *  necessary fields in the virtual memory object structure.
 *
 *  The kernel relies on a *default memory manager* to
 *  provide backing storage for the zero-filled virtual
 *  memory objects.  The pager memory objects associated
 *  with these temporary virtual memory objects are only
 *  requested from the default memory manager when it
 *  becomes necessary.  Virtual memory objects
 *  that depend on the default memory manager are called
 *  "internal".  The "pager_created" field is provided to
 *  indicate whether these ports have ever been allocated.
 *
 *  The kernel may also create virtual memory objects to
 *  hold changed pages after a copy-on-write operation.
 *  In this case, the virtual memory object (and its
 *  backing storage -- its memory object) only contain
 *  those pages that have been changed.  The "shadow"
 *  field refers to the virtual memory object that contains
 *  the remainder of the contents.  The "shadow_offset"
 *  field indicates where in the "shadow" these contents begin.
 *  The "copy" field refers to a virtual memory object
 *  to which changed pages must be copied before changing
 *  this object, in order to implement another form
 *  of copy-on-write optimization.
 *
 *  The virtual memory object structure also records
 *  the attributes associated with its memory object.
 *  The "pager_ready", "can_persist" and "copy_strategy"
 *  fields represent those attributes.  The "cached_list"
 *  field is used in the implementation of the persistence
 *  attribute.
 *
 * ZZZ Continue this comment.
 */

/* Forward declarations for internal functions. */
// static kern_return_t    vm_object_terminate(
//                 vm_object_t object);

extern void     vm_object_remove(
                vm_object_t object);

// static kern_return_t    vm_object_copy_call(
//                 vm_object_t     src_object,
//                 vm_object_offset_t  src_offset,
//                 vm_object_size_t    size,
//                 vm_object_t     *_result_object);

// static void     vm_object_do_collapse(
//                 vm_object_t object,
//                 vm_object_t backing_object);

// static void     vm_object_do_bypass(
//                 vm_object_t object,
//                 vm_object_t backing_object);

// static void     vm_object_release_pager(
//                             memory_object_t pager,
//                 boolean_t   hashed);

static zone_t       vm_object_zone;     /* vm backing store zone */

/*
 *  All wired-down kernel memory belongs to a single virtual
 *  memory object (kernel_object) to avoid wasting data structures.
 */
static struct vm_object         kernel_object_store;
vm_object_t                     kernel_object;


/*
 *  The submap object is used as a placeholder for vm_map_submap
 *  operations.  The object is declared in vm_map.c because it
 *  is exported by the vm_map module.  The storage is declared
 *  here because it must be initialized here.
 */
static struct vm_object         vm_submap_object_store;

/*
 *  Virtual memory objects are initialized from
 *  a template (see vm_object_allocate).
 *
 *  When adding a new field to the virtual memory
 *  object structure, be sure to add initialization
 *  (see _vm_object_allocate()).
 */
static struct vm_object         vm_object_template;

unsigned int vm_page_purged_wired = 0;
unsigned int vm_page_purged_busy = 0;
unsigned int vm_page_purged_others = 0;

#if VM_OBJECT_CACHE
/*
 *  Virtual memory objects that are not referenced by
 *  any address maps, but that are allowed to persist
 *  (an attribute specified by the associated memory manager),
 *  are kept in a queue (vm_object_cached_list).
 *
 *  When an object from this queue is referenced again,
 *  for example to make another address space mapping,
 *  it must be removed from the queue.  That is, the
 *  queue contains *only* objects with zero references.
 *
 *  The kernel may choose to terminate objects from this
 *  queue in order to reclaim storage.  The current policy
 *  is to permit a fixed maximum number of unreferenced
 *  objects (vm_object_cached_max).
 *
 *  A spin lock (accessed by routines
 *  vm_object_cache_{lock,lock_try,unlock}) governs the
 *  object cache.  It must be held when objects are
 *  added to or removed from the cache (in vm_object_terminate).
 *  The routines that acquire a reference to a virtual
 *  memory object based on one of the memory object ports
 *  must also lock the cache.
 *
 *  Ideally, the object cache should be more isolated
 *  from the reference mechanism, so that the lock need
 *  not be held to make simple references.
 */
static vm_object_t  vm_object_cache_trim(
                boolean_t called_from_vm_object_deallocate);

static void     vm_object_deactivate_all_pages(
                vm_object_t object);

static int      vm_object_cached_high;  /* highest # cached objects */
static int      vm_object_cached_max = 512; /* may be patched*/

#define vm_object_cache_lock()      \
        lck_mtx_lock(&vm_object_cached_lock_data)
#define vm_object_cache_lock_try()      \
        lck_mtx_try_lock(&vm_object_cached_lock_data)

#endif  /* VM_OBJECT_CACHE */

static queue_head_t vm_object_cached_list;
static uint32_t     vm_object_cache_pages_freed = 0;
static uint32_t     vm_object_cache_pages_moved = 0;
static uint32_t     vm_object_cache_pages_skipped = 0;
static uint32_t     vm_object_cache_adds = 0;
static uint32_t     vm_object_cached_count = 0;
static lck_mtx_t    vm_object_cached_lock_data;
static lck_mtx_ext_t    vm_object_cached_lock_data_ext;

static uint32_t     vm_object_page_grab_failed = 0;
static uint32_t     vm_object_page_grab_skipped = 0;
static uint32_t     vm_object_page_grab_returned = 0;
static uint32_t     vm_object_page_grab_pmapped = 0;
static uint32_t     vm_object_page_grab_reactivations = 0;

#define vm_object_cache_lock_spin()     \
        lck_mtx_lock_spin(&vm_object_cached_lock_data)
#define vm_object_cache_unlock()    \
        lck_mtx_unlock(&vm_object_cached_lock_data)

// static void vm_object_cache_remove_locked(vm_object_t);


#define VM_OBJECT_HASH_COUNT        1024
#define VM_OBJECT_HASH_LOCK_COUNT   512

static lck_mtx_t    vm_object_hashed_lock_data[VM_OBJECT_HASH_LOCK_COUNT];
static lck_mtx_ext_t    vm_object_hashed_lock_data_ext[VM_OBJECT_HASH_LOCK_COUNT];

static queue_head_t vm_object_hashtable[VM_OBJECT_HASH_COUNT];
static struct zone  *vm_object_hash_zone;

struct vm_object_hash_entry {
    queue_chain_t       hash_link;  /* hash chain link */
    memory_object_t pager;      /* pager we represent */
    vm_object_t     object;     /* corresponding object */
    boolean_t       waiting;    /* someone waiting for
                         * termination */
};

typedef struct vm_object_hash_entry *vm_object_hash_entry_t;
#define VM_OBJECT_HASH_ENTRY_NULL   ((vm_object_hash_entry_t) 0)

#define VM_OBJECT_HASH_SHIFT    5
#define vm_object_hash(pager) \
    ((int)((((uintptr_t)pager) >> VM_OBJECT_HASH_SHIFT) % VM_OBJECT_HASH_COUNT))

#define vm_object_lock_hash(pager) \
    ((int)((((uintptr_t)pager) >> VM_OBJECT_HASH_SHIFT) % VM_OBJECT_HASH_LOCK_COUNT))

void vm_object_hash_entry_free(
    vm_object_hash_entry_t  entry);

// static void vm_object_reap(vm_object_t object);
// static void vm_object_reap_async(vm_object_t object);
// static void vm_object_reaper_thread(void);

static lck_mtx_t    vm_object_reaper_lock_data;
static lck_mtx_ext_t    vm_object_reaper_lock_data_ext;

static queue_head_t vm_object_reaper_queue; /* protected by vm_object_reaper_lock() */
unsigned int vm_object_reap_count = 0;
unsigned int vm_object_reap_count_async = 0;

#define vm_object_reaper_lock()     \
        lck_mtx_lock(&vm_object_reaper_lock_data)
#define vm_object_reaper_lock_spin()        \
        lck_mtx_lock_spin(&vm_object_reaper_lock_data)
#define vm_object_reaper_unlock()   \
        lck_mtx_unlock(&vm_object_reaper_lock_data)

#if 0
#undef KERNEL_DEBUG
#define KERNEL_DEBUG KERNEL_DEBUG_CONSTANT
#endif


// static lck_mtx_t *
// vm_object_hash_lock_spin(
//     memory_object_t pager)
// {
//     return 0;
// }

// static void
// vm_object_hash_unlock(lck_mtx_t *lck)
// {
//     ;
// }


/*
 *  vm_object_hash_lookup looks up a pager in the hashtable
 *  and returns the corresponding entry, with optional removal.
 */
// static vm_object_hash_entry_t
// vm_object_hash_lookup(
//     memory_object_t pager,
//     boolean_t   remove_entry)
// {
//     return 0;
// }

/*
 *  vm_object_hash_enter enters the specified
 *  pager / cache object association in the hashtable.
 */

// static void
// vm_object_hash_insert(
//     vm_object_hash_entry_t  entry,
//     vm_object_t     object)
// {
//     ;
// }

// static vm_object_hash_entry_t
// vm_object_hash_entry_alloc(
//     memory_object_t pager)
// {
//     return 0;
// }

void
vm_object_hash_entry_free(
    vm_object_hash_entry_t  entry)
{
        kprintf("not implemented: vm_object_hash_entry_free()\n");
}

/*
 *  vm_object_allocate:
 *
 *  Returns a new object with the given size.
 */

/* __private_extern__ */ extern void
_vm_object_allocate(
    vm_object_size_t    size,
    vm_object_t     object)
{
        kprintf("not implemented: _vm_object_allocate()\n");
}

/* __private_extern__ */ extern vm_object_t
vm_object_allocate(
    vm_object_size_t    size)
{
        kprintf("not implemented: vm_object_allocate()\n");
        return 0;
}


lck_grp_t       vm_object_lck_grp;
lck_grp_t       vm_object_cache_lck_grp;
lck_grp_attr_t      vm_object_lck_grp_attr;
lck_attr_t      vm_object_lck_attr;
lck_attr_t      kernel_object_lck_attr;

/*
 *  vm_object_bootstrap:
 *
 *  Initialize the VM objects module.
 */
/* __private_extern__ */ extern void
vm_object_bootstrap(void)
{
        kprintf("not implemented: vm_object_bootstrap()\n");
}

void
vm_object_reaper_init(void)
{
        kprintf("not implemented: vm_object_reaper_init()\n");
}

/* __private_extern__ */ extern void
vm_object_init(void)
{
        kprintf("not implemented: vm_object_init()\n");
}


/* __private_extern__ */ extern void
vm_object_init_lck_grp(void)
{
        kprintf("not implemented: vm_object_init_lck_grp()\n");
}

#if VM_OBJECT_CACHE
#define MIGHT_NOT_CACHE_SHADOWS     1
#if MIGHT_NOT_CACHE_SHADOWS
static int cache_shadows = TRUE;
#endif  /* MIGHT_NOT_CACHE_SHADOWS */
#endif

/*
 *  vm_object_deallocate:
 *
 *  Release a reference to the specified object,
 *  gained either through a vm_object_allocate
 *  or a vm_object_reference call.  When all references
 *  are gone, storage associated with this object
 *  may be relinquished.
 *
 *  No object may be locked.
 */
unsigned long vm_object_deallocate_shared_successes = 0;
unsigned long vm_object_deallocate_shared_failures = 0;
unsigned long vm_object_deallocate_shared_swap_failures = 0;
/* __private_extern__ */ extern void
vm_object_deallocate(
    register vm_object_t    object)
{
        kprintf("not implemented: vm_object_deallocate()\n");
}



vm_page_t
vm_object_page_grab(
    vm_object_t object)
{
        kprintf("not implemented: vm_object_page_grab()\n");
        return 0;
}



#define EVICT_PREPARE_LIMIT 64
#define EVICT_AGE       10

static  clock_sec_t vm_object_cache_aging_ts = 0;

// static void
// vm_object_cache_remove_locked(
//     vm_object_t object)
// {
//     ;
// }

void
vm_object_cache_remove(
    vm_object_t object)
{
        kprintf("not implemented: vm_object_cache_remove()\n");
}

void
vm_object_cache_add(
    vm_object_t object)
{
        kprintf("not implemented: vm_object_cache_add()\n");
}

int
vm_object_cache_evict(
    int num_to_evict,
    int max_objects_to_examine)
{
        kprintf("not implemented: vm_object_cache_evict()\n");
        return 0;
}


#if VM_OBJECT_CACHE
/*
 *  Check to see whether we really need to trim
 *  down the cache. If so, remove an object from
 *  the cache, terminate it, and repeat.
 *
 *  Called with, and returns with, cache lock unlocked.
 */
vm_object_t
vm_object_cache_trim(
    boolean_t called_from_vm_object_deallocate)
{
        kprintf("not implemented: vm_object_cache_trim()\n");
}
#endif


/*
 *  Routine:    vm_object_terminate
 *  Purpose:
 *      Free all resources associated with a vm_object.
 *  In/out conditions:
 *      Upon entry, the object must be locked,
 *      and the object must have exactly one reference.
 *
 *      The shadow object reference is left alone.
 *
 *      The object must be unlocked if its found that pages
 *      must be flushed to a backing object.  If someone
 *      manages to map the object while it is being flushed
 *      the object is returned unlocked and unchanged.  Otherwise,
 *      upon exit, the cache will be unlocked, and the
 *      object will cease to exist.
 */
// static kern_return_t
// vm_object_terminate(
//     vm_object_t object)
// {
//     return 0;
// }


/*
 * vm_object_reap():
 *
 * Complete the termination of a VM object after it's been marked
 * as "terminating" and "!alive" by vm_object_terminate().
 *
 * The VM object must be locked by caller.
 * The lock will be released on return and the VM object is no longer valid.
 */
void
vm_object_reap(
    vm_object_t object)
{
        kprintf("not implemented: vm_object_reap()\n");
}


unsigned int vm_max_batch = 256;

#define V_O_R_MAX_BATCH 128

#define BATCH_LIMIT(max)    (vm_max_batch >= max ? max : vm_max_batch)


#define VM_OBJ_REAP_FREELIST(_local_free_q, do_disconnect)      \
    MACRO_BEGIN                         \
    if (_local_free_q) {                        \
        if (do_disconnect) {                    \
            vm_page_t m;                    \
            for (m = _local_free_q;             \
                 m != VM_PAGE_NULL;             \
                 m = (vm_page_t) m->pageq.next) {       \
                if (m->pmapped) {           \
                    pmap_disconnect(m->phys_page);  \
                }                   \
            }                       \
        }                           \
        vm_page_free_list(_local_free_q, TRUE);         \
        _local_free_q = VM_PAGE_NULL;               \
    }                               \
    MACRO_END


void
vm_object_reap_pages(
    vm_object_t     object,
    int     reap_type)
{
        kprintf("not implemented: vm_object_reap_pages()\n");
}


void
vm_object_reap_async(
    vm_object_t object)
{
        kprintf("not implemented: vm_object_reap_async()\n");
}


void
vm_object_reaper_thread(void)
{
        kprintf("not implemented: vm_object_reaper_thread()\n");
}

/*
 *  Routine:    vm_object_pager_wakeup
 *  Purpose:    Wake up anyone waiting for termination of a pager.
 */

// static void
// vm_object_pager_wakeup(
//     memory_object_t pager)
// {
//     ;
// }

/*
 *  Routine:    vm_object_release_pager
 *  Purpose:    Terminate the pager and, upon completion,
 *          release our last reference to it.
 *          just like memory_object_terminate, except
 *          that we wake up anyone blocked in vm_object_enter
 *          waiting for termination message to be queued
 *          before calling memory_object_init.
 */
// static void
// vm_object_release_pager(
//     memory_object_t pager,
//     boolean_t   hashed)
// {
//     ;
// }

/*
 *  Routine:    vm_object_destroy
 *  Purpose:
 *      Shut down a VM object, despite the
 *      presence of address map (or other) references
 *      to the vm_object.
 */
kern_return_t
vm_object_destroy(
    vm_object_t     object,
    __unused kern_return_t      reason)
{
        kprintf("not implemented: vm_object_destroy()\n");
        return 0;
}


#if VM_OBJECT_CACHE

#define VM_OBJ_DEACT_ALL_STATS DEBUG
#if VM_OBJ_DEACT_ALL_STATS
uint32_t vm_object_deactivate_all_pages_batches = 0;
uint32_t vm_object_deactivate_all_pages_pages = 0;
#endif /* VM_OBJ_DEACT_ALL_STATS */
/*
 *  vm_object_deactivate_all_pages
 *
 *  Deactivate all pages in the specified object.  (Keep its pages
 *  in memory even though it is no longer referenced.)
 *
 *  The object must be locked.
 */
// static void
// vm_object_deactivate_all_pages(
//     register vm_object_t    object)
// {
//     ;
// }
#endif  /* VM_OBJECT_CACHE */



/*
 * The "chunk" macros are used by routines below when looking for pages to deactivate.  These
 * exist because of the need to handle shadow chains.  When deactivating pages, we only
 * want to deactive the ones at the top most level in the object chain.  In order to do
 * this efficiently, the specified address range is divided up into "chunks" and we use
 * a bit map to keep track of which pages have already been processed as we descend down
 * the shadow chain.  These chunk macros hide the details of the bit map implementation
 * as much as we can.
 *
 * For convenience, we use a 64-bit data type as the bit map, and therefore a chunk is
 * set to 64 pages.  The bit map is indexed from the low-order end, so that the lowest
 * order bit represents page 0 in the current range and highest order bit represents
 * page 63.
 *
 * For further convenience, we also use negative logic for the page state in the bit map.
 * The bit is set to 1 to indicate it has not yet been seen, and to 0 to indicate it has
 * been processed.  This way we can simply test the 64-bit long word to see if it's zero
 * to easily tell if the whole range has been processed.  Therefore, the bit map starts
 * out with all the bits set.  The macros below hide all these details from the caller.
 */

#define PAGES_IN_A_CHUNK    64  /* The number of pages in the chunk must */
                    /* be the same as the number of bits in  */
                    /* the chunk_state_t type. We use 64     */
                    /* just for convenience.         */

#define CHUNK_SIZE  (PAGES_IN_A_CHUNK * PAGE_SIZE_64)   /* Size of a chunk in bytes */

typedef uint64_t    chunk_state_t;

/*
 * The bit map uses negative logic, so we start out with all 64 bits set to indicate
 * that no pages have been processed yet.  Also, if len is less than the full CHUNK_SIZE,
 * then we mark pages beyond the len as having been "processed" so that we don't waste time
 * looking at pages in that range.  This can save us from unnecessarily chasing down the
 * shadow chain.
 */

#define CHUNK_INIT(c, len)                      \
    MACRO_BEGIN                         \
    uint64_t p;                         \
                                    \
    (c) = 0xffffffffffffffffLL;                     \
                                    \
    for (p = (len) / PAGE_SIZE_64; p < PAGES_IN_A_CHUNK; p++)   \
        MARK_PAGE_HANDLED(c, p);                \
    MACRO_END


/*
 * Return true if all pages in the chunk have not yet been processed.
 */

#define CHUNK_NOT_COMPLETE(c)   ((c) != 0)

/*
 * Return true if the page at offset 'p' in the bit map has already been handled
 * while processing a higher level object in the shadow chain.
 */

#define PAGE_ALREADY_HANDLED(c, p)  (((c) & (1LL << (p))) == 0)

/*
 * Mark the page at offset 'p' in the bit map as having been processed.
 */

#define MARK_PAGE_HANDLED(c, p) \
MACRO_BEGIN \
    (c) = (c) & ~(1LL << (p)); \
MACRO_END


/*
 * Return true if the page at the given offset has been paged out.  Object is
 * locked upon entry and returned locked.
 */

// static boolean_t
// page_is_paged_out(
//     vm_object_t     object,
//     vm_object_offset_t  offset)
// {
//     return 0;
// }



/*
 * Deactivate the pages in the specified object and range.  If kill_page is set, also discard any
 * page modified state from the pmap.  Update the chunk_state as we go along.  The caller must specify
 * a size that is less than or equal to the CHUNK_SIZE.
 */

// static void
// deactivate_pages_in_object(
//     vm_object_t     object,
//     vm_object_offset_t  offset,
//     vm_object_size_t    size,
//     boolean_t               kill_page,
//     boolean_t       reusable_page,
// #if !MACH_ASSERT
//     __unused
// #endif
//     boolean_t       all_reusable,
//     chunk_state_t       *chunk_state)
// {
//     ;
// }


/*
 * Deactive a "chunk" of the given range of the object starting at offset.  A "chunk"
 * will always be less than or equal to the given size.  The total range is divided up
 * into chunks for efficiency and performance related to the locks and handling the shadow
 * chain.  This routine returns how much of the given "size" it actually processed.  It's
 * up to the caler to loop and keep calling this routine until the entire range they want
 * to process has been done.
 */

// static vm_object_size_t
// deactivate_a_chunk(
//     vm_object_t     orig_object,
//     vm_object_offset_t  offset,
//     vm_object_size_t    size,
//     boolean_t               kill_page,
//     boolean_t       reusable_page,
//     boolean_t       all_reusable)
// {
//     return 0;
// }



/*
 * Move any resident pages in the specified range to the inactive queue.  If kill_page is set,
 * we also clear the modified status of the page and "forget" any changes that have been made
 * to the page.
 */

/* __private_extern__ */ extern void
vm_object_deactivate_pages(
    vm_object_t     object,
    vm_object_offset_t  offset,
    vm_object_size_t    size,
    boolean_t               kill_page,
    boolean_t       reusable_page)
{
        kprintf("not implemented: vm_object_deactivate_pages()\n");
}

void
vm_object_reuse_pages(
    vm_object_t     object,
    vm_object_offset_t  start_offset,
    vm_object_offset_t  end_offset,
    boolean_t       allow_partial_reuse)
{
        kprintf("not implemented: vm_object_reuse_pages()\n");
}

/*
 *  Routine:    vm_object_pmap_protect
 *
 *  Purpose:
 *      Reduces the permission for all physical
 *      pages in the specified object range.
 *
 *      If removing write permission only, it is
 *      sufficient to protect only the pages in
 *      the top-level object; only those pages may
 *      have write permission.
 *
 *      If removing all access, we must follow the
 *      shadow chain from the top-level object to
 *      remove access to all pages in shadowed objects.
 *
 *      The object must *not* be locked.  The object must
 *      be temporary/internal.
 *
 *              If pmap is not NULL, this routine assumes that
 *              the only mappings for the pages are in that
 *              pmap.
 */

/* __private_extern__ */ extern void
vm_object_pmap_protect(
    register vm_object_t        object,
    register vm_object_offset_t offset,
    vm_object_size_t        size,
    pmap_t              pmap,
    vm_map_offset_t         pmap_start,
    vm_prot_t           prot)
{
        kprintf("not implemented: vm_object_pmap_protect()\n");
}

/*
 *  Routine:    vm_object_copy_slowly
 *
 *  Description:
 *      Copy the specified range of the source
 *      virtual memory object without using
 *      protection-based optimizations (such
 *      as copy-on-write).  The pages in the
 *      region are actually copied.
 *
 *  In/out conditions:
 *      The caller must hold a reference and a lock
 *      for the source virtual memory object.  The source
 *      object will be returned *unlocked*.
 *
 *  Results:
 *      If the copy is completed successfully, KERN_SUCCESS is
 *      returned.  If the caller asserted the interruptible
 *      argument, and an interruption occurred while waiting
 *      for a user-generated event, MACH_SEND_INTERRUPTED is
 *      returned.  Other values may be returned to indicate
 *      hard errors during the copy operation.
 *
 *      A new virtual memory object is returned in a
 *      parameter (_result_object).  The contents of this
 *      new object, starting at a zero offset, are a copy
 *      of the source memory region.  In the event of
 *      an error, this parameter will contain the value
 *      VM_OBJECT_NULL.
 */
/* __private_extern__ */ extern kern_return_t
vm_object_copy_slowly(
    register vm_object_t    src_object,
    vm_object_offset_t  src_offset,
    vm_object_size_t    size,
    boolean_t       interruptible,
    vm_object_t     *_result_object)    /* OUT */
{
        kprintf("not implemented: vm_object_copy_slowly()\n");
        return 0;
}

/*
 *  Routine:    vm_object_copy_quickly
 *
 *  Purpose:
 *      Copy the specified range of the source virtual
 *      memory object, if it can be done without waiting
 *      for user-generated events.
 *
 *  Results:
 *      If the copy is successful, the copy is returned in
 *      the arguments; otherwise, the arguments are not
 *      affected.
 *
 *  In/out conditions:
 *      The object should be unlocked on entry and exit.
 */

/*ARGSUSED*/
/* __private_extern__ */ extern boolean_t
vm_object_copy_quickly(
    vm_object_t     *_object,       /* INOUT */
    __unused vm_object_offset_t offset, /* IN */
    __unused vm_object_size_t   size,   /* IN */
    boolean_t       *_src_needs_copy,   /* OUT */
    boolean_t       *_dst_needs_copy)   /* OUT */
{
        kprintf("not implemented: vm_object_copy_quickly()\n");
        return 0;
}

static int copy_call_count = 0;
static int copy_call_sleep_count = 0;
static int copy_call_restart_count = 0;

/*
 *  Routine:    vm_object_copy_call [internal]
 *
 *  Description:
 *      Copy the source object (src_object), using the
 *      user-managed copy algorithm.
 *
 *  In/out conditions:
 *      The source object must be locked on entry.  It
 *      will be *unlocked* on exit.
 *
 *  Results:
 *      If the copy is successful, KERN_SUCCESS is returned.
 *      A new object that represents the copied virtual
 *      memory is returned in a parameter (*_result_object).
 *      If the return value indicates an error, this parameter
 *      is not valid.
 */
// static kern_return_t
// vm_object_copy_call(
//     vm_object_t     src_object,
//     vm_object_offset_t  src_offset,
//     vm_object_size_t    size,
//     vm_object_t     *_result_object)    /* OUT */
// {
//     return 0;
// }

static int copy_delayed_lock_collisions = 0;
static int copy_delayed_max_collisions = 0;
static int copy_delayed_lock_contention = 0;
static int copy_delayed_protect_iterate = 0;

/*
 *  Routine:    vm_object_copy_delayed [internal]
 *
 *  Description:
 *      Copy the specified virtual memory object, using
 *      the asymmetric copy-on-write algorithm.
 *
 *  In/out conditions:
 *      The src_object must be locked on entry.  It will be unlocked
 *      on exit - so the caller must also hold a reference to it.
 *
 *      This routine will not block waiting for user-generated
 *      events.  It is not interruptible.
 */
/* __private_extern__ */ extern vm_object_t
vm_object_copy_delayed(
    vm_object_t     src_object,
    vm_object_offset_t  src_offset,
    vm_object_size_t    size,
    boolean_t       src_object_shared)
{
        kprintf("not implemented: vm_object_copy_delayed()\n");
        return 0;
}

/*
 *  Routine:    vm_object_copy_strategically
 *
 *  Purpose:
 *      Perform a copy according to the source object's
 *      declared strategy.  This operation may block,
 *      and may be interrupted.
 */
/* __private_extern__ */ extern kern_return_t
vm_object_copy_strategically(
    register vm_object_t    src_object,
    vm_object_offset_t  src_offset,
    vm_object_size_t    size,
    vm_object_t     *dst_object,    /* OUT */
    vm_object_offset_t  *dst_offset,    /* OUT */
    boolean_t       *dst_needs_copy) /* OUT */
{
        kprintf("not implemented: vm_object_copy_strategically()\n");
        return 0;
}

/*
 *  vm_object_shadow:
 *
 *  Create a new object which is backed by the
 *  specified existing object range.  The source
 *  object reference is deallocated.
 *
 *  The new object and offset into that object
 *  are returned in the source parameters.
 */
boolean_t vm_object_shadow_check = TRUE;

/* __private_extern__ */ extern boolean_t
vm_object_shadow(
    vm_object_t     *object,    /* IN/OUT */
    vm_object_offset_t  *offset,    /* IN/OUT */
    vm_object_size_t    length)
{
        kprintf("not implemented: vm_object_shadow()\n");
        return 0;
}

/*
 *  The relationship between vm_object structures and
 *  the memory_object requires careful synchronization.
 *
 *  All associations are created by memory_object_create_named
 *  for external pagers and vm_object_pager_create for internal
 *  objects as follows:
 *
 *      pager:  the memory_object itself, supplied by
 *          the user requesting a mapping (or the kernel,
 *          when initializing internal objects); the
 *          kernel simulates holding send rights by keeping
 *          a port reference;
 *
 *      pager_request:
 *          the memory object control port,
 *          created by the kernel; the kernel holds
 *          receive (and ownership) rights to this
 *          port, but no other references.
 *
 *  When initialization is complete, the "initialized" field
 *  is asserted.  Other mappings using a particular memory object,
 *  and any references to the vm_object gained through the
 *  port association must wait for this initialization to occur.
 *
 *  In order to allow the memory manager to set attributes before
 *  requests (notably virtual copy operations, but also data or
 *  unlock requests) are made, a "ready" attribute is made available.
 *  Only the memory manager may affect the value of this attribute.
 *  Its value does not affect critical kernel functions, such as
 *  internal object initialization or destruction.  [Furthermore,
 *  memory objects created by the kernel are assumed to be ready
 *  immediately; the default memory manager need not explicitly
 *  set the "ready" attribute.]
 *
 *  [Both the "initialized" and "ready" attribute wait conditions
 *  use the "pager" field as the wait event.]
 *
 *  The port associations can be broken down by any of the
 *  following routines:
 *      vm_object_terminate:
 *          No references to the vm_object remain, and
 *          the object cannot (or will not) be cached.
 *          This is the normal case, and is done even
 *          though one of the other cases has already been
 *          done.
 *      memory_object_destroy:
 *          The memory manager has requested that the
 *          kernel relinquish references to the memory
 *          object. [The memory manager may not want to
 *          destroy the memory object, but may wish to
 *          refuse or tear down existing memory mappings.]
 *
 *  Each routine that breaks an association must break all of
 *  them at once.  At some later time, that routine must clear
 *  the pager field and release the memory object references.
 *  [Furthermore, each routine must cope with the simultaneous
 *  or previous operations of the others.]
 *
 *  In addition to the lock on the object, the vm_object_hash_lock
 *  governs the associations.  References gained through the
 *  association require use of the hash lock.
 *
 *  Because the pager field may be cleared spontaneously, it
 *  cannot be used to determine whether a memory object has
 *  ever been associated with a particular vm_object.  [This
 *  knowledge is important to the shadow object mechanism.]
 *  For this reason, an additional "created" attribute is
 *  provided.
 *
 *  During various paging operations, the pager reference found in the
 *  vm_object must be valid.  To prevent this from being released,
 *  (other than being removed, i.e., made null), routines may use
 *  the vm_object_paging_begin/end routines [actually, macros].
 *  The implementation uses the "paging_in_progress" and "wanted" fields.
 *  [Operations that alter the validity of the pager values include the
 *  termination routines and vm_object_collapse.]
 */


/*
 *  Routine:    vm_object_enter
 *  Purpose:
 *      Find a VM object corresponding to the given
 *      pager; if no such object exists, create one,
 *      and initialize the pager.
 */
vm_object_t
vm_object_enter(
    memory_object_t     pager,
    vm_object_size_t    size,
    boolean_t       internal,
    boolean_t       init,
    boolean_t       named)
{
        kprintf("not implemented: vm_object_enter()\n");
        return 0;
}

/*
 *  Routine:    vm_object_pager_create
 *  Purpose:
 *      Create a memory object for an internal object.
 *  In/out conditions:
 *      The object is locked on entry and exit;
 *      it may be unlocked within this call.
 *  Limitations:
 *      Only one thread may be performing a
 *      vm_object_pager_create on an object at
 *      a time.  Presumably, only the pageout
 *      daemon will be using this routine.
 */

void
vm_object_pager_create(
    register vm_object_t    object)
{
        kprintf("not implemented: vm_object_pager_create()\n");
}

/*
 *  Routine:    vm_object_remove
 *  Purpose:
 *      Eliminate the pager/object association
 *      for this pager.
 *  Conditions:
 *      The object cache must be locked.
 */
/* __private_extern__ */ extern void
vm_object_remove(
    vm_object_t object)
{
        kprintf("not implemented: vm_object_remove()\n");
}

// static void
// vm_object_do_bypass(
//     vm_object_t object,
//     vm_object_t backing_object)
// {
//     ;
// }

/*
 *  vm_object_collapse:
 *
 *  Perform an object collapse or an object bypass if appropriate.
 *  The real work of collapsing and bypassing is performed in
 *  the routines vm_object_do_collapse and vm_object_do_bypass.
 *
 *  Requires that the object be locked and the page queues be unlocked.
 *
 */
static unsigned long vm_object_collapse_calls = 0;
static unsigned long vm_object_collapse_objects = 0;
static unsigned long vm_object_collapse_do_collapse = 0;
static unsigned long vm_object_collapse_do_bypass = 0;

/* __private_extern__ */ extern void
vm_object_collapse(
    register vm_object_t            object,
    register vm_object_offset_t     hint_offset,
    boolean_t               can_bypass)
{
        kprintf("not implemented: vm_object_collapse()\n");
}

/*
 *  Routine:    vm_object_page_remove: [internal]
 *  Purpose:
 *      Removes all physical pages in the specified
 *      object range from the object's list of pages.
 *
 *  In/out conditions:
 *      The object must be locked.
 *      The object must not have paging_in_progress, usually
 *      guaranteed by not having a pager.
 */
unsigned int vm_object_page_remove_lookup = 0;
unsigned int vm_object_page_remove_iterate = 0;

/* __private_extern__ */ extern void
vm_object_page_remove(
    register vm_object_t        object,
    register vm_object_offset_t start,
    register vm_object_offset_t end)
{
        kprintf("not implemented: vm_object_page_remove()\n");
}


/*
 *  Routine:    vm_object_coalesce
 *  Function:   Coalesces two objects backing up adjoining
 *          regions of memory into a single object.
 *
 *  returns TRUE if objects were combined.
 *
 *  NOTE:   Only works at the moment if the second object is NULL -
 *      if it's not, which object do we lock first?
 *
 *  Parameters:
 *      prev_object First object to coalesce
 *      prev_offset Offset into prev_object
 *      next_object Second object into coalesce
 *      next_offset Offset into next_object
 *
 *      prev_size   Size of reference to prev_object
 *      next_size   Size of reference to next_object
 *
 *  Conditions:
 *  The object(s) must *not* be locked. The map must be locked
 *  to preserve the reference to the object(s).
 */
static int vm_object_coalesce_count = 0;

/* __private_extern__ */ extern boolean_t
vm_object_coalesce(
    register vm_object_t        prev_object,
    vm_object_t         next_object,
    vm_object_offset_t      prev_offset,
    __unused vm_object_offset_t next_offset,
    vm_object_size_t        prev_size,
    vm_object_size_t        next_size)
{
        kprintf("not implemented: vm_object_coalesce()\n");
        return 0;
}

/*
 *  Attach a set of physical pages to an object, so that they can
 *  be mapped by mapping the object.  Typically used to map IO memory.
 *
 *  The mapping function and its private data are used to obtain the
 *  physical addresses for each page to be mapped.
 */
void
vm_object_page_map(
    vm_object_t     object,
    vm_object_offset_t  offset,
    vm_object_size_t    size,
    vm_object_offset_t  (*map_fn)(void *map_fn_data,
        vm_object_offset_t offset),
        void        *map_fn_data)   /* private to map_fn */
{
        kprintf("not implemented: vm_object_page_map()\n");
}

kern_return_t
vm_object_populate_with_private(
        vm_object_t     object,
        vm_object_offset_t  offset,
        ppnum_t         phys_page,
        vm_size_t       size)
{
        kprintf("not implemented: vm_object_populate_with_private()\n");
        return 0;
}

/*
 *  memory_object_free_from_cache:
 *
 *  Walk the vm_object cache list, removing and freeing vm_objects
 *  which are backed by the pager identified by the caller, (pager_ops).
 *  Remove up to "count" objects, if there are that may available
 *  in the cache.
 *
 *  Walk the list at most once, return the number of vm_objects
 *  actually freed.
 */

/* __private_extern__ */ extern kern_return_t
memory_object_free_from_cache(
    __unused host_t     host,
    __unused memory_object_pager_ops_t pager_ops,
    int     *count)
{
        kprintf("not implemented: memory_object_free_from_cache()\n");
        return 0;
}



kern_return_t
memory_object_create_named(
    memory_object_t pager,
    memory_object_offset_t  size,
    memory_object_control_t     *control)
{
        kprintf("not implemented: memory_object_create_named()\n");
        return 0;
}


/*
 *  Routine:    memory_object_recover_named [user interface]
 *  Purpose:
 *      Attempt to recover a named reference for a VM object.
 *      VM will verify that the object has not already started
 *      down the termination path, and if it has, will optionally
 *      wait for that to finish.
 *  Returns:
 *      KERN_SUCCESS - we recovered a named reference on the object
 *      KERN_FAILURE - we could not recover a reference (object dead)
 *      KERN_INVALID_ARGUMENT - bad memory object control
 */
kern_return_t
memory_object_recover_named(
    memory_object_control_t control,
    boolean_t       wait_on_terminating)
{
        kprintf("not implemented: memory_object_recover_named()\n");
        return 0;
}


/*
 *  vm_object_release_name:
 *
 *  Enforces name semantic on memory_object reference count decrement
 *  This routine should not be called unless the caller holds a name
 *  reference gained through the memory_object_create_named.
 *
 *  If the TERMINATE_IDLE flag is set, the call will return if the
 *  reference count is not 1. i.e. idle with the only remaining reference
 *  being the name.
 *  If the decision is made to proceed the name field flag is set to
 *  false and the reference count is decremented.  If the RESPECT_CACHE
 *  flag is set and the reference count has gone to zero, the
 *  memory_object is checked to see if it is cacheable otherwise when
 *  the reference count is zero, it is simply terminated.
 */

/* __private_extern__ */ extern kern_return_t
vm_object_release_name(
    vm_object_t object,
    int     flags)
{
        kprintf("not implemented: vm_object_release_name()\n");
        return 0;
}


/* __private_extern__ */ extern kern_return_t
vm_object_lock_request(
    vm_object_t         object,
    vm_object_offset_t      offset,
    vm_object_size_t        size,
    memory_object_return_t      should_return,
    int             flags,
    vm_prot_t           prot)
{
        kprintf("not implemented: vm_object_lock_request()\n");
        return 0;
}

/*
 * Empty a purgeable object by grabbing the physical pages assigned to it and
 * putting them on the free queue without writing them to backing store, etc.
 * When the pages are next touched they will be demand zero-fill pages.  We
 * skip pages which are busy, being paged in/out, wired, etc.  We do _not_
 * skip referenced/dirty pages, pages on the active queue, etc.  We're more
 * than happy to grab these since this is a purgeable object.  We mark the
 * object as "empty" after reaping its pages.
 *
 * On entry the object must be locked and it must be
 * purgeable with no delayed copies pending.
 */
void
vm_object_purge(vm_object_t object)
{
        kprintf("not implemented: vm_object_purge()\n");
}


/*
 * vm_object_purgeable_control() allows the caller to control and investigate the
 * state of a purgeable object.  A purgeable object is created via a call to
 * vm_allocate() with VM_FLAGS_PURGABLE specified.  A purgeable object will
 * never be coalesced with any other object -- even other purgeable objects --
 * and will thus always remain a distinct object.  A purgeable object has
 * special semantics when its reference count is exactly 1.  If its reference
 * count is greater than 1, then a purgeable object will behave like a normal
 * object and attempts to use this interface will result in an error return
 * of KERN_INVALID_ARGUMENT.
 *
 * A purgeable object may be put into a "volatile" state which will make the
 * object's pages elligable for being reclaimed without paging to backing
 * store if the system runs low on memory.  If the pages in a volatile
 * purgeable object are reclaimed, the purgeable object is said to have been
 * "emptied."  When a purgeable object is emptied the system will reclaim as
 * many pages from the object as it can in a convenient manner (pages already
 * en route to backing store or busy for other reasons are left as is).  When
 * a purgeable object is made volatile, its pages will generally be reclaimed
 * before other pages in the application's working set.  This semantic is
 * generally used by applications which can recreate the data in the object
 * faster than it can be paged in.  One such example might be media assets
 * which can be reread from a much faster RAID volume.
 *
 * A purgeable object may be designated as "non-volatile" which means it will
 * behave like all other objects in the system with pages being written to and
 * read from backing store as needed to satisfy system memory needs.  If the
 * object was emptied before the object was made non-volatile, that fact will
 * be returned as the old state of the purgeable object (see
 * VM_PURGABLE_SET_STATE below).  In this case, any pages of the object which
 * were reclaimed as part of emptying the object will be refaulted in as
 * zero-fill on demand.  It is up to the application to note that an object
 * was emptied and recreate the objects contents if necessary.  When a
 * purgeable object is made non-volatile, its pages will generally not be paged
 * out to backing store in the immediate future.  A purgeable object may also
 * be manually emptied.
 *
 * Finally, the current state (non-volatile, volatile, volatile & empty) of a
 * volatile purgeable object may be queried at any time.  This information may
 * be used as a control input to let the application know when the system is
 * experiencing memory pressure and is reclaiming memory.
 *
 * The specified address may be any address within the purgeable object.  If
 * the specified address does not represent any object in the target task's
 * virtual address space, then KERN_INVALID_ADDRESS will be returned.  If the
 * object containing the specified address is not a purgeable object, then
 * KERN_INVALID_ARGUMENT will be returned.  Otherwise, KERN_SUCCESS will be
 * returned.
 *
 * The control parameter may be any one of VM_PURGABLE_SET_STATE or
 * VM_PURGABLE_GET_STATE.  For VM_PURGABLE_SET_STATE, the in/out parameter
 * state is used to set the new state of the purgeable object and return its
 * old state.  For VM_PURGABLE_GET_STATE, the current state of the purgeable
 * object is returned in the parameter state.
 *
 * The in/out parameter state may be one of VM_PURGABLE_NONVOLATILE,
 * VM_PURGABLE_VOLATILE or VM_PURGABLE_EMPTY.  These, respectively, represent
 * the non-volatile, volatile and volatile/empty states described above.
 * Setting the state of a purgeable object to VM_PURGABLE_EMPTY will
 * immediately reclaim as many pages in the object as can be conveniently
 * collected (some may have already been written to backing store or be
 * otherwise busy).
 *
 * The process of making a purgeable object non-volatile and determining its
 * previous state is atomic.  Thus, if a purgeable object is made
 * VM_PURGABLE_NONVOLATILE and the old state is returned as
 * VM_PURGABLE_VOLATILE, then the purgeable object's previous contents are
 * completely intact and will remain so until the object is made volatile
 * again.  If the old state is returned as VM_PURGABLE_EMPTY then the object
 * was reclaimed while it was in a volatile state and its previous contents
 * have been lost.
 */
/*
 * The object must be locked.
 */
kern_return_t
vm_object_purgable_control(
    vm_object_t object,
    vm_purgable_t   control,
    int     *state)
{
        kprintf("not implemented: vm_object_purgable_control()\n");
        return 0;
}

#if TASK_SWAPPER
/*
 * vm_object_res_deallocate
 *
 * (recursively) decrement residence counts on vm objects and their shadows.
 * Called from vm_object_deallocate and when swapping out an object.
 *
 * The object is locked, and remains locked throughout the function,
 * even as we iterate down the shadow chain.  Locks on intermediate objects
 * will be dropped, but not the original object.
 *
 * NOTE: this function used to use recursion, rather than iteration.
 */

/* __private_extern__ */ extern void
vm_object_res_deallocate(
    vm_object_t object)
{
        kprintf("not implemented: vm_object_res_deallocate()\n");
}

/*
 * vm_object_res_reference
 *
 * Internal function to increment residence count on a vm object
 * and its shadows.  It is called only from vm_object_reference, and
 * when swapping in a vm object, via vm_map_swap.
 *
 * The object is locked, and remains locked throughout the function,
 * even as we iterate down the shadow chain.  Locks on intermediate objects
 * will be dropped, but not the original object.
 *
 * NOTE: this function used to use recursion, rather than iteration.
 */

/* __private_extern__ */ extern void
vm_object_res_reference(
    vm_object_t object)
{
        kprintf("not implemented: vm_object_res_reference()\n");
}
#endif  /* TASK_SWAPPER */

/*
 *  vm_object_reference:
 *
 *  Gets another reference to the given object.
 */
#ifdef vm_object_reference
#undef vm_object_reference
#endif
/* __private_extern__ */ extern void
vm_object_reference(
    register vm_object_t    object)
{
        kprintf("not implemented: vm_object_reference()\n");
}

#ifdef MACH_BSD
/*
 * Scale the vm_object_cache
 * This is required to make sure that the vm_object_cache is big
 * enough to effectively cache the mapped file.
 * This is really important with UBC as all the regular file vnodes
 * have memory object associated with them. Havving this cache too
 * small results in rapid reclaim of vnodes and hurts performance a LOT!
 *
 * This is also needed as number of vnodes can be dynamically scaled.
 */
kern_return_t
adjust_vm_object_cache(
    __unused vm_size_t oval,
    __unused vm_size_t nval)
{
        kprintf("not implemented: adjust_vm_object_cache()\n");
        return 0;
}
#endif /* MACH_BSD */


/*
 * vm_object_transpose
 *
 * This routine takes two VM objects of the same size and exchanges
 * their backing store.
 * The objects should be "quiesced" via a UPL operation with UPL_SET_IO_WIRE
 * and UPL_BLOCK_ACCESS if they are referenced anywhere.
 *
 * The VM objects must not be locked by caller.
 */
unsigned int vm_object_transpose_count = 0;
kern_return_t
vm_object_transpose(
    vm_object_t     object1,
    vm_object_t     object2,
    vm_object_size_t    transpose_size)
{
        kprintf("not implemented: vm_object_transpose()\n");
        return 0;
}


/*
 *      vm_object_cluster_size
 *
 *      Determine how big a cluster we should issue an I/O for...
 *
 *  Inputs:   *start == offset of page needed
 *        *length == maximum cluster pager can handle
 *  Outputs:  *start == beginning offset of cluster
 *        *length == length of cluster to try
 *
 *  The original *start will be encompassed by the cluster
 *
 */
extern int speculative_reads_disabled;
extern int ignore_is_ssd;

#if CONFIG_EMBEDDED
unsigned int preheat_pages_max = MAX_UPL_TRANSFER;
unsigned int preheat_pages_min = 10;
#else
unsigned int preheat_pages_max = MAX_UPL_TRANSFER;
unsigned int preheat_pages_min = 8;
#endif

uint32_t pre_heat_scaling[MAX_UPL_TRANSFER + 1];
uint32_t pre_heat_cluster[MAX_UPL_TRANSFER + 1];


/* __private_extern__ */ extern void
vm_object_cluster_size(vm_object_t object, vm_object_offset_t *start,
               vm_size_t *length, vm_object_fault_info_t fault_info, uint32_t *io_streaming)
{
        kprintf("not implemented: vm_object_cluster_size()\n");
}


/*
 * Allow manipulation of individual page state.  This is actually part of
 * the UPL regimen but takes place on the VM object rather than on a UPL
 */

kern_return_t
vm_object_page_op(
    vm_object_t     object,
    vm_object_offset_t  offset,
    int         ops,
    ppnum_t         *phys_entry,
    int         *flags)
{
        kprintf("not implemented: vm_object_page_op()\n");
        return 0;
}

/*
 * vm_object_range_op offers performance enhancement over
 * vm_object_page_op for page_op functions which do not require page
 * level state to be returned from the call.  Page_op was created to provide
 * a low-cost alternative to page manipulation via UPLs when only a single
 * page was involved.  The range_op call establishes the ability in the _op
 * family of functions to work on multiple pages where the lack of page level
 * state handling allows the caller to avoid the overhead of the upl structures.
 */

kern_return_t
vm_object_range_op(
    vm_object_t     object,
    vm_object_offset_t  offset_beg,
    vm_object_offset_t  offset_end,
    int                     ops,
    uint32_t        *range)
{
        kprintf("not implemented: vm_object_range_op()\n");
        return 0;
}


uint32_t scan_object_collision = 0;

void
vm_object_lock(vm_object_t object)
{
        kprintf("not implemented: vm_object_lock()\n");
}

boolean_t
vm_object_lock_avoid(vm_object_t object)
{
        kprintf("not implemented: vm_object_lock_avoid()\n");
        return 0;
}

boolean_t
_vm_object_lock_try(vm_object_t object)
{
        kprintf("not implemented: _vm_object_lock_try()\n");
        return 0;
}

boolean_t
vm_object_lock_try(vm_object_t object)
{
        kprintf("not implemented: vm_object_lock_try()\n");
        return 0;
}

void
vm_object_lock_shared(vm_object_t object)
{
        kprintf("not implemented: vm_object_lock_shared()\n");
}

boolean_t
vm_object_lock_try_shared(vm_object_t object)
{
        kprintf("not implemented: vm_object_lock_try_shared()\n");
        return 0;
}


unsigned int vm_object_change_wimg_mode_count = 0;

/*
 * The object must be locked
 */
void
vm_object_change_wimg_mode(vm_object_t object, unsigned int wimg_mode)
{
        kprintf("not implemented: vm_object_change_wimg_mode()\n");
}

#if CONFIG_FREEZE

kern_return_t vm_object_pack(
    unsigned int    *purgeable_count,
    unsigned int    *wired_count,
    unsigned int    *clean_count,
    unsigned int    *dirty_count,
    unsigned int    dirty_budget,
    boolean_t   *shared,
    vm_object_t src_object,
    struct default_freezer_handle *df_handle)
{
        kprintf("not implemented: vm_object_pack()\n");
        return 0;
}


void
vm_object_pack_pages(
    unsigned int        *wired_count,
    unsigned int        *clean_count,
    unsigned int        *dirty_count,
    unsigned int        dirty_budget,
    vm_object_t     src_object,
    struct default_freezer_handle *df_handle)
{
        kprintf("not implemented: vm_object_pack_pages()\n");
}

void
vm_object_pageout(
    vm_object_t object)
{
        kprintf("not implemented: vm_object_pageout()\n");
}

kern_return_t
vm_object_pagein(
    vm_object_t object)
{
        kprintf("not implemented: vm_object_pagein()\n");
        return 0;
}
#endif /* CONFIG_FREEZE */
