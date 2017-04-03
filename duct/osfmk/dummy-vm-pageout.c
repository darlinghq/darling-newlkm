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
 *  File:   vm/vm_pageout.c
 *  Author: Avadis Tevanian, Jr., Michael Wayne Young
 *  Date:   1985
 *
 *  The proverbial page-out daemon.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <stdint.h>

#include <debug.h>
#include <mach_pagemap.h>
#include <mach_cluster_stats.h>
#include <advisory_pageout.h>

#include <mach/mach_types.h>
#include <mach/memory_object.h>
#include <mach/memory_object_default.h>
#include <mach/memory_object_control_server.h>
#include <mach/mach_host_server.h>
#include <mach/upl.h>
#include <mach/vm_map.h>
#include <mach/vm_param.h>
#include <mach/vm_statistics.h>
#include <mach/sdt.h>

#include <kern/kern_types.h>
#include <kern/counters.h>
#include <kern/host_statistics.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/sched.h>
#include <kern/thread.h>
#include <kern/xpr.h>
#include <kern/kalloc.h>

#include <machine/vm_tuning.h>
#include <machine/commpage.h>

#include <vm/pmap.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h> /* must be last */
#include <vm/memory_object.h>
#include <vm/vm_purgeable_internal.h>
#include <vm/vm_shared_region.h>
/*
 * ENCRYPTED SWAP:
 */
#include <libkern/crypto/aes.h>
extern u_int32_t random(void);  /* from <libkern/libkern.h> */

extern int cs_debug;

#if UPL_DEBUG
#include <libkern/OSDebug.h>
#endif

#if VM_PRESSURE_EVENTS
extern void consider_vm_pressure_events(void);
#endif

#ifndef VM_PAGEOUT_BURST_ACTIVE_THROTTLE   /* maximum iterations of the active queue to move pages to inactive */
#define VM_PAGEOUT_BURST_ACTIVE_THROTTLE  100
#endif

#ifndef VM_PAGEOUT_BURST_INACTIVE_THROTTLE  /* maximum iterations of the inactive queue w/o stealing/cleaning a page */
#ifdef  CONFIG_EMBEDDED
#define VM_PAGEOUT_BURST_INACTIVE_THROTTLE 1024
#else
#define VM_PAGEOUT_BURST_INACTIVE_THROTTLE 4096
#endif
#endif

#ifndef VM_PAGEOUT_DEADLOCK_RELIEF
#define VM_PAGEOUT_DEADLOCK_RELIEF 100  /* number of pages to move to break deadlock */
#endif

#ifndef VM_PAGEOUT_INACTIVE_RELIEF
#define VM_PAGEOUT_INACTIVE_RELIEF 50   /* minimum number of pages to move to the inactive q */
#endif

#ifndef VM_PAGE_LAUNDRY_MAX
#define VM_PAGE_LAUNDRY_MAX 128UL   /* maximum pageouts on a given pageout queue */
#endif  /* VM_PAGEOUT_LAUNDRY_MAX */

#ifndef VM_PAGEOUT_BURST_WAIT
#define VM_PAGEOUT_BURST_WAIT   30  /* milliseconds */
#endif  /* VM_PAGEOUT_BURST_WAIT */

#ifndef VM_PAGEOUT_EMPTY_WAIT
#define VM_PAGEOUT_EMPTY_WAIT   200 /* milliseconds */
#endif  /* VM_PAGEOUT_EMPTY_WAIT */

#ifndef VM_PAGEOUT_DEADLOCK_WAIT
#define VM_PAGEOUT_DEADLOCK_WAIT    300 /* milliseconds */
#endif  /* VM_PAGEOUT_DEADLOCK_WAIT */

#ifndef VM_PAGEOUT_IDLE_WAIT
#define VM_PAGEOUT_IDLE_WAIT    10  /* milliseconds */
#endif  /* VM_PAGEOUT_IDLE_WAIT */

#ifndef VM_PAGEOUT_PRESSURE_PAGES_CONSIDERED
#define VM_PAGEOUT_PRESSURE_PAGES_CONSIDERED        1000    /* maximum pages considered before we issue a pressure event */
#endif /* VM_PAGEOUT_PRESSURE_PAGES_CONSIDERED */

#ifndef VM_PAGEOUT_PRESSURE_EVENT_MONITOR_SECS
#define VM_PAGEOUT_PRESSURE_EVENT_MONITOR_SECS      5   /* seconds */
#endif /* VM_PAGEOUT_PRESSURE_EVENT_MONITOR_SECS */

unsigned int    vm_page_speculative_q_age_ms = VM_PAGE_SPECULATIVE_Q_AGE_MS;
unsigned int    vm_page_speculative_percentage = 5;

#ifndef VM_PAGE_SPECULATIVE_TARGET
#define VM_PAGE_SPECULATIVE_TARGET(total) ((total) * 1 / (100 / vm_page_speculative_percentage))
#endif /* VM_PAGE_SPECULATIVE_TARGET */


#ifndef VM_PAGE_INACTIVE_HEALTHY_LIMIT
#define VM_PAGE_INACTIVE_HEALTHY_LIMIT(total) ((total) * 1 / 200)
#endif /* VM_PAGE_INACTIVE_HEALTHY_LIMIT */


/*
 *  To obtain a reasonable LRU approximation, the inactive queue
 *  needs to be large enough to give pages on it a chance to be
 *  referenced a second time.  This macro defines the fraction
 *  of active+inactive pages that should be inactive.
 *  The pageout daemon uses it to update vm_page_inactive_target.
 *
 *  If vm_page_free_count falls below vm_page_free_target and
 *  vm_page_inactive_count is below vm_page_inactive_target,
 *  then the pageout daemon starts running.
 */

#ifndef VM_PAGE_INACTIVE_TARGET
#define VM_PAGE_INACTIVE_TARGET(avail)  ((avail) * 1 / 2)
#endif  /* VM_PAGE_INACTIVE_TARGET */

/*
 *  Once the pageout daemon starts running, it keeps going
 *  until vm_page_free_count meets or exceeds vm_page_free_target.
 */

#ifndef VM_PAGE_FREE_TARGET
#ifdef  CONFIG_EMBEDDED
#define VM_PAGE_FREE_TARGET(free)   (15 + (free) / 100)
#else
#define VM_PAGE_FREE_TARGET(free)   (15 + (free) / 80)
#endif
#endif  /* VM_PAGE_FREE_TARGET */

/*
 *  The pageout daemon always starts running once vm_page_free_count
 *  falls below vm_page_free_min.
 */

#ifndef VM_PAGE_FREE_MIN
#ifdef  CONFIG_EMBEDDED
#define VM_PAGE_FREE_MIN(free)      (10 + (free) / 200)
#else
#define VM_PAGE_FREE_MIN(free)      (10 + (free) / 100)
#endif
#endif  /* VM_PAGE_FREE_MIN */

#define VM_PAGE_FREE_RESERVED_LIMIT 100
#define VM_PAGE_FREE_MIN_LIMIT      1500
#define VM_PAGE_FREE_TARGET_LIMIT   2000


/*
 *  When vm_page_free_count falls below vm_page_free_reserved,
 *  only vm-privileged threads can allocate pages.  vm-privilege
 *  allows the pageout daemon and default pager (and any other
 *  associated threads needed for default pageout) to continue
 *  operation by dipping into the reserved pool of pages.
 */

#ifndef VM_PAGE_FREE_RESERVED
#define VM_PAGE_FREE_RESERVED(n)    \
    ((unsigned) (6 * VM_PAGE_LAUNDRY_MAX) + (n))
#endif  /* VM_PAGE_FREE_RESERVED */

/*
 *  When we dequeue pages from the inactive list, they are
 *  reactivated (ie, put back on the active queue) if referenced.
 *  However, it is possible to starve the free list if other
 *  processors are referencing pages faster than we can turn off
 *  the referenced bit.  So we limit the number of reactivations
 *  we will make per call of vm_pageout_scan().
 */
#define VM_PAGE_REACTIVATE_LIMIT_MAX 20000
#ifndef VM_PAGE_REACTIVATE_LIMIT
#ifdef  CONFIG_EMBEDDED
#define VM_PAGE_REACTIVATE_LIMIT(avail) (VM_PAGE_INACTIVE_TARGET(avail) / 2)
#else
#define VM_PAGE_REACTIVATE_LIMIT(avail) (MAX((avail) * 1 / 20,VM_PAGE_REACTIVATE_LIMIT_MAX))
#endif
#endif  /* VM_PAGE_REACTIVATE_LIMIT */
#define VM_PAGEOUT_INACTIVE_FORCE_RECLAIM   100


extern boolean_t hibernate_cleaning_in_progress;

/*
 * Exported variable used to broadcast the activation of the pageout scan
 * Working Set uses this to throttle its use of pmap removes.  In this
 * way, code which runs within memory in an uncontested context does
 * not keep encountering soft faults.
 */

unsigned int    vm_pageout_scan_event_counter = 0;

/*
 * Forward declarations for internal routines.
 */

// static void vm_pressure_thread(void);
// static void vm_pageout_garbage_collect(int);
// static void vm_pageout_iothread_continue(struct vm_pageout_queue *);
// static void vm_pageout_iothread_external(void);
// static void vm_pageout_iothread_internal(void);
// static void vm_pageout_adjust_io_throttles(struct vm_pageout_queue *, struct vm_pageout_queue *, boolean_t);

extern void vm_pageout_continue(void);
extern void vm_pageout_scan(void);

static thread_t vm_pageout_external_iothread = THREAD_NULL;
static thread_t vm_pageout_internal_iothread = THREAD_NULL;

unsigned int vm_pageout_reserved_internal = 0;
unsigned int vm_pageout_reserved_really = 0;

unsigned int vm_pageout_idle_wait = 0;      /* milliseconds */
unsigned int vm_pageout_empty_wait = 0;     /* milliseconds */
unsigned int vm_pageout_burst_wait = 0;     /* milliseconds */
unsigned int vm_pageout_deadlock_wait = 0;  /* milliseconds */
unsigned int vm_pageout_deadlock_relief = 0;
unsigned int vm_pageout_inactive_relief = 0;
unsigned int vm_pageout_burst_active_throttle = 0;
unsigned int vm_pageout_burst_inactive_throttle = 0;

int vm_upl_wait_for_pages = 0;


/*
 *  These variables record the pageout daemon's actions:
 *  how many pages it looks at and what happens to those pages.
 *  No locking needed because only one thread modifies the variables.
 */

unsigned int vm_pageout_active = 0;     /* debugging */
unsigned int vm_pageout_active_busy = 0;    /* debugging */
unsigned int vm_pageout_inactive = 0;       /* debugging */
unsigned int vm_pageout_inactive_throttled = 0; /* debugging */
unsigned int vm_pageout_inactive_forced = 0;    /* debugging */
unsigned int vm_pageout_inactive_nolock = 0;    /* debugging */
unsigned int vm_pageout_inactive_avoid = 0; /* debugging */
unsigned int vm_pageout_inactive_busy = 0;  /* debugging */
unsigned int vm_pageout_inactive_error = 0; /* debugging */
unsigned int vm_pageout_inactive_absent = 0;    /* debugging */
unsigned int vm_pageout_inactive_notalive = 0;  /* debugging */
unsigned int vm_pageout_inactive_used = 0;  /* debugging */
unsigned int vm_pageout_cache_evicted = 0;  /* debugging */
unsigned int vm_pageout_inactive_clean = 0; /* debugging */
unsigned int vm_pageout_speculative_clean = 0;  /* debugging */

unsigned int vm_pageout_freed_from_cleaned = 0;
unsigned int vm_pageout_freed_from_speculative = 0;
unsigned int vm_pageout_freed_from_inactive_clean = 0;

unsigned int vm_pageout_enqueued_cleaned_from_inactive_clean = 0;
unsigned int vm_pageout_enqueued_cleaned_from_inactive_dirty = 0;

unsigned int vm_pageout_cleaned_reclaimed = 0;      /* debugging; how many cleaned pages are reclaimed by the pageout scan */
unsigned int vm_pageout_cleaned_reactivated = 0;    /* debugging; how many cleaned pages are found to be referenced on pageout (and are therefore reactivated) */
unsigned int vm_pageout_cleaned_reference_reactivated = 0;
unsigned int vm_pageout_cleaned_volatile_reactivated = 0;
unsigned int vm_pageout_cleaned_fault_reactivated = 0;
unsigned int vm_pageout_cleaned_commit_reactivated = 0; /* debugging; how many cleaned pages are found to be referenced on commit (and are therefore reactivated) */
unsigned int vm_pageout_cleaned_busy = 0;
unsigned int vm_pageout_cleaned_nolock = 0;

unsigned int vm_pageout_inactive_dirty_internal = 0;    /* debugging */
unsigned int vm_pageout_inactive_dirty_external = 0;    /* debugging */
unsigned int vm_pageout_inactive_deactivated = 0;   /* debugging */
unsigned int vm_pageout_inactive_anonymous = 0; /* debugging */
unsigned int vm_pageout_dirty_no_pager = 0; /* debugging */
unsigned int vm_pageout_purged_objects = 0; /* debugging */
unsigned int vm_stat_discard = 0;       /* debugging */
unsigned int vm_stat_discard_sent = 0;      /* debugging */
unsigned int vm_stat_discard_failure = 0;   /* debugging */
unsigned int vm_stat_discard_throttle = 0;  /* debugging */
unsigned int vm_pageout_reactivation_limit_exceeded = 0;    /* debugging */
unsigned int vm_pageout_catch_ups = 0;              /* debugging */
unsigned int vm_pageout_inactive_force_reclaim = 0; /* debugging */

unsigned int vm_pageout_scan_reclaimed_throttled = 0;
unsigned int vm_pageout_scan_active_throttled = 0;
unsigned int vm_pageout_scan_inactive_throttled_internal = 0;
unsigned int vm_pageout_scan_inactive_throttled_external = 0;
unsigned int vm_pageout_scan_throttle = 0;          /* debugging */
unsigned int vm_pageout_scan_burst_throttle = 0;        /* debugging */
unsigned int vm_pageout_scan_empty_throttle = 0;        /* debugging */
unsigned int vm_pageout_scan_deadlock_detected = 0;     /* debugging */
unsigned int vm_pageout_scan_active_throttle_success = 0;   /* debugging */
unsigned int vm_pageout_scan_inactive_throttle_success = 0; /* debugging */
unsigned int vm_pageout_inactive_external_forced_reactivate_count = 0;  /* debugging */
unsigned int vm_pageout_inactive_external_forced_jetsam_count = 0;  /* debugging */
unsigned int vm_page_speculative_count_drifts = 0;
unsigned int vm_page_speculative_count_drift_max = 0;


unsigned int vm_precleaning_aborted = 0;

static boolean_t vm_pageout_need_to_refill_clean_queue = FALSE;
static boolean_t vm_pageout_precleaning_delayed = FALSE;

/*
 * Backing store throttle when BS is exhausted
 */
unsigned int    vm_backing_store_low = 0;

unsigned int vm_pageout_out_of_line  = 0;
unsigned int vm_pageout_in_place  = 0;

unsigned int vm_page_steal_pageout_page = 0;

/*
 * ENCRYPTED SWAP:
 * counters and statistics...
 */
unsigned long vm_page_decrypt_counter = 0;
unsigned long vm_page_decrypt_for_upl_counter = 0;
unsigned long vm_page_encrypt_counter = 0;
unsigned long vm_page_encrypt_abort_counter = 0;
unsigned long vm_page_encrypt_already_encrypted_counter = 0;
boolean_t vm_pages_encrypted = FALSE; /* are there encrypted pages ? */

struct  vm_pageout_queue vm_pageout_queue_internal;
struct  vm_pageout_queue vm_pageout_queue_external;

unsigned int vm_page_speculative_target = 0;

vm_object_t     vm_pageout_scan_wants_object = VM_OBJECT_NULL;

boolean_t (* volatile consider_buffer_cache_collect)(int) = NULL;

#if DEVELOPMENT || DEBUG
unsigned long vm_cs_validated_resets = 0;
#endif

int vm_debug_events = 0;

#if CONFIG_MEMORYSTATUS
extern int memorystatus_wakeup;
#endif
#if CONFIG_JETSAM
extern int memorystatus_kill_top_proc_from_VM(void);
#endif

/*
 *  Routine:    vm_backing_store_disable
 *  Purpose:
 *      Suspend non-privileged threads wishing to extend
 *      backing store when we are low on backing store
 *      (Synchronized by caller)
 */
void
vm_backing_store_disable(
    boolean_t   disable)
{
        kprintf("not implemented: vm_backing_store_disable()\n");
}


#if MACH_CLUSTER_STATS
unsigned long vm_pageout_cluster_dirtied = 0;
unsigned long vm_pageout_cluster_cleaned = 0;
unsigned long vm_pageout_cluster_collisions = 0;
unsigned long vm_pageout_cluster_clusters = 0;
unsigned long vm_pageout_cluster_conversions = 0;
unsigned long vm_pageout_target_collisions = 0;
unsigned long vm_pageout_target_page_dirtied = 0;
unsigned long vm_pageout_target_page_freed = 0;
#define CLUSTER_STAT(clause)    clause
#else   /* MACH_CLUSTER_STATS */
#define CLUSTER_STAT(clause)
#endif  /* MACH_CLUSTER_STATS */

/*
 *  Routine:    vm_pageout_object_terminate
 *  Purpose:
 *      Destroy the pageout_object, and perform all of the
 *      required cleanup actions.
 *
 *  In/Out conditions:
 *      The object must be locked, and will be returned locked.
 */
void
vm_pageout_object_terminate(
    vm_object_t object)
{
        kprintf("not implemented: vm_pageout_object_terminate()\n");
}

/*
 * Routine: vm_pageclean_setup
 *
 * Purpose: setup a page to be cleaned (made non-dirty), but not
 *      necessarily flushed from the VM page cache.
 *      This is accomplished by cleaning in place.
 *
 *      The page must not be busy, and new_object
 *      must be locked.
 *
 */
void
vm_pageclean_setup(
    vm_page_t       m,
    vm_page_t       new_m,
    vm_object_t     new_object,
    vm_object_offset_t  new_offset)
{
        kprintf("not implemented: vm_pageclean_setup()\n");
}

/*
 *  Routine:    vm_pageout_initialize_page
 *  Purpose:
 *      Causes the specified page to be initialized in
 *      the appropriate memory object. This routine is used to push
 *      pages into a copy-object when they are modified in the
 *      permanent object.
 *
 *      The page is moved to a temporary object and paged out.
 *
 *  In/out conditions:
 *      The page in question must not be on any pageout queues.
 *      The object to which it belongs must be locked.
 *      The page must be busy, but not hold a paging reference.
 *
 *  Implementation:
 *      Move this page to a completely new object.
 */
void
vm_pageout_initialize_page(
    vm_page_t   m)
{
        kprintf("not implemented: vm_pageout_initialize_page()\n");
}

#if MACH_CLUSTER_STATS
#define MAXCLUSTERPAGES 16
struct {
    unsigned long pages_in_cluster;
    unsigned long pages_at_higher_offsets;
    unsigned long pages_at_lower_offsets;
} cluster_stats[MAXCLUSTERPAGES];
#endif  /* MACH_CLUSTER_STATS */


/*
 * vm_pageout_cluster:
 *
 * Given a page, queue it to the appropriate I/O thread,
 * which will page it out and attempt to clean adjacent pages
 * in the same operation.
 *
 * The page must be busy, and the object and queues locked. We will take a
 * paging reference to prevent deallocation or collapse when we
 * release the object lock back at the call site.  The I/O thread
 * is responsible for consuming this reference
 *
 * The page must not be on any pageout queue.
 */

void
vm_pageout_cluster(vm_page_t m, boolean_t pageout)
{
        kprintf("not implemented: vm_pageout_cluster()\n");
}


unsigned long vm_pageout_throttle_up_count = 0;

/*
 * A page is back from laundry or we are stealing it back from
 * the laundering state.  See if there are some pages waiting to
 * go to laundry and if we can let some of them go now.
 *
 * Object and page queues must be locked.
 */
void
vm_pageout_throttle_up(
       vm_page_t       m)
{
        kprintf("not implemented: vm_pageout_throttle_up()\n");
}


/*
 * VM memory pressure monitoring.
 *
 * vm_pageout_scan() keeps track of the number of pages it considers and
 * reclaims, in the currently active vm_pageout_stat[vm_pageout_stat_now].
 *
 * compute_memory_pressure() is called every second from compute_averages()
 * and moves "vm_pageout_stat_now" forward, to start accumulating the number
 * of recalimed pages in a new vm_pageout_stat[] bucket.
 *
 * mach_vm_pressure_monitor() collects past statistics about memory pressure.
 * The caller provides the number of seconds ("nsecs") worth of statistics
 * it wants, up to 30 seconds.
 * It computes the number of pages reclaimed in the past "nsecs" seconds and
 * also returns the number of pages the system still needs to reclaim at this
 * moment in time.
 */
#define VM_PAGEOUT_STAT_SIZE    31
struct vm_pageout_stat {
    unsigned int considered;
    unsigned int reclaimed;
} vm_pageout_stats[VM_PAGEOUT_STAT_SIZE] = {{0,0}, };
unsigned int vm_pageout_stat_now = 0;
unsigned int vm_memory_pressure = 0;

#define VM_PAGEOUT_STAT_BEFORE(i) \
    (((i) == 0) ? VM_PAGEOUT_STAT_SIZE - 1 : (i) - 1)
#define VM_PAGEOUT_STAT_AFTER(i) \
    (((i) == VM_PAGEOUT_STAT_SIZE - 1) ? 0 : (i) + 1)

/*
 * Called from compute_averages().
 */
void
compute_memory_pressure(
    __unused void *arg)
{
        kprintf("not implemented: compute_memory_pressure()\n");
}


/*
 * IMPORTANT
 * mach_vm_ctl_page_free_wanted() is called indirectly, via
 * mach_vm_pressure_monitor(), when taking a stackshot. Therefore,
 * it must be safe in the restricted stackshot context. Locks and/or
 * blocking are not allowable.
 */
unsigned int
mach_vm_ctl_page_free_wanted(void)
{
        kprintf("not implemented: mach_vm_ctl_page_free_wanted()\n");
        return 0;
}


/*
 * IMPORTANT:
 * mach_vm_pressure_monitor() is called when taking a stackshot, with
 * wait_for_pressure FALSE, so that code path must remain safe in the
 * restricted stackshot context. No blocking or locks are allowable.
 * on that code path.
 */

kern_return_t
mach_vm_pressure_monitor(
    boolean_t   wait_for_pressure,
    unsigned int    nsecs_monitored,
    unsigned int    *pages_reclaimed_p,
    unsigned int    *pages_wanted_p)
{
        kprintf("not implemented: mach_vm_pressure_monitor()\n");
        return 0;
}



/*
 * function in BSD to apply I/O throttle to the pageout thread
 */
extern void vm_pageout_io_throttle(void);


/*
 * Page States: Used below to maintain the page state
 * before it's removed from it's Q. This saved state
 * helps us do the right accounting in certain cases
 */
#define PAGE_STATE_SPECULATIVE      1
#define PAGE_STATE_ANONYMOUS        2
#define PAGE_STATE_INACTIVE     3
#define PAGE_STATE_INACTIVE_FIRST   4
#define PAGE_STATE_CLEAN      5

#define VM_PAGEOUT_SCAN_HANDLE_REUSABLE_PAGE(m)             \
    MACRO_BEGIN                         \
    /*                              \
     * If a "reusable" page somehow made it back into       \
     * the active queue, it's been re-used and is not       \
     * quite re-usable.                     \
     * If the VM object was "all_reusable", consider it     \
     * as "all re-used" instead of converting it to         \
     * "partially re-used", which could be expensive.       \
     */                             \
    if ((m)->reusable ||                        \
        (m)->object->all_reusable) {                \
        vm_object_reuse_pages((m)->object,          \
                      (m)->offset,          \
                      (m)->offset + PAGE_SIZE_64,   \
                      FALSE);               \
    }                               \
    MACRO_END


#define VM_PAGEOUT_DELAYED_UNLOCK_LIMIT     128
#define VM_PAGEOUT_DELAYED_UNLOCK_LIMIT_MAX 1024

#define FCS_IDLE        0
#define FCS_DELAYED     1
#define FCS_DEADLOCK_DETECTED   2

struct flow_control {
        int     state;
        mach_timespec_t ts;
};

uint32_t vm_pageout_considered_page = 0;


/*
 *  vm_pageout_scan does the dirty work for the pageout daemon.
 *  It returns with both vm_page_queue_free_lock and vm_page_queue_lock
 *  held and vm_page_free_wanted == 0.
 */
void
vm_pageout_scan(void)
{
        kprintf("not implemented: vm_pageout_scan()\n");
}


int vm_page_free_count_init;

void
vm_page_free_reserve(
    int pages)
{
        kprintf("not implemented: vm_page_free_reserve()\n");
}

/*
 *  vm_pageout is the high level pageout daemon.
 */

void
vm_pageout_continue(void)
{
        kprintf("not implemented: vm_pageout_continue()\n");
}


#ifdef FAKE_DEADLOCK

#define FAKE_COUNT  5000

int internal_count = 0;
int fake_deadlock = 0;

#endif

// static void
// vm_pageout_iothread_continue(struct vm_pageout_queue *q)
// {
//     ;
// }



// static void
// vm_pageout_adjust_io_throttles(struct vm_pageout_queue *iq, struct vm_pageout_queue *eq, boolean_t req_lowpriority)
// {
//     ;
// }


// static void
// vm_pageout_iothread_external(void)
// {
//     ;
// }

// static void
// vm_pageout_iothread_internal(void)
// {
//     ;
// }

kern_return_t
vm_set_buffer_cleanup_callout(boolean_t (*func)(int))
{
        kprintf("not implemented: vm_set_buffer_cleanup_callout()\n");
        return 0;
}

// static void
// vm_pressure_thread(void)
// {
//     ;
// }

uint32_t vm_pageout_considered_page_last = 0;

/*
 * called once per-second via "compute_averages"
 */
// void
// compute_pageout_gc_throttle()
// {
//     ;
// }


// static void
// vm_pageout_garbage_collect(int collect)
// {
//     ;
// }



void
vm_pageout(void)
{
        kprintf("not implemented: vm_pageout()\n");
}

kern_return_t
vm_pageout_internal_start(void)
{
        kprintf("not implemented: vm_pageout_internal_start()\n");
        return 0;
}


// static upl_t
// upl_create(int type, int flags, upl_size_t size)
// {
//     return 0;
// }

// static void
// upl_destroy(upl_t upl)
// {
//     ;
// }

void
upl_deallocate(upl_t upl)
{
        kprintf("not implemented: upl_deallocate()\n");
}

#if DEVELOPMENT || DEBUG
/*/*
 * Statistics about UPL enforcement of copy-on-write obligations.
 */
unsigned long upl_cow = 0;
unsigned long upl_cow_again = 0;
unsigned long upl_cow_pages = 0;
unsigned long upl_cow_again_pages = 0;

unsigned long iopl_cow = 0;
unsigned long iopl_cow_pages = 0;
#endif

/*
 *  Routine:    vm_object_upl_request
 *  Purpose:
 *      Cause the population of a portion of a vm_object.
 *      Depending on the nature of the request, the pages
 *      returned may be contain valid data or be uninitialized.
 *      A page list structure, listing the physical pages
 *      will be returned upon request.
 *      This function is called by the file system or any other
 *      supplier of backing store to a pager.
 *      IMPORTANT NOTE: The caller must still respect the relationship
 *      between the vm_object and its backing memory object.  The
 *      caller MUST NOT substitute changes in the backing file
 *      without first doing a memory_object_lock_request on the
 *      target range unless it is know that the pages are not
 *      shared with another entity at the pager level.
 *      Copy_in_to:
 *          if a page list structure is present
 *          return the mapped physical pages, where a
 *          page is not present, return a non-initialized
 *          one.  If the no_sync bit is turned on, don't
 *          call the pager unlock to synchronize with other
 *          possible copies of the page. Leave pages busy
 *          in the original object, if a page list structure
 *          was specified.  When a commit of the page list
 *          pages is done, the dirty bit will be set for each one.
 *      Copy_out_from:
 *          If a page list structure is present, return
 *          all mapped pages.  Where a page does not exist
 *          map a zero filled one. Leave pages busy in
 *          the original object.  If a page list structure
 *          is not specified, this call is a no-op.
 *
 *      Note:  access of default pager objects has a rather interesting
 *      twist.  The caller of this routine, presumably the file system
 *      page cache handling code, will never actually make a request
 *      against a default pager backed object.  Only the default
 *      pager will make requests on backing store related vm_objects
 *      In this way the default pager can maintain the relationship
 *      between backing store files (abstract memory objects) and
 *      the vm_objects (cache objects), they support.
 *
 */

/* __private_extern__ */
extern kern_return_t
vm_object_upl_request(
    vm_object_t     object,
    vm_object_offset_t  offset,
    upl_size_t      size,
    upl_t           *upl_ptr,
    upl_page_info_array_t   user_page_list,
    unsigned int        *page_list_count,
    int         cntrl_flags)
{
        kprintf("not implemented: vm_object_upl_request()\n");
        return 0;
}

/* JMM - Backward compatability for now */
kern_return_t
vm_fault_list_request(          /* forward */
    memory_object_control_t     control,
    vm_object_offset_t  offset,
    upl_size_t      size,
    upl_t           *upl_ptr,
    upl_page_info_t     **user_page_list_ptr,
    unsigned int        page_list_count,
    int         cntrl_flags);

kern_return_t
vm_fault_list_request(
    memory_object_control_t     control,
    vm_object_offset_t  offset,
    upl_size_t      size,
    upl_t           *upl_ptr,
    upl_page_info_t     **user_page_list_ptr,
    unsigned int        page_list_count,
    int         cntrl_flags)
{
        kprintf("not implemented: vm_fault_list_request()\n");
        return 0;
}

/*
 *  Routine:    vm_object_super_upl_request
 *  Purpose:
 *      Cause the population of a portion of a vm_object
 *      in much the same way as memory_object_upl_request.
 *      Depending on the nature of the request, the pages
 *      returned may be contain valid data or be uninitialized.
 *      However, the region may be expanded up to the super
 *      cluster size provided.
 */

/* __private_extern__ */
extern kern_return_t
vm_object_super_upl_request(
    vm_object_t object,
    vm_object_offset_t  offset,
    upl_size_t      size,
    upl_size_t      super_cluster,
    upl_t           *upl,
    upl_page_info_t     *user_page_list,
    unsigned int        *page_list_count,
    int         cntrl_flags)
{
        kprintf("not implemented: vm_object_super_upl_request()\n");
        return 0;
}


kern_return_t
vm_map_create_upl(
    vm_map_t        map,
    vm_map_address_t    offset,
    upl_size_t      *upl_size,
    upl_t           *upl,
    upl_page_info_array_t   page_list,
    unsigned int        *count,
    int         *flags)
{
        kprintf("not implemented: vm_map_create_upl()\n");
        return 0;
}

/*
 * Internal routine to enter a UPL into a VM map.
 *
 * JMM - This should just be doable through the standard
 * vm_map_enter() API.
 */
kern_return_t
vm_map_enter_upl(
    vm_map_t        map,
    upl_t           upl,
    vm_map_offset_t     *dst_addr)
{
        kprintf("not implemented: vm_map_enter_upl()\n");
        return 0;
}

/*
 * Internal routine to remove a UPL mapping from a VM map.
 *
 * XXX - This should just be doable through a standard
 * vm_map_remove() operation.  Otherwise, implicit clean-up
 * of the target map won't be able to correctly remove
 * these (and release the reference on the UPL).  Having
 * to do this means we can't map these into user-space
 * maps yet.
 */
kern_return_t
vm_map_remove_upl(
    vm_map_t    map,
    upl_t       upl)
{
        kprintf("not implemented: vm_map_remove_upl()\n");
        return 0;
}


kern_return_t
upl_commit_range(
    upl_t           upl,
    upl_offset_t        offset,
    upl_size_t      size,
    int         flags,
    upl_page_info_t     *page_list,
    mach_msg_type_number_t  count,
    boolean_t       *empty)
{
        kprintf("not implemented: upl_commit_range()\n");
        return 0;
}

kern_return_t
upl_abort_range(
    upl_t           upl,
    upl_offset_t        offset,
    upl_size_t      size,
    int         error,
    boolean_t       *empty)
{
        kprintf("not implemented: upl_abort_range()\n");
        return 0;
}


kern_return_t
upl_abort(
    upl_t   upl,
    int error)
{
        kprintf("not implemented: upl_abort()\n");
        return 0;
}


/* an option on commit should be wire */
kern_return_t
upl_commit(
    upl_t           upl,
    upl_page_info_t     *page_list,
    mach_msg_type_number_t  count)
{
        kprintf("not implemented: upl_commit()\n");
        return 0;
}

void
vm_object_set_pmap_cache_attr(
        vm_object_t     object,
        upl_page_info_array_t   user_page_list,
        unsigned int        num_pages,
        boolean_t       batch_pmap_op)
{
        kprintf("not implemented: vm_object_set_pmap_cache_attr()\n");
}

unsigned int vm_object_iopl_request_sleep_for_cleaning = 0;

kern_return_t
vm_object_iopl_request(
    vm_object_t     object,
    vm_object_offset_t  offset,
    upl_size_t      size,
    upl_t           *upl_ptr,
    upl_page_info_array_t   user_page_list,
    unsigned int        *page_list_count,
    int         cntrl_flags)
{
        kprintf("not implemented: vm_object_iopl_request()\n");
        return 0;
}

kern_return_t
upl_transpose(
    upl_t       upl1,
    upl_t       upl2)
{
        kprintf("not implemented: upl_transpose()\n");
        return 0;
}

void
upl_range_needed(
    upl_t       upl,
    int     index,
    int     count)
{
        kprintf("not implemented: upl_range_needed()\n");
}


/*
 * ENCRYPTED SWAP:
 *
 * Rationale:  the user might have some encrypted data on disk (via
 * FileVault or any other mechanism).  That data is then decrypted in
 * memory, which is safe as long as the machine is secure.  But that
 * decrypted data in memory could be paged out to disk by the default
 * pager.  The data would then be stored on disk in clear (not encrypted)
 * and it could be accessed by anyone who gets physical access to the
 * disk (if the laptop or the disk gets stolen for example).  This weakens
 * the security offered by FileVault.
 *
 * Solution:  the default pager will optionally request that all the
 * pages it gathers for pageout be encrypted, via the UPL interfaces,
 * before it sends this UPL to disk via the vnode_pageout() path.
 *
 * Notes:
 *
 * To avoid disrupting the VM LRU algorithms, we want to keep the
 * clean-in-place mechanisms, which allow us to send some extra pages to
 * swap (clustering) without actually removing them from the user's
 * address space.  We don't want the user to unknowingly access encrypted
 * data, so we have to actually remove the encrypted pages from the page
 * table.  When the user accesses the data, the hardware will fail to
 * locate the virtual page in its page table and will trigger a page
 * fault.  We can then decrypt the page and enter it in the page table
 * again.  Whenever we allow the user to access the contents of a page,
 * we have to make sure it's not encrypted.
 *
 *
 */
/*
 * ENCRYPTED SWAP:
 * Reserve of virtual addresses in the kernel address space.
 * We need to map the physical pages in the kernel, so that we
 * can call the encryption/decryption routines with a kernel
 * virtual address.  We keep this pool of pre-allocated kernel
 * virtual addresses so that we don't have to scan the kernel's
 * virtaul address space each time we need to encrypt or decrypt
 * a physical page.
 * It would be nice to be able to encrypt and decrypt in physical
 * mode but that might not always be more efficient...
 */
decl_simple_lock_data(,vm_paging_lock)
#define VM_PAGING_NUM_PAGES 64
vm_map_offset_t vm_paging_base_address = 0;
boolean_t   vm_paging_page_inuse[VM_PAGING_NUM_PAGES] = { FALSE, };
int     vm_paging_max_index = 0;
int     vm_paging_page_waiter = 0;
int     vm_paging_page_waiter_total = 0;
unsigned long   vm_paging_no_kernel_page = 0;
unsigned long   vm_paging_objects_mapped = 0;
unsigned long   vm_paging_pages_mapped = 0;
unsigned long   vm_paging_objects_mapped_slow = 0;
unsigned long   vm_paging_pages_mapped_slow = 0;

void
vm_paging_map_init(void)
{
        kprintf("not implemented: vm_paging_map_init()\n");
}

/*
 * ENCRYPTED SWAP:
 * vm_paging_map_object:
 *  Maps part of a VM object's pages in the kernel
 *  virtual address space, using the pre-allocated
 *  kernel virtual addresses, if possible.
 * Context:
 *  The VM object is locked.  This lock will get
 *  dropped and re-acquired though, so the caller
 *  must make sure the VM object is kept alive
 *  (by holding a VM map that has a reference
 *  on it, for example, or taking an extra reference).
 *  The page should also be kept busy to prevent
 *  it from being reclaimed.
 */
kern_return_t
vm_paging_map_object(
    vm_map_offset_t     *address,
    vm_page_t       page,
    vm_object_t     object,
    vm_object_offset_t  offset,
    vm_map_size_t       *size,
    vm_prot_t       protection,
    boolean_t       can_unlock_object)
{
        kprintf("not implemented: vm_paging_map_object()\n");
        return 0;
}

/*
 * ENCRYPTED SWAP:
 * vm_paging_unmap_object:
 *  Unmaps part of a VM object's pages from the kernel
 *  virtual address space.
 * Context:
 *  The VM object is locked.  This lock will get
 *  dropped and re-acquired though.
 */
void
vm_paging_unmap_object(
    vm_object_t object,
    vm_map_offset_t start,
    vm_map_offset_t end)
{
        kprintf("not implemented: vm_paging_unmap_object()\n");
}

#if CRYPTO
/*
 * Encryption data.
 * "iv" is the "initial vector".  Ideally, we want to
 * have a different one for each page we encrypt, so that
 * crackers can't find encryption patterns too easily.
 */
#define SWAP_CRYPT_AES_KEY_SIZE 128 /* XXX 192 and 256 don't work ! */
boolean_t       swap_crypt_ctx_initialized = FALSE;
uint32_t        swap_crypt_key[8]; /* big enough for a 256 key */
aes_ctx         swap_crypt_ctx;
const unsigned char swap_crypt_null_iv[AES_BLOCK_SIZE] = {0xa, };

#if DEBUG
boolean_t       swap_crypt_ctx_tested = FALSE;
unsigned char swap_crypt_test_page_ref[4096] __attribute__((aligned(4096)));
unsigned char swap_crypt_test_page_encrypt[4096] __attribute__((aligned(4096)));
unsigned char swap_crypt_test_page_decrypt[4096] __attribute__((aligned(4096)));
#endif /* DEBUG */

/*
 * Initialize the encryption context: key and key size.
 */
void swap_crypt_ctx_initialize(void); /* forward */
void
swap_crypt_ctx_initialize(void)
{
        kprintf("not implemented: swap_crypt_ctx_initialize()\n");
}

/*
 * ENCRYPTED SWAP:
 * vm_page_encrypt:
 *  Encrypt the given page, for secure paging.
 *  The page might already be mapped at kernel virtual
 *  address "kernel_mapping_offset".  Otherwise, we need
 *  to map it.
 *
 * Context:
 *  The page's object is locked, but this lock will be released
 *  and re-acquired.
 *  The page is busy and not accessible by users (not entered in any pmap).
 */
void
vm_page_encrypt(
    vm_page_t   page,
    vm_map_offset_t kernel_mapping_offset)
{
        kprintf("not implemented: vm_page_encrypt()\n");
}

/*
 * ENCRYPTED SWAP:
 * vm_page_decrypt:
 *  Decrypt the given page.
 *  The page might already be mapped at kernel virtual
 *  address "kernel_mapping_offset".  Otherwise, we need
 *  to map it.
 *
 * Context:
 *  The page's VM object is locked but will be unlocked and relocked.
 *  The page is busy and not accessible by users (not entered in any pmap).
 */
void
vm_page_decrypt(
    vm_page_t   page,
    vm_map_offset_t kernel_mapping_offset)
{
        kprintf("not implemented: vm_page_decrypt()\n");
}

#if DEVELOPMENT || DEBUG
unsigned long upl_encrypt_upls = 0;
unsigned long upl_encrypt_pages = 0;
#endif

/*
 * ENCRYPTED SWAP:
 *
 * upl_encrypt:
 *  Encrypts all the pages in the UPL, within the specified range.
 *
 */
void
upl_encrypt(
    upl_t           upl,
    upl_offset_t        crypt_offset,
    upl_size_t      crypt_size)
{
        kprintf("not implemented: upl_encrypt()\n");
}

#else /* CRYPTO */
void
upl_encrypt(
    __unused upl_t          upl,
    __unused upl_offset_t   crypt_offset,
    __unused upl_size_t crypt_size)
{
        kprintf("not implemented: upl_encrypt()\n");
}

void
vm_page_encrypt(
    __unused vm_page_t      page,
    __unused vm_map_offset_t    kernel_mapping_offset)
{
        kprintf("not implemented: vm_page_encrypt()\n");
}

void
vm_page_decrypt(
    __unused vm_page_t      page,
    __unused vm_map_offset_t    kernel_mapping_offset)
{
        kprintf("not implemented: vm_page_decrypt()\n");
}

#endif /* CRYPTO */

/*
 * page->object must be locked
 */
void
vm_pageout_steal_laundry(vm_page_t page, boolean_t queues_locked)
{
        kprintf("not implemented: vm_pageout_steal_laundry()\n");
}

upl_t
vector_upl_create(vm_offset_t upl_offset)
{
        kprintf("not implemented: vector_upl_create()\n");
        return 0;
}

void
vector_upl_deallocate(upl_t upl)
{
        kprintf("not implemented: vector_upl_deallocate()\n");
}

boolean_t
vector_upl_is_valid(upl_t upl)
{
        kprintf("not implemented: vector_upl_is_valid()\n");
        return 0;
}

boolean_t
vector_upl_set_subupl(upl_t upl,upl_t subupl, uint32_t io_size)
{
        kprintf("not implemented: vector_upl_set_subupl()\n");
        return 0;
}

void
vector_upl_set_pagelist(upl_t upl)
{
        kprintf("not implemented: vector_upl_set_pagelist()\n");
}

upl_t
vector_upl_subupl_byoffset(upl_t upl, upl_offset_t *upl_offset, upl_size_t *upl_size)
{
        kprintf("not implemented: vector_upl_subupl_byoffset()\n");
        return 0;
}

void
vector_upl_get_submap(upl_t upl, vm_map_t *v_upl_submap, vm_offset_t *submap_dst_addr)
{
        kprintf("not implemented: vector_upl_get_submap()\n");
}

void
vector_upl_set_submap(upl_t upl, vm_map_t submap, vm_offset_t submap_dst_addr)
{
        kprintf("not implemented: vector_upl_set_submap()\n");
}

void
vector_upl_set_iostate(upl_t upl, upl_t subupl, upl_offset_t offset, upl_size_t size)
{
        kprintf("not implemented: vector_upl_set_iostate()\n");
}

void
vector_upl_get_iostate(upl_t upl, upl_t subupl, upl_offset_t *offset, upl_size_t *size)
{
        kprintf("not implemented: vector_upl_get_iostate()\n");
}

void
vector_upl_get_iostate_byindex(upl_t upl, uint32_t index, upl_offset_t *offset, upl_size_t *size)
{
        kprintf("not implemented: vector_upl_get_iostate_byindex()\n");
}

upl_page_info_t *
upl_get_internal_vectorupl_pagelist(upl_t upl)
{
        kprintf("not implemented: upl_get_internal_vectorupl_pagelist()\n");
        return 0;
}

void *
upl_get_internal_vectorupl(upl_t upl)
{
        kprintf("not implemented: upl_get_internal_vectorupl()\n");
        return 0;
}

vm_size_t
upl_get_internal_pagelist_offset(void)
{
        kprintf("not implemented: upl_get_internal_pagelist_offset()\n");
        return 0;
}

void
upl_clear_dirty(
    upl_t       upl,
    boolean_t   value)
{
        kprintf("not implemented: upl_clear_dirty()\n");
}

void
upl_set_referenced(
    upl_t       upl,
    boolean_t   value)
{
        kprintf("not implemented: upl_set_referenced()\n");
}

boolean_t
vm_page_is_slideable(vm_page_t m)
{
        kprintf("not implemented: vm_page_is_slideable()\n");
        return 0;
}

int vm_page_slide_counter = 0;
int vm_page_slide_errors = 0;
kern_return_t
vm_page_slide(
    vm_page_t   page,
    vm_map_offset_t kernel_mapping_offset)
{
        kprintf("not implemented: vm_page_slide()\n");
        return 0;
}


#ifdef MACH_BSD

boolean_t  upl_device_page(upl_page_info_t *upl)
{
        kprintf("not implemented: upl_device_page()\n");
        return 0;
}
boolean_t  upl_page_present(upl_page_info_t *upl, int index)
{
        kprintf("not implemented: upl_page_present()\n");
        return 0;
}
boolean_t  upl_speculative_page(upl_page_info_t *upl, int index)
{
        kprintf("not implemented: upl_speculative_page()\n");
        return 0;
}
boolean_t  upl_dirty_page(upl_page_info_t *upl, int index)
{
        kprintf("not implemented: upl_dirty_page()\n");
        return 0;
}
boolean_t  upl_valid_page(upl_page_info_t *upl, int index)
{
        kprintf("not implemented: upl_valid_page()\n");
        return 0;
}
ppnum_t  upl_phys_page(upl_page_info_t *upl, int index)
{
        kprintf("not implemented: upl_phys_page()\n");
        return 0;
}

void
vm_countdirtypages(void)
{
        kprintf("not implemented: vm_countdirtypages()\n");
}

#endif /* MACH_BSD */

upl_size_t upl_get_size(
                 upl_t          upl)
{
        kprintf("not implemented: upl_get_size()\n");
        return 0;
}

#if UPL_DEBUG
kern_return_t  upl_ubc_alias_set(upl_t upl, uintptr_t alias1, uintptr_t alias2)
{
        kprintf("not implemented: upl_ubc_alias_set()\n");
        return 0;
}
int  upl_ubc_alias_get(upl_t upl, uintptr_t * al, uintptr_t * al2)
{
        kprintf("not implemented: upl_ubc_alias_get()\n");
        return 0;
}
#endif /* UPL_DEBUG */
