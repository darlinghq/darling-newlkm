/*
 * Copyright (c) 1993-1995, 1999-2008 Apple Inc. All rights reserved.
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

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <mach/mach_types.h>
#include <mach/thread_act.h>

#include <kern/kern_types.h>
#include <kern/zalloc.h>
#include <kern/sched_prim.h>
#include <kern/clock.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/wait_queue.h>

#include <vm/vm_pageout.h>

#include <kern/thread_call.h>
#include <kern/call_entry.h>
#include <kern/timer_call.h>

#include <libkern/OSAtomic.h>

#include <sys/kdebug.h>
#if CONFIG_DTRACE
#include <mach/sdt.h>
#endif

static zone_t           thread_call_zone;
static struct wait_queue    daemon_wqueue;

struct thread_call_group {
    queue_head_t        pending_queue;
    uint32_t        pending_count;

    queue_head_t        delayed_queue;
    uint32_t        delayed_count;

    timer_call_data_t   delayed_timer;
    timer_call_data_t   dealloc_timer;

    struct wait_queue   idle_wqueue;
    uint32_t        idle_count, active_count;

    integer_t       pri;
    uint32_t        target_thread_count;
    uint64_t        idle_timestamp;

    uint32_t        flags;
    sched_call_t        sched_call;
};

typedef struct thread_call_group    *thread_call_group_t;

#define TCG_PARALLEL        0x01
#define TCG_DEALLOC_ACTIVE  0x02

#define THREAD_CALL_GROUP_COUNT     4
#define THREAD_CALL_THREAD_MIN      4
#define INTERNAL_CALL_COUNT     768
#define THREAD_CALL_DEALLOC_INTERVAL_NS (5 * 1000 * 1000) /* 5 ms */
#define THREAD_CALL_ADD_RATIO       4
#define THREAD_CALL_MACH_FACTOR_CAP 3

#define qe(x)       ((queue_entry_t)(x))
#define TC(x)       ((thread_call_t)(x))


lck_grp_t               thread_call_queues_lck_grp;
lck_grp_t               thread_call_lck_grp;
lck_attr_t              thread_call_lck_attr;
lck_grp_attr_t          thread_call_lck_grp_attr;

#if defined(__i386__) || defined(__x86_64__)
lck_mtx_t       thread_call_lock_data;
#else
lck_spin_t      thread_call_lock_data;
#endif

#define thread_call_lock_spin()         \
    lck_mtx_lock_spin_always(&thread_call_lock_data)

#define thread_call_unlock()            \
    lck_mtx_unlock_always(&thread_call_lock_data)

static inline spl_t
disable_ints_and_lock(void)
{
        kprintf("not implemented: disable_ints_and_lock()\n");
        return 0;
}

#if 0
/*
 *  thread_call_initialize:
 *
 *  Initialize this module, called
 *  early during system initialization.
 */
void
thread_call_initialize(void)
{
        kprintf("not implemented: thread_call_initialize()\n");
}

void
thread_call_setup(
    thread_call_t           call,
    thread_call_func_t      func,
    thread_call_param_t     param0)
{
        kprintf("not implemented: thread_call_setup()\n");
}
#endif

#ifndef __LP64__

/*
 *  thread_call_func:
 *
 *  Enqueue a function callout.
 *
 *  Guarantees { function, argument }
 *  uniqueness if unique_call is TRUE.
 */
void
thread_call_func(
    thread_call_func_t      func,
    thread_call_param_t     param,
    boolean_t               unique_call)
{
        kprintf("not implemented: thread_call_func()\n");
}

#endif  /* __LP64__ */

/*
 *  thread_call_func_delayed:
 *
 *  Enqueue a function callout to
 *  occur at the stated time.
 */
void
thread_call_func_delayed(
        thread_call_func_t      func,
        thread_call_param_t     param,
        uint64_t            deadline)
{
        kprintf("not implemented: thread_call_func_delayed()\n");
}

/*
 *  thread_call_func_cancel:
 *
 *  Dequeue a function callout.
 *
 *  Removes one (or all) { function, argument }
 *  instance(s) from either (or both)
 *  the pending and the delayed queue,
 *  in that order.
 *
 *  Returns TRUE if any calls were cancelled.
 */
boolean_t
thread_call_func_cancel(
        thread_call_func_t      func,
        thread_call_param_t     param,
        boolean_t           cancel_all)
{
        kprintf("not implemented: thread_call_func_cancel()\n");
        return 0;
}

/*
 * Allocate a thread call with a given priority.  Importances
 * other than THREAD_CALL_PRIORITY_HIGH will be run in threads
 * with eager preemption enabled (i.e. may be aggressively preempted
 * by higher-priority threads which are not in the normal "urgent" bands).
 */
thread_call_t
thread_call_allocate_with_priority(
        thread_call_func_t      func,
        thread_call_param_t     param0,
        thread_call_priority_t      pri)
{
        kprintf("not implemented: thread_call_allocate_with_priority()\n");
        return 0;
}

/*
 *  thread_call_allocate:
 *
 *  Allocate a callout entry.
 */
thread_call_t
thread_call_allocate(
        thread_call_func_t      func,
        thread_call_param_t     param0)
{
        kprintf("not implemented: thread_call_allocate()\n");
        return 0;
}

/*
 *  thread_call_free:
 *
 *  Release a callout.  If the callout is currently
 *  executing, it will be freed when all invocations
 *  finish.
 */
boolean_t
thread_call_free(
        thread_call_t       call)
{
        kprintf("not implemented: thread_call_free()\n");
        return 0;
}

/*
 *  thread_call_enter:
 *
 *  Enqueue a callout entry to occur "soon".
 *
 *  Returns TRUE if the call was
 *  already on a queue.
 */
boolean_t
thread_call_enter(
        thread_call_t       call)
{
        kprintf("not implemented: thread_call_enter()\n");
        return 0;
}

boolean_t
thread_call_enter1(
        thread_call_t           call,
        thread_call_param_t     param1)
{
        kprintf("not implemented: thread_call_enter1()\n");
        return 0;
}

/*
 *  thread_call_enter_delayed:
 *
 *  Enqueue a callout entry to occur
 *  at the stated time.
 *
 *  Returns TRUE if the call was
 *  already on a queue.
 */
#if 0
boolean_t
thread_call_enter_delayed(
        thread_call_t       call,
        uint64_t            deadline)
{
        kprintf("not implemented: thread_call_enter_delayed()\n");
        return 0;
}
#endif

boolean_t
thread_call_enter1_delayed(
        thread_call_t           call,
        thread_call_param_t     param1,
        uint64_t            deadline)
{
        kprintf("not implemented: thread_call_enter1_delayed()\n");
        return 0;
}

#if 0
/*
 *  thread_call_cancel:
 *
 *  Dequeue a callout entry.
 *
 *  Returns TRUE if the call was
 *  on a queue.
 */
boolean_t
thread_call_cancel(
        thread_call_t       call)
{
        kprintf("not implemented: thread_call_cancel()\n");
        return 0;
}
#endif

/*
 * Cancel a thread call.  If it cannot be cancelled (i.e.
 * is already in flight), waits for the most recent invocation
 * to finish.  Note that if clients re-submit this thread call,
 * it may still be pending or in flight when thread_call_cancel_wait
 * returns, but all requests to execute this work item prior
 * to the call to thread_call_cancel_wait will have finished.
 */
boolean_t
thread_call_cancel_wait(
        thread_call_t       call)
{
        kprintf("not implemented: thread_call_cancel_wait()\n");
        return 0;
}


#ifndef __LP64__

/*
 *  thread_call_is_delayed:
 *
 *  Returns TRUE if the call is
 *  currently on a delayed queue.
 *
 *  Optionally returns the expiration time.
 */
boolean_t
thread_call_is_delayed(
    thread_call_t       call,
    uint64_t            *deadline)
{
        kprintf("not implemented: thread_call_is_delayed()\n");
        return 0;
}

#endif  /* __LP64__ */

void
thread_call_delayed_timer(
        timer_call_param_t      p0,
        __unused timer_call_param_t p1
)
{
        kprintf("not implemented: thread_call_delayed_timer()\n");
}

/*
 * Determine whether a thread call is either on a queue or
 * currently being executed.
 */
boolean_t
thread_call_isactive(thread_call_t call)
{
        kprintf("not implemented: thread_call_isactive()\n");
        return 0;
}


