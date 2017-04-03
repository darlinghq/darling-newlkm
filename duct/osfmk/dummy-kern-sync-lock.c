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
 *
 */
/*
 *  File:   kern/sync_lock.c
 *  Author: Joseph CaraDonna
 *
 *  Contains RT distributed lock synchronization services.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <mach/mach_types.h>
#include <mach/lock_set_server.h>
#include <mach/task_server.h>

#include <kern/misc_protos.h>
#include <kern/kalloc.h>
#include <kern/sync_lock.h>
#include <kern/sched_prim.h>
#include <kern/ipc_kobject.h>
#include <kern/ipc_sync.h>
#include <kern/thread.h>
#include <kern/task.h>

#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>
#include <libkern/OSAtomic.h>

/*
 *  Ulock ownership MACROS
 *
 *  Assumes: ulock internal lock is held
 */

#define ulock_ownership_set(ul, th)             \
    MACRO_BEGIN                     \
    thread_mtx_lock(th);                    \
    enqueue (&th->held_ulocks, (queue_entry_t) (ul));  \
    thread_mtx_unlock(th);                  \
    (ul)->holder = th;                  \
    MACRO_END

#define ulock_ownership_clear(ul)               \
    MACRO_BEGIN                     \
    thread_t th;                        \
    th = (ul)->holder;                  \
        if ((th)->active) {                 \
        thread_mtx_lock(th);                \
        remqueue((queue_entry_t) (ul));     \
        thread_mtx_unlock(th);              \
    } else {                        \
        remqueue((queue_entry_t) (ul));     \
    }                           \
    (ul)->holder = THREAD_NULL;             \
    MACRO_END

/*
 *  Lock set ownership MACROS
 */

#define lock_set_ownership_set(ls, t)               \
    MACRO_BEGIN                     \
    task_lock((t));                     \
    enqueue_head(&(t)->lock_set_list, (queue_entry_t) (ls));\
    (t)->lock_sets_owned++;                 \
    task_unlock((t));                   \
    (ls)->owner = (t);                  \
    MACRO_END

#define lock_set_ownership_clear(ls, t)             \
    MACRO_BEGIN                     \
    task_lock((t));                     \
    remqueue((queue_entry_t) (ls)); \
    (t)->lock_sets_owned--;                 \
    task_unlock((t));                   \
    MACRO_END

unsigned int lock_set_event;
#define LOCK_SET_EVENT CAST_EVENT64_T(&lock_set_event)

unsigned int lock_set_handoff;
#define LOCK_SET_HANDOFF CAST_EVENT64_T(&lock_set_handoff)


lck_attr_t              lock_set_attr;
lck_grp_t               lock_set_grp;
static lck_grp_attr_t   lock_set_grp_attr;

/*
 *  ROUTINE:    lock_set_init       [private]
 *
 *  Initialize the lock_set subsystem.
 */
void
lock_set_init(void)
{
        kprintf("not implemented: lock_set_init()\n");
}

/*
 *  ROUTINE:    lock_set_create     [exported]
 *
 *  Creates a lock set.
 *  The port representing the lock set is returned as a parameter.
 */
kern_return_t
lock_set_create (
    task_t      task,
    lock_set_t  *new_lock_set,
    int     n_ulocks,
    int     policy)
{
        kprintf("not implemented: lock_set_create()\n");
        return 0;
}

/*
 *  ROUTINE:    lock_set_destroy    [exported]
 *
 *  Destroys a lock set.  This call will only succeed if the
 *  specified task is the SAME task name specified at the lock set's
 *  creation.
 *
 *  NOTES:
 *  - All threads currently blocked on the lock set's ulocks are awoken.
 *  - These threads will return with the KERN_LOCK_SET_DESTROYED error.
 */
kern_return_t
lock_set_destroy (task_t task, lock_set_t lock_set)
{
        kprintf("not implemented: lock_set_destroy()\n");
        return 0;
}
kern_return_t
lock_acquire (lock_set_t lock_set, int lock_id)
{
        kprintf("not implemented: lock_acquire()\n");
        return 0;
}

kern_return_t
lock_release (lock_set_t lock_set, int lock_id)
{
        kprintf("not implemented: lock_release()\n");
        return 0;
}

kern_return_t
lock_try (lock_set_t lock_set, int lock_id)
{
        kprintf("not implemented: lock_try()\n");
        return 0;
}

kern_return_t
lock_make_stable (lock_set_t lock_set, int lock_id)
{
        kprintf("not implemented: lock_make_stable()\n");
        return 0;
}

/*
 *  ROUTINE:    lock_make_unstable  [internal]
 *
 *  Marks the lock as unstable.
 *
 *  NOTES:
 *  - All future acquisitions of the lock will return with a
 *    KERN_LOCK_UNSTABLE status, until the lock is made stable again.
 */
kern_return_t
lock_make_unstable (ulock_t ulock, thread_t thread)
{
        kprintf("not implemented: lock_make_unstable()\n");
        return 0;
}

/*
 *  ROUTINE:    ulock_release_internal  [internal]
 *
 *  Releases the ulock.
 *  If any threads are blocked waiting for the ulock, one is woken-up.
 *
 */
kern_return_t
ulock_release_internal (ulock_t ulock, thread_t thread)
{
        kprintf("not implemented: ulock_release_internal()\n");
        return 0;
}

kern_return_t
lock_handoff (lock_set_t lock_set, int lock_id)
{
        kprintf("not implemented: lock_handoff()\n");
        return 0;
}

kern_return_t
lock_handoff_accept (lock_set_t lock_set, int lock_id)
{
        kprintf("not implemented: lock_handoff_accept()\n");
        return 0;
}

/*
 *  Routine:    lock_set_reference
 *
 *  Take out a reference on a lock set.  This keeps the data structure
 *  in existence (but the lock set may be deactivated).
 */
void
lock_set_reference(lock_set_t lock_set)
{
        kprintf("not implemented: lock_set_reference()\n");
}

/*
 *  Routine:    lock_set_dereference
 *
 *  Release a reference on a lock set.  If this is the last reference,
 *  the lock set data structure is deallocated.
 */
void
lock_set_dereference(lock_set_t lock_set)
{
        kprintf("not implemented: lock_set_dereference()\n");
}

void
ulock_release_all(
    thread_t        thread)
{
        kprintf("not implemented: ulock_release_all()\n");
}
