/*
Copyright (c) 2014-2017, Wenqi Chen
Copyright (c) 2019-2020, Lubos Dolezel

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
#include "duct_kern_thread.h"
#include "duct_kern_task.h"
#include "duct_kern_zalloc.h"

#include <mach/mach_types.h>
#include <kern/mach_param.h>
#include <kern/thread.h>
#include <kern/ipc_tt.h>
#include <kern/policy_internal.h>

#include "duct_post_xnu.h"
#include <darling/task_registry.h>
#include <darling/debug_print.h>

// From pcb.c
#ifdef __x86_64__
unsigned int _MachineStateCount[] = {
	[x86_THREAD_STATE32]	= x86_THREAD_STATE32_COUNT,
	[x86_THREAD_STATE64]	= x86_THREAD_STATE64_COUNT,
	[x86_THREAD_STATE]	= x86_THREAD_STATE_COUNT,
	[x86_FLOAT_STATE32]	= x86_FLOAT_STATE32_COUNT,
	[x86_FLOAT_STATE64]	= x86_FLOAT_STATE64_COUNT,
	[x86_FLOAT_STATE]	= x86_FLOAT_STATE_COUNT,
	[x86_EXCEPTION_STATE32]	= x86_EXCEPTION_STATE32_COUNT,
	[x86_EXCEPTION_STATE64]	= x86_EXCEPTION_STATE64_COUNT,
	[x86_EXCEPTION_STATE]	= x86_EXCEPTION_STATE_COUNT,
	[x86_DEBUG_STATE32]	= x86_DEBUG_STATE32_COUNT,
	[x86_DEBUG_STATE64]	= x86_DEBUG_STATE64_COUNT,
	[x86_DEBUG_STATE]	= x86_DEBUG_STATE_COUNT,
	[x86_AVX_STATE32]	= x86_AVX_STATE32_COUNT,
	[x86_AVX_STATE64]	= x86_AVX_STATE64_COUNT,
	[x86_AVX_STATE]		= x86_AVX_STATE_COUNT,
};
#endif

#define LockTimeOutUsec 1000*500

static struct zone          *thread_zone;
static lck_grp_attr_t       thread_lck_grp_attr;
lck_attr_t                  thread_lck_attr;
lck_grp_t                   thread_lck_grp;

os_refgrp_decl(static, thread_refgrp, "thread", NULL);

// decl_simple_lock_data(static,thread_stack_lock)
// static queue_head_t     thread_stack_queue;
//
// decl_simple_lock_data(static,thread_terminate_lock)
// static queue_head_t     thread_terminate_queue;
//
static struct thread    thread_template, init_thread;
//
static void     sched_call_null(
                    int         type,
                    thread_t    thread)

{
    ;
}


//
// #ifdef MACH_BSD
// extern void proc_exit(void *);
// extern uint64_t get_dispatchqueue_offset_from_proc(void *);
// #endif /* MACH_BSD */
//
extern int debug_task;
int thread_max = CONFIG_THREAD_MAX; /* Max number of threads */
int task_threadmax = CONFIG_THREAD_MAX;

static uint64_t     thread_unique_id = 0;
//
// struct _thread_ledger_indices thread_ledgers = { -1 };
// static ledger_template_t thread_ledger_template = NULL;
// void init_thread_ledgers(void);

static kern_return_t duct_thread_create_internal (task_t parent_task, integer_t priority, thread_continue_t continuation, void* parameter, int options, thread_t * out_thread);
static kern_return_t duct_thread_create_internal2 (task_t task, thread_t * new_thread, boolean_t from_user, thread_continue_t continuation);
static void thread_deallocate_complete(thread_t thread);

kern_return_t duct_thread_terminate (thread_t thread);
void duct_thread_deallocate (thread_t thread);

void duct_thread_bootstrap (void)
{
	/*
	 *	Fill in a template thread for fast initialization.
	 */
	// Note for Darling, since static (and global) variables are always initialized to `0`,
	// we can avoid unnecessarily copying lots of code (stuff like `thread_template.<blah> = 0`)

	thread_template.runq = PROCESSOR_NULL;

	thread_template.reason = AST_NONE;
	thread_template.at_safe_point = FALSE;
	thread_template.wait_event = NO_EVENT64;
	thread_template.waitq = NULL;
	thread_template.wait_result = THREAD_WAITING;
	thread_template.options = THREAD_ABORTSAFE;
	thread_template.state = TH_WAIT | TH_UNINT;
	thread_template.wake_active = FALSE;
	thread_template.continuation = THREAD_CONTINUE_NULL;
	thread_template.parameter = NULL;

	thread_template.sched_mode = TH_MODE_NONE;
	thread_template.saved_mode = TH_MODE_NONE;
	thread_template.th_sched_bucket = TH_BUCKET_RUN;

	thread_template.sfi_class = SFI_CLASS_UNSPECIFIED;
	thread_template.sfi_wait_class = SFI_CLASS_UNSPECIFIED;

	thread_template.base_pri = BASEPRI_DEFAULT;
	thread_template.waiting_for_mutex = NULL;

	thread_template.realtime.deadline = UINT64_MAX;

	thread_template.last_made_runnable_time = THREAD_NOT_RUNNABLE;
	thread_template.last_basepri_change_time = THREAD_NOT_RUNNABLE;

#if defined(CONFIG_SCHED_TIMESHARE_CORE)
	thread_template.pri_shift = INT8_MAX;
#endif

	thread_template.bound_processor = PROCESSOR_NULL;
	thread_template.last_processor = PROCESSOR_NULL;

	thread_template.sched_call = NULL;

	thread_template.wait_timer_is_set = FALSE;

	thread_template.recover = (vm_offset_t)NULL;

	thread_template.map = VM_MAP_NULL;
#if DEVELOPMENT || DEBUG
	thread_template.pmap_footprint_suspended = FALSE;
#endif /* DEVELOPMENT || DEBUG */

#if KPC
	thread_template.kpc_buf = NULL;
#endif

#if HYPERVISOR
	thread_template.hv_thread_target = NULL;
#endif /* HYPERVISOR */

	thread_template.affinity_set = NULL;

	thread_template.t_ledger = LEDGER_NULL;
	thread_template.t_threadledger = LEDGER_NULL;
	thread_template.t_bankledger = LEDGER_NULL;

	thread_template.requested_policy = (struct thread_requested_policy) {};
	thread_template.effective_policy = (struct thread_effective_policy) {};

	bzero(&thread_template.overrides, sizeof(thread_template.overrides));

	thread_template.iotier_override = THROTTLE_LEVEL_NONE;
	thread_template.thread_io_stats = NULL;
#if CONFIG_EMBEDDED
	thread_template.taskwatch = NULL;
#endif /* CONFIG_EMBEDDED */

	thread_template.ith_voucher_name = MACH_PORT_NULL;
	thread_template.ith_voucher = IPC_VOUCHER_NULL;

	thread_template.th_work_interval = NULL;

	init_thread = thread_template;

	/* fiddle with init thread to skip asserts in set_sched_pri */
	init_thread.sched_pri = MAXPRI_KERNEL;

#warning Init thread initialization disabled!
#if 0
	linux_current->mach_thread = (void*)&init_thread;
	init_thread.linux_task = linux_current;
#endif
}


void duct_thread_init (void)
{
        thread_zone = duct_zinit(
                sizeof(struct thread),
                thread_max * sizeof(struct thread),
                THREAD_CHUNK * sizeof(struct thread),
                "threads");

        lck_grp_attr_setdefault(&thread_lck_grp_attr);
        lck_grp_init(&thread_lck_grp, "thread", &thread_lck_grp_attr);
        lck_attr_setdefault(&thread_lck_attr);

#if defined (__DARLING__)
#else
        stack_init();

        /*
         *    Initialize any machine-dependent
         *    per-thread structures necessary.
         */
        machine_thread_init();

        init_thread_ledgers();
#endif

}

#define TH_OPTION_NONE          0x00
#define TH_OPTION_NOCRED        0x01
#define TH_OPTION_NOSUSP        0x02
#define TH_OPTION_WORKQ         0x04

#ifdef current_thread
#undef current_thread
#endif

static kern_return_t duct_thread_create_internal (task_t parent_task, integer_t priority, thread_continue_t continuation, void* parameter, int options, thread_t * out_thread)
{
	thread_t new_thread;
	static thread_t first_thread = THREAD_NULL;

	/*
	 *	Allocate a thread and initialize static fields
	 */
#ifdef __DARLING__
	// we always allocate a thread because we don't create a kernel startup thread (maybe we should?)
	new_thread = (thread_t)duct_zalloc(thread_zone);
	if (first_thread == THREAD_NULL) {
		first_thread = new_thread;
	}
#else
	if (first_thread == THREAD_NULL) {
		new_thread = first_thread = current_thread();
	} else {
		new_thread = (thread_t)zalloc(thread_zone);
	}
#endif
	if (new_thread == THREAD_NULL) {
		return KERN_RESOURCE_SHORTAGE;
	}

#ifdef __DARLING__
	// like i said before, because we don't create a kernel thread on startup, this thread is always a normal thread, so we need to initialize it
	{
#else
	if (new_thread != first_thread) {
#endif
		*new_thread = thread_template;
	}

	os_ref_init_count(&new_thread->ref_count, &thread_refgrp, 2);

	new_thread->uthread = uthread_alloc(parent_task, new_thread, (options & TH_OPTION_NOCRED) != 0);

	new_thread->task = parent_task;

	// Darling addition
#ifdef MACH_ASSERT
	new_thread->thread_magic = THREAD_MAGIC;
#endif

	thread_lock_init(new_thread);

	lck_mtx_init(&new_thread->mutex, &thread_lck_grp, &thread_lck_attr);

	ipc_thread_init(new_thread);

	new_thread->continuation = continuation;
	new_thread->parameter = parameter;
	new_thread->inheritor_flags = TURNSTILE_UPDATE_FLAGS_NONE;
	priority_queue_init(&new_thread->sched_inheritor_queue, PRIORITY_QUEUE_BUILTIN_MAX_HEAP);
	priority_queue_init(&new_thread->base_inheritor_queue, PRIORITY_QUEUE_BUILTIN_MAX_HEAP);
#if CONFIG_SCHED_CLUTCH
	priority_queue_entry_init(&new_thread->sched_clutchpri_link);
#endif /* CONFIG_SCHED_CLUTCH */

	// do we still need this in Darling?
	/* Allocate I/O Statistics structure */
	new_thread->thread_io_stats = (io_stat_info_t)kalloc(sizeof(struct io_stat_info));
	assert(new_thread->thread_io_stats != NULL);
	bzero(new_thread->thread_io_stats, sizeof(struct io_stat_info));

#if CONFIG_IOSCHED
	/* Clear out the I/O Scheduling info for AppleFSCompression */
	new_thread->decmp_upl = NULL;
#endif /* CONFIG_IOSCHED */

	lck_mtx_lock(&tasks_threads_lock);
	task_lock(parent_task);

	/*
	 * Fail thread creation if parent task is being torn down or has too many threads
	 * If the caller asked for TH_OPTION_NOSUSP, also fail if the parent task is suspended
	 */
	if (parent_task->active == 0 || parent_task->halting ||
	    (parent_task->suspend_count > 0 && (options & TH_OPTION_NOSUSP) != 0) ||
	    (parent_task->thread_count >= task_threadmax && parent_task != kernel_task)) {
		task_unlock(parent_task);
		lck_mtx_unlock(&tasks_threads_lock);

		ipc_thread_disable(new_thread);
		ipc_thread_terminate(new_thread);
		kfree(new_thread->thread_io_stats, sizeof(struct io_stat_info));
		lck_mtx_destroy(&new_thread->mutex, &thread_lck_grp);
		zfree(thread_zone, new_thread);
		return KERN_FAILURE;
	}

	/* Protected by the tasks_threads_lock */
	new_thread->thread_id = ++thread_unique_id;

	task_reference_internal(parent_task);

#if defined(CONFIG_SCHED_MULTIQ)
	/* Cache the task's sched_group */
	new_thread->sched_group = parent_task->sched_group;
#endif /* defined(CONFIG_SCHED_MULTIQ) */

	/* Cache the task's map */
	new_thread->map = parent_task->map;

	timer_call_setup(&new_thread->wait_timer, thread_timer_expire, new_thread);

	/* Set the thread's scheduling parameters */
	new_thread->max_priority = parent_task->max_priority;
	new_thread->task_priority = parent_task->priority;
	int new_priority = (priority < 0) ? parent_task->priority: priority;
	new_priority = (priority < 0)? parent_task->priority: priority;
	if (new_priority > new_thread->max_priority) {
		new_priority = new_thread->max_priority;
	}
#if CONFIG_EMBEDDED
	if (new_priority < MAXPRI_THROTTLE) {
		new_priority = MAXPRI_THROTTLE;
	}
#endif /* CONFIG_EMBEDDED */
	new_thread->importance = new_priority - new_thread->task_priority;

	/* Chain the thread onto the task's list */
	queue_enter(&parent_task->threads, new_thread, thread_t, task_threads);
	parent_task->thread_count++;

	/* So terminating threads don't need to take the task lock to decrement */
	os_atomic_inc(&parent_task->active_thread_count, relaxed);

	new_thread->active = TRUE;
	new_thread->turnstile = turnstile_alloc();

	// Darling additions
	get_task_struct(linux_current);
	new_thread->linux_task = linux_current;
	new_thread->in_sigprocess = FALSE;

	*out_thread = new_thread;

	return KERN_SUCCESS;
}


static kern_return_t duct_thread_create_internal2 (task_t task, thread_t * new_thread, boolean_t from_user, thread_continue_t continuation)
{
        kern_return_t       result;
        thread_t            thread;

        if (task == TASK_NULL /*|| task == kernel_task*/) // Lubos: we need to register kthreads to do IPC from them
                return (KERN_INVALID_ARGUMENT);

        result  =
        duct_thread_create_internal (task, -1, continuation, NULL, TH_OPTION_NONE, &thread);
        if (result != KERN_SUCCESS)
                return (result);


        // thread->user_stop_count = 1;
        // thread_hold (thread);
        // if (task->suspend_count > 0)
        //     thread_hold(thread);
        //
        // if (from_user)
        //     extmod_statistics_incr_thread_create(task);
        //
        //task_unlock(task);
        lck_mtx_unlock(&tasks_threads_lock);

        *new_thread     = thread;

        return (KERN_SUCCESS);
}

kern_return_t duct_thread_create(task_t task, thread_t* new_thread) {
	return duct_thread_create_internal2(task, new_thread, FALSE, (thread_continue_t)thread_bootstrap_return);
}

kern_return_t duct_thread_create_from_user(task_t task, thread_t* new_thread) {
	return duct_thread_create_internal2(task, new_thread, TRUE, (thread_continue_t)thread_bootstrap_return);
}

kern_return_t duct_thread_create_with_continuation(task_t task, thread_t* new_thread, thread_continue_t continuation) {
	return duct_thread_create_internal2(task, new_thread, FALSE, continuation);
}

thread_t current_thread (void)
{
        // kprintf ("calling current thread on linux task: 0x%x\n", (unsigned int) linux_current);
		return darling_thread_get_current();
}

void duct_thread_destroy(thread_t thread)
{
	task_t task;
	task = thread->task;
	thread->linux_task = NULL;
	
	thread->active = FALSE;
	os_atomic_dec(&task->active_thread_count, relaxed);
	
	duct_thread_deallocate(thread);
}

static bool thread_ref_release(thread_t thread)
{
	if (thread == THREAD_NULL) {
		return false;
	}

	return os_ref_release(&thread->ref_count) == 0;
}

void
thread_deallocate_safe(thread_t thread)
{
	// for now; we can't really use the MPSC daemon until we implement `kernel_thread_create`, and thus, kernel threads in general
	return duct_thread_deallocate(thread);
}

void duct_thread_deallocate(thread_t thread)
{
	if (thread_ref_release(thread)) {
		thread_deallocate_complete(thread);
	}
}

static void thread_deallocate_complete(thread_t thread)
{
	task_t task;

	ipc_thread_terminate(thread);

	task = thread->task;

	if (thread->turnstile) {
		turnstile_deallocate(thread->turnstile);
	}

	if (IPC_VOUCHER_NULL != thread->ith_voucher) {
		ipc_voucher_release(thread->ith_voucher);
	}

	if (thread->thread_io_stats) {
		duct_kfree(thread->thread_io_stats, sizeof(struct io_stat_info));
	}

	task_lock(task);

	queue_remove(&task->threads, thread, thread_t, task_threads);
	task->thread_count--;

	task_deallocate(task);

	if (thread->linux_task != NULL)
		put_task_struct(thread->linux_task);

	debug_msg("Deallocating thread %p\n", thread);
	duct_zfree(thread_zone, thread);
}

struct task_struct* thread_get_linux_task(thread_t thread)
{
	return thread->linux_task;
}

void thread_timer_expire(void* p0, void* p1)
{
    thread_t thread = (thread_t) p0;
    thread_lock(thread);
    
    if (--thread->wait_timer_active == 0)
    {
        if (thread->wait_timer_is_set)
        {
            thread->wait_timer_is_set = FALSE;
	    printf("calling clear_wait\n");
            clear_wait_internal(thread, THREAD_TIMED_OUT);
        }
    }
    
    thread_unlock(thread);
}

kern_return_t thread_go(thread_t thread, wait_result_t wresult)
{
    return thread_unblock(thread, wresult);
}

kern_return_t thread_unblock(thread_t thread, wait_result_t wresult)
{
    thread->wait_result = wresult;

    thread->state &= ~(TH_WAIT|TH_UNINT);

	if (!(thread->state & TH_RUN))
		thread->state |= TH_RUN;
    
    if (thread->wait_timer_is_set)
    {
        if (timer_call_cancel(&thread->wait_timer))
            thread->wait_timer_active--;
        thread->wait_timer_is_set = FALSE;
    }
 
    struct task_struct* t = thread_get_linux_task(thread);
    if (t != NULL)
    {
        wake_up_process(t);
        return KERN_SUCCESS;
    }

    return KERN_FAILURE;
}

#define current linux_current

wait_result_t thread_mark_wait_locked(thread_t thread, wait_interrupt_t interruptible)
{
    if (/*interruptible == THREAD_UNINT || !signal_pending(linux_current)*/ 1)
    {
        thread->state &= TH_RUN;
        thread->state |= TH_WAIT;

        if (interruptible == THREAD_UNINT)
            thread->state |= TH_UNINT;

        // printf("thread_mark_wait_locked - unint? %d\n", interruptible == THREAD_UNINT);
        thread_interrupt_level(interruptible);
        return thread->wait_result = THREAD_WAITING;
    }

    thread->wait_result = THREAD_INTERRUPTED;
    clear_wait_internal(thread, THREAD_INTERRUPTED);

    //if (thread->waitq != NULL)
    //    duct_panic("thread->waitq is NOT NULL in thread_mark_wait_locked");

    return thread->wait_result;
}


wait_result_t
thread_block_parameter(
	thread_continue_t	cont,
	void				*parameter)
{
    thread_t thread = current_thread();

    thread_lock(thread);
    thread->parameter = parameter;
    
    while ((thread->state & TH_WAIT) && linux_current->state != TASK_RUNNING)
    {
        thread_unlock(thread);
	printf("about to schedule - my state: %d\n", thread->linux_task->state);
        schedule();
        thread_lock(thread);

        if (signal_pending(linux_current))
            break;
    }

    if (thread->wait_result == THREAD_WAITING)
        clear_wait_internal(thread, THREAD_INTERRUPTED);
    
    thread_unlock(thread);

    if (thread->waitq != NULL)
        duct_panic("thread->waitq is NOT NULL in thread_block_parameter");

    if (cont != THREAD_CONTINUE_NULL)
    {
        cont(thread->parameter, thread->wait_result);
        panic("thread_block: continuation isn't supposed to return!");
    }
    
    return thread->wait_result;
}

wait_result_t thread_block(thread_continue_t cont)
{
    return thread_block_parameter(cont, NULL);
}

// COPED FROM kern/sched_prim.c START

__private_extern__ kern_return_t
clear_wait_internal(
	thread_t		thread,
	wait_result_t	wresult)
{
	uint32_t	i = LockTimeOutUsec;
	struct waitq *waitq = thread->waitq;
	
	do {
#ifndef DARLING // We don't maintain the value of thread->state
		if (wresult == THREAD_INTERRUPTED && (thread->state & TH_UNINT))
			return (KERN_FAILURE);
#endif

		if (waitq != NULL) {
			if (!waitq_pull_thread_locked(waitq, thread)) {
				thread_unlock(thread);
				delay(1);
				if (i > 0 && !machine_timeout_suspended())
					i--;
				thread_lock(thread);
				if (waitq != thread->waitq)
					return KERN_NOT_WAITING;
				continue;
			}
		}

		/* TODO: Can we instead assert TH_TERMINATE is not set?  */
//		if ((thread->state & (TH_WAIT|TH_TERMINATE)) == TH_WAIT)
			return (thread_go(thread, wresult));
//		else
//			return (KERN_NOT_WAITING);
	} while (i > 0);

	panic("clear_wait_internal: deadlock: thread=%p, wq=%p, cpu=%d\n",
		  thread, waitq, cpu_number());

	return (KERN_FAILURE);
}


/*
 *	clear_wait:
 *
 *	Clear the wait condition for the specified thread.  Start the thread
 *	executing if that is appropriate.
 *
 *	parameters:
 *	  thread		thread to awaken
 *	  result		Wakeup result the thread should see
 */
kern_return_t
clear_wait(
	thread_t		thread,
	wait_result_t	result)
{
	kern_return_t ret;
	spl_t		s;

	s = splsched();
	thread_lock(thread);
	ret = clear_wait_internal(thread, result);
	thread_unlock(thread);
	splx(s);
	return ret;
}

kern_return_t
thread_wakeup_one_with_pri(
                           event_t      event,
                           int          priority)
{
	if (__improbable(event == NO_EVENT))
		panic("%s() called with NO_EVENT", __func__);

	struct waitq *wq = global_eventq(event);

	return waitq_wakeup64_one(wq, CAST_EVENT64_T(event), THREAD_AWAKENED, priority);
}

kern_return_t
thread_wakeup_prim(
                   event_t          event,
                   boolean_t        one_thread,
                   wait_result_t    result)
{
	if (__improbable(event == NO_EVENT))
		panic("%s() called with NO_EVENT", __func__);

	struct waitq *wq = global_eventq(event);

	if (one_thread)
		return waitq_wakeup64_one(wq, CAST_EVENT64_T(event), result, WAITQ_ALL_PRIORITIES);
	else
		return waitq_wakeup64_all(wq, CAST_EVENT64_T(event), result, WAITQ_ALL_PRIORITIES);
}

wait_result_t
assert_wait_deadline(
	event_t				event,
	wait_interrupt_t	interruptible,
	uint64_t			deadline)
{
	thread_t			thread = current_thread();
	wait_result_t		wresult;
	spl_t				s;
	printf("assert wait with deadline %d\n", deadline);

	if (__improbable(event == NO_EVENT))
		panic("%s() called with NO_EVENT", __func__);

	struct waitq *waitq;
	waitq = global_eventq(event);

	s = splsched();
	waitq_lock(waitq);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
				  MACHDBG_CODE(DBG_MACH_SCHED, MACH_WAIT)|DBG_FUNC_NONE,
				  VM_KERNEL_UNSLIDE_OR_PERM(event), interruptible, deadline, 0, 0);

	wresult = waitq_assert_wait64_locked(waitq, CAST_EVENT64_T(event),
					     interruptible,
					     TIMEOUT_URGENCY_SYS_NORMAL, deadline,
					     TIMEOUT_NO_LEEWAY, thread);
	waitq_unlock(waitq);
	splx(s);
	return wresult;
}

wait_result_t
assert_wait(
	event_t				event,
	wait_interrupt_t	interruptible)
{
	if (__improbable(event == NO_EVENT))
		panic("%s() called with NO_EVENT", __func__);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		MACHDBG_CODE(DBG_MACH_SCHED, MACH_WAIT)|DBG_FUNC_NONE,
		VM_KERNEL_UNSLIDE_OR_PERM(event), 0, 0, 0, 0);

	struct waitq *waitq;
	waitq = global_eventq(event);
	return waitq_assert_wait64(waitq, CAST_EVENT64_T(event), interruptible, TIMEOUT_WAIT_FOREVER);
}

// COPED FROM kern/sched_prim.c END

wait_interrupt_t
thread_interrupt_level(
	wait_interrupt_t new_level)
{
    int rv = (linux_current->state & TASK_UNINTERRUPTIBLE) ? THREAD_UNINT : THREAD_INTERRUPTIBLE;
    set_current_state(new_level == THREAD_UNINT ? TASK_KILLABLE : TASK_INTERRUPTIBLE);
    return rv;
}

void thread_exception_return(void)
{
    printf("thread_exception_return called!\n");
}

void thread_bootstrap_return(void) {
	kprintf("thread_bootstrap_return called!\n");
	// we should probably panic when these (i.e. this and `thread_exception_return`) are called?
};

#undef current

kern_return_t 
thread_set_mach_voucher(
	thread_t		thread,
	ipc_voucher_t		voucher)
{
    printf("NOT IMPLEMENTED: thread_set_mach_voucher\n");
	return KERN_NOT_SUPPORTED;
}

kern_return_t 
thread_get_mach_voucher(
	thread_act_t		thread,
	mach_voucher_selector_t which,
	ipc_voucher_t		*voucherp)
{
    printf("NOT IMPLEMENTED: thread_get_mach_voucher\n");
	return KERN_NOT_SUPPORTED;
}

kern_return_t
thread_swap_mach_voucher(
	thread_t		thread,
	ipc_voucher_t		new_voucher,
	ipc_voucher_t		*in_out_old_voucher)
{
    printf("NOT IMPLEMENTED: thread_swap_mach_voucher\n");
	return KERN_NOT_SUPPORTED;
}

uint64_t
thread_dispatchqaddr(
    thread_t        thread)
{
    return thread->dispatch_qaddr;
}

#if 0
kern_return_t thread_set_cthread_self (uint32_t cthread)
{
        thread_t    thread      = current_thread ();

        assert (thread);
        thread->machine.cthread_self    = cthread;

        /* Linux's restores p15, c13, c0, 3 from tp_value upon context switching so
         * better let Linux aware */
        current_thread_info ()->tp_value = cthread;

        // arm_set_threadpid_user_readonly ((uint32_t *) curthr->machine.cthread_self);

        __asm__ __volatile__ ("mcr p15, 0, %0, c13, c0, 3"::"r"(thread->machine.cthread_self));
        return KERN_SUCCESS;
}

/* WC - todo should pass in argsptr (like mach traps) rather than args */
// WC - should not be here
#if defined (XNU_USE_MACHTRAP_WRAPPERS_THREAD)
kern_return_t xnusys_thread_set_cthread_self (uint32_t cthread)
{
        printk (KERN_NOTICE "- cthread: 0x%x\n", cthread);

        return thread_set_cthread_self (cthread);
}
#endif
#endif

// <copied from="xnu://6153.61.1/osfmk/kern/thread.c">

void
thread_inspect_deallocate(
	thread_inspect_t                thread_inspect)
{
	return thread_deallocate((thread_t)thread_inspect);
}

// </copied>
