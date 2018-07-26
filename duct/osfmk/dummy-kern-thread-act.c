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
 * @OSF_FREE_COPYRIGHT@
 */
/*
 * Copyright (c) 1993 The University of Utah and
 * the Center for Software Science (CSS).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 *  Author: Bryan Ford, University of Utah CSS
 *
 *  Thread management routines
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>
#include <duct/duct_vm_map.h>

#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <mach/alert.h>
#include <mach/rpc.h>
#include <mach/thread_act_server.h>

#include <kern/kern_types.h>
#include <kern/ast.h>
#include <kern/mach_param.h>
#include <kern/zalloc.h>
#include <kern/extmod_statistics.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>
#include <kern/exception.h>
#include <kern/ipc_mig.h>
#include <kern/ipc_tt.h>
#include <kern/machine.h>
#include <kern/spl.h>
#include <kern/syscall_subr.h>
#include <kern/sync_lock.h>
#include <kern/processor.h>
#include <kern/timer.h>
#include <kern/affinity.h>

#include <mach/rpc.h>

#include <security/mac_mach_internal.h>

#include <asm/ptrace.h>
#include <asm/barrier.h>
#include <linux/thread_info.h>

void            act_abort(thread_t);
void            install_special_handler_locked(thread_t);
void            special_handler_continue(void);

/*
 * Internal routine to mark a thread as started.
 * Always called with the thread locked.
 *
 * Note: function intentionally declared with the noinline attribute to
 * prevent multiple declaration of probe symbols in this file; we would
 * prefer "#pragma noinline", but gcc does not support it.
 * PR-6385749 -- the lwp-start probe should fire from within the context
 * of the newly created thread.  Commented out for now, in case we
 * turn it into a dead code probe.
 */
void
thread_start_internal(
    thread_t            thread)
{
        kprintf("not implemented: thread_start_internal()\n");
}

/*
 * Internal routine to terminate a thread.
 * Sometimes called with task already locked.
 */
kern_return_t
thread_terminate_internal(
    thread_t            thread)
{
        kprintf("not implemented: thread_terminate_internal()\n");
        return 0;
}

/*
 * Terminate a thread.
 */
// use dummy for cpp files
#undef thread_terminate

kern_return_t
thread_terminate(
    thread_t        thread)
{
        kprintf("not implemented: thread_terminate()\n");
        return 0;
}

/*
 * Suspend execution of the specified thread.
 * This is a recursive-style suspension of the thread, a count of
 * suspends is maintained.
 *
 * Called with thread mutex held.
 */
void
thread_hold(
    register thread_t   thread)
{
        kprintf("not implemented: thread_hold()\n");
}

/*
 * Decrement internal suspension count, setting thread
 * runnable when count falls to zero.
 *
 * Called with thread mutex held.
 */
void
thread_release(
    register thread_t   thread)
{
        kprintf("not implemented: thread_release()\n");
}

kern_return_t
thread_suspend(
    register thread_t   thread)
{
	if (thread == THREAD_NULL)
	{
		return KERN_INVALID_ARGUMENT;
	}
	kprintf(KERN_DEBUG " - thread_suspend(): thread=%p, linux_task=%p\n", thread, thread->linux_task);
	
	smp_store_mb(thread->linux_task->state, TASK_STOPPED);
	// wake_up_process(thread->linux_task);
	thread->suspend_count = 1;

	return KERN_SUCCESS;
}

kern_return_t
thread_resume(
    register thread_t   thread)
{
	if (thread == THREAD_NULL)
	{
		return KERN_INVALID_ARGUMENT;
	}

	smp_store_mb(thread->linux_task->state, TASK_INTERRUPTIBLE);
	wake_up_process(thread->linux_task);
	thread->suspend_count = 0;

	return KERN_SUCCESS;
}

/*
 *  thread_depress_abort:
 *
 *  Prematurely abort priority depression if there is one.
 */
kern_return_t
thread_depress_abort(
    register thread_t   thread)
{
        kprintf("not implemented: thread_depress_abort()\n");
        return 0;
}


/*
 * Indicate that the activation should run its
 * special handler to detect a condition.
 *
 * Called with thread mutex held.
 */
void
act_abort(
    thread_t    thread)
{
        kprintf("not implemented: act_abort()\n");
}

kern_return_t
thread_abort(
    register thread_t   thread)
{
        kprintf("not implemented: thread_abort()\n");
        return 0;
}

kern_return_t
thread_abort_safely(
    thread_t        thread)
{
        kprintf("not implemented: thread_abort_safely()\n");
        return 0;
}

/*** backward compatibility hacks ***/
#include <mach/thread_info.h>
#include <mach/thread_special_ports.h>
#include <ipc/ipc_port.h>

extern kern_return_t
thread_set_state(
    register thread_t       thread,
    int                     flavor,
    thread_state_t          state,
    mach_msg_type_number_t  state_count);

kern_return_t
thread_set_state_from_user(
    register thread_t       thread,
    int                     flavor,
    thread_state_t          state,
    mach_msg_type_number_t  state_count)
{
	return thread_set_state(thread, flavor, state, state_count);
}

/*
 * Kernel-internal "thread" interfaces used outside this file:
 */

/* Initialize (or re-initialize) a thread state.  Called from execve
 * with nothing locked, returns same way.
 */
kern_return_t
thread_state_initialize(
    register thread_t       thread)
{
        kprintf("not implemented: thread_state_initialize()\n");
        return 0;
}


kern_return_t
thread_dup(
    register thread_t   target)
{
        kprintf("not implemented: thread_dup()\n");
        return 0;
}


/*
 *  thread_setstatus:
 *
 *  Set the status of the specified thread.
 *  Called with (and returns with) no locks held.
 */
kern_return_t
thread_setstatus(
    register thread_t       thread,
    int                     flavor,
    thread_state_t          tstate,
    mach_msg_type_number_t  count)
{
        kprintf("not implemented: thread_setstatus()\n");
        return 0;
}

/*
 *  thread_getstatus:
 *
 *  Get the status of the specified thread.
 */
kern_return_t
thread_getstatus(
    register thread_t       thread,
    int                     flavor,
    thread_state_t          tstate,
    mach_msg_type_number_t  *count)
{
        kprintf("not implemented: thread_getstatus()\n");
        return 0;
}

/*
 * install_special_handler:
 *
 *  Install the special returnhandler that handles suspension and
 *  termination, if it hasn't been installed already.
 *
 *  Called with the thread mutex held.
 */
void
install_special_handler(
    thread_t        thread)
{
        kprintf("not implemented: install_special_handler()\n");
}

/*
 * install_special_handler_locked:
 *
 *  Do the work of installing the special_handler.
 *
 *  Called with the thread mutex and scheduling lock held.
 */
void
install_special_handler_locked(
    thread_t                thread)
{
        kprintf("not implemented: install_special_handler_locked()\n");
}

/*
 * Activation control support routines internal to this file:
 */

void
act_execute_returnhandlers(void)
{
        kprintf("not implemented: act_execute_returnhandlers()\n");
}

/*
 * special_handler_continue
 *
 * Continuation routine for the special handler blocks.  It checks
 * to see whether there has been any new suspensions.  If so, it
 * installs the special handler again.  Otherwise, it checks to see
 * if the current depression needs to be re-instated (it may have
 * been temporarily removed in order to get to this point in a hurry).
 */
void
special_handler_continue(void)
{
        kprintf("not implemented: special_handler_continue()\n");
}

/*
 * special_handler  - handles suspension, termination.  Called
 * with nothing locked.  Returns (if it returns) the same way.
 */
void
special_handler(
    __unused ReturnHandler  *rh,
    thread_t                thread)
{
        kprintf("not implemented: special_handler()\n");
}

/* Prototype, see justification above */
kern_return_t
act_set_state(
    thread_t                thread,
    int                     flavor,
    thread_state_t          state,
    mach_msg_type_number_t  count);

kern_return_t
act_set_state(
    thread_t                thread,
    int                     flavor,
    thread_state_t          state,
    mach_msg_type_number_t  count)
{
        kprintf("not implemented: act_set_state()\n");
        return 0;
}

kern_return_t
act_set_state_from_user(
    thread_t                thread,
    int                     flavor,
    thread_state_t          state,
    mach_msg_type_number_t  count)
{
        kprintf("not implemented: act_set_state_from_user()\n");
        return 0;
}

kern_return_t
act_get_state(
    thread_t                thread,
    int                     flavor,
    thread_state_t          state,
    mach_msg_type_number_t  *count)
{
        kprintf("not implemented: act_get_state()\n");
        return 0;
}

// static void
// act_set_ast(
//         thread_t    thread,
//         ast_t ast)
// {
//     ;
// }

void
act_set_astbsd(
    thread_t    thread)
{
        kprintf("not implemented: act_set_astbsd()\n");
}

void
act_set_apc(
    thread_t    thread)
{
        kprintf("not implemented: act_set_apc()\n");
}

void
act_set_kperf(
    thread_t    thread)
{
        kprintf("not implemented: act_set_kperf()\n");
}

#if CONFIG_MACF
void
act_set_astmacf(
    thread_t    thread)
{
        kprintf("not implemented: act_set_astmacf()\n");
}
#endif
