/*
 * Copyright (c) 2000-2010 Apple Inc. All rights reserved.
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
 * http://www.opensource.apple.com/apsl/ and read it before using this file.{
    return 0;
}

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
 * @OSF_FREE_COPYRIGHT@
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
 *  File:   kern/thread.c
 *  Author: Avadis Tevanian, Jr., Michael Wayne Young, David Golub
 *  Date:   1986
 *
 *  Thread management primitives implementation.
 */
/*
 * Copyright (c) 1993 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <kern/thread.h>
#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <mach/policy.h>
#include <mach/thread_info.h>
#include <mach/thread_special_ports.h>
#include <mach/thread_status.h>
#include <mach/time_value.h>
#include <mach/vm_param.h>

#include <machine/thread.h>
#include <machine/pal_routines.h>
#include <machine/limits.h>

#include <kern/kern_types.h>
#include <kern/kalloc.h>
#include <kern/cpu_data.h>
#include <kern/counters.h>
#include <kern/extmod_statistics.h>
#include <kern/ipc_mig.h>
#include <kern/ipc_tt.h>
#include <kern/mach_param.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/processor.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/sync_lock.h>
#include <kern/syscall_subr.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/host.h>
#include <kern/zalloc.h>
#include <kern/assert.h>

#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_port.h>

#include <vm/vm_kern.h>
#include <vm/vm_pageout.h>

#include <sys/kdebug.h>

#include <mach/sdt.h>

/*
 * Exported interfaces
 */
#include <mach/task_server.h>
#include <mach/thread_act_server.h>
#include <mach/mach_host_server.h>
#include <mach/host_priv_server.h>

static struct zone          *thread_zone;
static lck_grp_attr_t       thread_lck_grp_attr;
// lck_attr_t                  thread_lck_attr;
// lck_grp_t                   thread_lck_grp;

decl_simple_lock_data(static,thread_stack_lock)
static queue_head_t     thread_stack_queue;

decl_simple_lock_data(static,thread_terminate_lock)
static queue_head_t     thread_terminate_queue;

static struct thread    thread_template, init_thread;



struct _thread_ledger_indices thread_ledgers = { -1 };
static ledger_template_t thread_ledger_template = NULL;
void init_thread_ledgers(void);



/*
 *  thread_terminate_self:
 */
void
thread_terminate_self(void)
{
        kprintf("not implemented: thread_terminate_self()\n");
}

// void
// thread_deallocate(
//     thread_t            thread)
// {
//         kprintf("not implemented: thread_deallocate()\n");
// }



/*
 *  thread_terminate_enqueue:
 *
 *  Enqueue a terminating thread for final disposition.
 *
 *  Called at splsched.
 */
void
thread_terminate_enqueue(
    thread_t        thread)
{
        kprintf("not implemented: thread_terminate_enqueue()\n");
}




void
thread_stack_enqueue(
    thread_t        thread)
{
        kprintf("not implemented: thread_stack_enqueue()\n");
}

void
thread_daemon_init(void)
{
        kprintf("not implemented: thread_daemon_init()\n");
}

/*
 * Create a new thread.
 * Doesn't start the thread running.
 */
// static kern_return_t
// thread_create_internal(
//     task_t                  parent_task,
//     integer_t               priority,
//     thread_continue_t       continuation,
//     int                     options,
// #define TH_OPTION_NONE      0x00
// #define TH_OPTION_NOCRED    0x01
// #define TH_OPTION_NOSUSP    0x02
//     thread_t                *out_thread)
// {
//     return 0;
// }

// static kern_return_t
// thread_create_internal2(
//     task_t              task,
//     thread_t            *new_thread,
//     boolean_t           from_user)
// {
//     return 0;
// }

/* No prototype, since task_server.h has the _from_user version if KERNEL_SERVER */
// kern_return_t
// thread_create(
//     task_t              task,
//     thread_t            *new_thread);
//
// kern_return_t
// thread_create(
//     task_t              task,
//     thread_t            *new_thread)
// {
//         kprintf("not implemented: thread_create()\n");
//         return 0;
// }


kern_return_t
thread_create_from_user(
    task_t              task,
    thread_t            *new_thread)
{
        kprintf("not implemented: thread_create_from_user()\n");
        return 0;
}

// static kern_return_t
// thread_create_running_internal2(
//     register task_t         task,
//     int                     flavor,
//     thread_state_t          new_state,
//     mach_msg_type_number_t  new_state_count,
//     thread_t                *new_thread,
//     boolean_t               from_user)
// {
//     return 0;
// }

/* Prototype, see justification above */
kern_return_t
thread_create_running(
    register task_t         task,
    int                     flavor,
    thread_state_t          new_state,
    mach_msg_type_number_t  new_state_count,
    thread_t                *new_thread);

kern_return_t
thread_create_running(
    register task_t         task,
    int                     flavor,
    thread_state_t          new_state,
    mach_msg_type_number_t  new_state_count,
    thread_t                *new_thread)
{
        kprintf("not implemented: thread_create_running()\n");
        return 0;
}

kern_return_t
thread_create_running_from_user(
    register task_t         task,
    int                     flavor,
    thread_state_t          new_state,
    mach_msg_type_number_t  new_state_count,
    thread_t                *new_thread)
{
        kprintf("not implemented: thread_create_running_from_user()\n");
        return 0;
}

kern_return_t
thread_create_workq(
    task_t              task,
    thread_continue_t       thread_return,
    thread_t            *new_thread)
{
        kprintf("not implemented: thread_create_workq()\n");
        return 0;
}

/*
 *  kernel_thread_create:
 *
 *  Create a thread in the kernel task
 *  to execute in kernel context.
 */
kern_return_t
kernel_thread_create(
    thread_continue_t   continuation,
    void                *parameter,
    integer_t           priority,
    thread_t            *new_thread)
{
        kprintf("not implemented: kernel_thread_create()\n");
        return 0;
}

kern_return_t
kernel_thread_start_priority(
    thread_continue_t   continuation,
    void                *parameter,
    integer_t           priority,
    thread_t            *new_thread)
{
        kprintf("not implemented: kernel_thread_start_priority()\n");
        return 0;
}

kern_return_t
kernel_thread_start(
    thread_continue_t   continuation,
    void                *parameter,
    thread_t            *new_thread)
{
        kprintf("not implemented: kernel_thread_start()\n");
        return 0;
}

#if defined(__i386__)

thread_t
kernel_thread(
    task_t          task,
    void            (*start)(void))
{
        kprintf("not implemented: kernel_thread()\n");
        return 0;
}

#endif /* defined(__i386__) */

kern_return_t
thread_info_internal(
    register thread_t       thread,
    thread_flavor_t         flavor,
    thread_info_t           thread_info_out,    /* ptr to OUT array */
    mach_msg_type_number_t  *thread_info_count) /*IN/OUT*/
{
        kprintf("not implemented: thread_info_internal()\n");
        return 0;
}

void
thread_read_times(
    thread_t        thread,
    time_value_t    *user_time,
    time_value_t    *system_time)
{
        kprintf("not implemented: thread_read_times()\n");
}

kern_return_t
thread_assign(
    __unused thread_t           thread,
    __unused processor_set_t    new_pset)
{
        kprintf("not implemented: thread_assign()\n");
        return 0;
}

/*
 *  thread_assign_default:
 *
 *  Special version of thread_assign for assigning threads to default
 *  processor set.
 */
kern_return_t
thread_assign_default(
    thread_t        thread)
{
        kprintf("not implemented: thread_assign_default()\n");
        return 0;
}

/*
 *  thread_get_assignment
 *
 *  Return current assignment for this thread.
 */
kern_return_t
thread_get_assignment(
    thread_t        thread,
    processor_set_t *pset)
{
        kprintf("not implemented: thread_get_assignment()\n");
        return 0;
}

/*
 *  thread_wire_internal:
 *
 *  Specify that the target thread must always be able
 *  to run and to allocate memory.
 */
kern_return_t
thread_wire_internal(
    host_priv_t     host_priv,
    thread_t        thread,
    boolean_t       wired,
    boolean_t       *prev_state)
{
        kprintf("not implemented: thread_wire_internal()\n");
        return 0;
}


/*
 *  thread_wire:
 *
 *  User-api wrapper for thread_wire_internal()
 */
kern_return_t
thread_wire(
    host_priv_t host_priv,
    thread_t    thread,
    boolean_t   wired)
{
        kprintf("not implemented: thread_wire()\n");
        return 0;
}

// static void
// thread_resource_exception(const void *arg0, __unused const void *arg1)
// {
//     ;
// }

void
init_thread_ledgers(void)
{
        kprintf("not implemented: init_thread_ledgers()\n");
}

/*
 * Set CPU usage limit on a thread.
 *
 * Calling with percentage of 0 will unset the limit for this thread.
 */

int
thread_set_cpulimit(int action, uint8_t percentage, uint64_t interval_ns)
{
        kprintf("not implemented: thread_set_cpulimit()\n");
        return 0;
}

int     split_funnel_off = 0;
lck_grp_t   *funnel_lck_grp = LCK_GRP_NULL;
lck_grp_attr_t  *funnel_lck_grp_attr;
lck_attr_t  *funnel_lck_attr;

funnel_t *
funnel_alloc(
    int type)
{
        kprintf("not implemented: funnel_alloc()\n");
        return 0;
}

void
funnel_free(
    funnel_t * fnl)
{
        kprintf("not implemented: funnel_free()\n");
}

void
funnel_lock(
    funnel_t * fnl)
{
        kprintf("not implemented: funnel_lock()\n");
}

void
funnel_unlock(
    funnel_t * fnl)
{
        kprintf("not implemented: funnel_unlock()\n");
}

funnel_t *
thread_funnel_get(
    void)
{
        kprintf("not implemented: thread_funnel_get()\n");
        return 0;
}

boolean_t
thread_funnel_set(
        funnel_t *  fnl,
    boolean_t   funneled)
{
        kprintf("not implemented: thread_funnel_set()\n");
        return 0;
}

// static void
// sched_call_null(
// __unused    int         type,
// __unused    thread_t    thread)
// {
//     ;
// }

void
thread_sched_call(
    thread_t        thread,
    sched_call_t    call)
{
        kprintf("not implemented: thread_sched_call()\n");
}

void
thread_static_param(
    thread_t        thread,
    boolean_t       state)
{
        kprintf("not implemented: thread_static_param()\n");
}

uint64_t
thread_tid(
    thread_t    thread)
{
        kprintf("not implemented: thread_tid()\n");
        return 0;
}

uint16_t
thread_set_tag(thread_t th, uint16_t tag)
{
        kprintf("not implemented: thread_set_tag()\n");
        return 0;
}
uint16_t
thread_get_tag(thread_t th)
{
        kprintf("not implemented: thread_get_tag()\n");
        return 0;
}

uint64_t
thread_dispatchqaddr(
    thread_t        thread)
{
        kprintf("not implemented: thread_dispatchqaddr()\n");
        return 0;
}

/*
 * Export routines to other components for things that are done as macros
 * within the osfmk component.
 */

#undef thread_reference
void thread_reference(thread_t thread);
void
thread_reference(
    thread_t    thread)
{
        kprintf("not implemented: thread_reference()\n");
}

#undef thread_should_halt

boolean_t
thread_should_halt(
    thread_t        th)
{
        kprintf("not implemented: thread_should_halt()\n");
        return 0;
}

#if CONFIG_DTRACE
uint32_t dtrace_get_thread_predcache(thread_t thread)
{
        kprintf("not implemented: dtrace_get_thread_predcache()\n");
        return 0;
}

int64_t dtrace_get_thread_vtime(thread_t thread)
{
        kprintf("not implemented: dtrace_get_thread_vtime()\n");
        return 0;
}

int64_t dtrace_get_thread_tracing(thread_t thread)
{
        kprintf("not implemented: dtrace_get_thread_tracing()\n");
        return 0;
}

boolean_t dtrace_get_thread_reentering(thread_t thread)
{
        kprintf("not implemented: dtrace_get_thread_reentering()\n");
        return 0;
}

vm_offset_t dtrace_get_kernel_stack(thread_t thread)
{
        kprintf("not implemented: dtrace_get_kernel_stack()\n");
        return 0;
}

int64_t dtrace_calc_thread_recent_vtime(thread_t thread)
{
        kprintf("not implemented: dtrace_calc_thread_recent_vtime()\n");
        return 0;
}

void dtrace_set_thread_predcache(thread_t thread, uint32_t predcache)
{
        kprintf("not implemented: dtrace_set_thread_predcache()\n");
}

void dtrace_set_thread_vtime(thread_t thread, int64_t vtime)
{
        kprintf("not implemented: dtrace_set_thread_vtime()\n");
}

void dtrace_set_thread_tracing(thread_t thread, int64_t accum)
{
        kprintf("not implemented: dtrace_set_thread_tracing()\n");
}

void dtrace_set_thread_reentering(thread_t thread, boolean_t vbool)
{
        kprintf("not implemented: dtrace_set_thread_reentering()\n");
}

vm_offset_t dtrace_set_thread_recover(thread_t thread, vm_offset_t recover)
{
        kprintf("not implemented: dtrace_set_thread_recover()\n");
        return 0;
}

void dtrace_thread_bootstrap(void)
{
        kprintf("not implemented: dtrace_thread_bootstrap()\n");
}
#endif /* CONFIG_DTRACE */
