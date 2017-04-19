/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2015-2017 Lubos Dolezel
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <duct/duct.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/timer.h>
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
#include <darling/debug_print.h>
#include <darling/task_registry.h>

static struct workqueue_struct* _thread_call_wq = NULL;

static void
thread_call_worker(struct work_struct* work);

static void
thread_call_wq_init(struct work_struct* work);
static void
thread_call_wq_exit(struct work_struct* work);

extern kern_return_t
duct_thread_create(task_t task, thread_t* new_thread);
extern void
duct_thread_destroy(thread_t thread);

extern task_t kernel_task;

/*
 *  thread_call_initialize:
 *
 *  Initialize this module, called
 *  early during system initialization.
 */
void
thread_call_initialize(void)
{
	struct work_struct ws;

	assert(_thread_call_wq == NULL);
	_thread_call_wq = create_singlethread_workqueue("mach_thread_call");

	INIT_WORK(&ws, thread_call_wq_init);
	queue_work(_thread_call_wq, &ws);
	flush_work(&ws);
}

void
thread_call_deinitialize(void)
{
	struct work_struct ws;

	assert(_thread_call_wq != NULL);

	INIT_WORK(&ws, thread_call_wq_exit);
	queue_work(_thread_call_wq, &ws);
	flush_work(&ws);

	destroy_workqueue(_thread_call_wq);
	_thread_call_wq = NULL;
}

static void
thread_call_wq_init(struct work_struct* work)
{
	// Register self as a XNU thread, so that IPC works from this thread
	thread_t thread;
	duct_thread_create(kernel_task, &thread);
	thread_deallocate(thread);
	
	darling_thread_register(thread);
}

static void
thread_call_wq_exit(struct work_struct* work)
{
	thread_t thread = darling_thread_get_current();
	if (thread != NULL)
	{
		duct_thread_destroy(thread);
		darling_thread_deregister(thread);
	}
}

void
thread_call_setup(
    thread_call_t           call,
    thread_call_func_t      func,
    thread_call_param_t     param0)
{
	debug_msg("thread_call_setup(call=%p)\n", call);
	call_entry_setup((call_entry_t) call, func, param0);
	INIT_DELAYED_WORK(&call->tc_work, thread_call_worker);
}

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
	assert(_thread_call_wq != NULL);
	debug_msg("thread_call_cancel(%p)\n", call);
	return cancel_delayed_work(&call->tc_work);
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
boolean_t
thread_call_enter_delayed(
        thread_call_t       call,
        uint64_t            deadline)
{
	struct linux_timespec now;
	int64_t diffns;
	uint64_t arm_time;

	debug_msg("thread_call_enter_delayed(call=%p, deadline=%lld)\n", call, deadline);
	assert(_thread_call_wq != NULL);

	ktime_get_ts(&now);

	arm_time = now.tv_nsec + now.tv_sec * NSEC_PER_SEC;
	diffns = deadline - arm_time;

	if (diffns < 0)
		diffns = 0;

	debug_msg("... delayed by %lld ns\n", diffns);

	return queue_delayed_work(_thread_call_wq, &call->tc_work, nsecs_to_jiffies(diffns)) == 0;
}

static void
thread_call_worker(struct work_struct* work)
{
	thread_call_t call;

	debug_msg("thread_call_worker() invoked\n");
	call = container_of(to_delayed_work(work), struct thread_call, tc_work);
	
	call->tc_call.func(call->tc_call.param0, call->tc_call.param1);
}

