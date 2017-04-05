/*
Copyright (c) 2014-2017, Wenqi Chen

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

#include "duct_post_xnu.h"
#include <linux/task_registry.h>

static struct zone          *thread_zone;
static lck_grp_attr_t       thread_lck_grp_attr;
lck_attr_t                  thread_lck_attr;
lck_grp_t                   thread_lck_grp;

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

static kern_return_t duct_thread_create_internal (task_t parent_task, integer_t priority, thread_continue_t continuation, int options, thread_t * out_thread);
static kern_return_t duct_thread_create_internal2 (task_t task, thread_t * new_thread, boolean_t from_user);

kern_return_t duct_thread_terminate (thread_t thread);
void duct_thread_deallocate (thread_t thread);

void duct_thread_bootstrap (void)
{
        /*
         *    Fill in a template thread for fast initialization.
         */
        // WC - the following is not necessary

        thread_template.runq = PROCESSOR_NULL;

        thread_template.ref_count = 2;

        thread_template.reason = 0;
        thread_template.at_safe_point = FALSE;
        thread_template.wait_event = NO_EVENT64;
        thread_template.wait_queue = WAIT_QUEUE_NULL;
        thread_template.wait_result = THREAD_WAITING;
        thread_template.options = THREAD_ABORTSAFE;
        thread_template.state = TH_WAIT | TH_UNINT;
        thread_template.wake_active = FALSE;
        thread_template.continuation = THREAD_CONTINUE_NULL;
        thread_template.parameter = NULL;

        thread_template.importance = 0;
        thread_template.sched_mode = TH_MODE_NONE;
        thread_template.sched_flags = 0;
        thread_template.saved_mode = TH_MODE_NONE;
        thread_template.safe_release = 0;

        thread_template.priority = 0;
        thread_template.sched_pri = 0;
        thread_template.max_priority = 0;
        thread_template.task_priority = 0;
        thread_template.promotions = 0;
        thread_template.pending_promoter_index = 0;
        thread_template.pending_promoter[0] =
        thread_template.pending_promoter[1] = NULL;

        thread_template.realtime.deadline = UINT64_MAX;

        thread_template.current_quantum = 0;
        thread_template.last_run_time = 0;
        thread_template.last_quantum_refill_time = 0;

        thread_template.computation_metered = 0;
        thread_template.computation_epoch = 0;

    #if defined(CONFIG_SCHED_TRADITIONAL)
        thread_template.sched_stamp = 0;
        thread_template.pri_shift = INT8_MAX;
        thread_template.sched_usage = 0;
        thread_template.cpu_usage = thread_template.cpu_delta = 0;
    #endif
        thread_template.c_switch = thread_template.p_switch = thread_template.ps_switch = 0;

        thread_template.bound_processor = PROCESSOR_NULL;
        thread_template.last_processor = PROCESSOR_NULL;

        thread_template.sched_call = sched_call_null;

    #if defined (__DARLING__)
    #else
        timer_init(&thread_template.user_timer);
        timer_init(&thread_template.system_timer);
    #endif

        thread_template.user_timer_save = 0;
        thread_template.system_timer_save = 0;
        thread_template.vtimer_user_save = 0;
        thread_template.vtimer_prof_save = 0;
        thread_template.vtimer_rlim_save = 0;

        thread_template.wait_timer_is_set = FALSE;
        thread_template.wait_timer_active = 0;

        thread_template.depress_timer_active = 0;

        thread_template.special_handler.handler = special_handler;
        thread_template.special_handler.next = NULL;

        thread_template.funnel_lock = THR_FUNNEL_NULL;
        thread_template.funnel_state = 0;
        thread_template.recover = (vm_offset_t)NULL;

        thread_template.map = VM_MAP_NULL;

    #if CONFIG_DTRACE
        thread_template.t_dtrace_predcache = 0;
        thread_template.t_dtrace_vtime = 0;
        thread_template.t_dtrace_tracing = 0;
    #endif /* CONFIG_DTRACE */

        thread_template.t_chud = 0;
        thread_template.t_page_creation_count = 0;
        thread_template.t_page_creation_time = 0;

        thread_template.affinity_set = NULL;

        thread_template.syscalls_unix = 0;
        thread_template.syscalls_mach = 0;

        thread_template.t_ledger = LEDGER_NULL;
        thread_template.t_threadledger = LEDGER_NULL;

 
#if 0
		thread_template.appliedstate = default_task_null_policy;
        thread_template.ext_appliedstate = default_task_null_policy;
        thread_template.policystate = default_task_proc_policy;
        thread_template.ext_policystate = default_task_proc_policy;
#endif
    #if CONFIG_EMBEDDED
        thread_template.taskwatch = NULL;
        thread_template.saved_importance = 0;
    #endif /* CONFIG_EMBEDDED */

        init_thread = thread_template;

#warning Init thread initialization disabled!
    #if defined (__DARLING__) && 0
        // machine_set_current_thread(&init_thread);
        linux_current->mach_thread      = (void *) &init_thread;
        init_thread.linux_task          = linux_current;
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


kern_return_t duct_thread_create (task_t task, thread_t * new_thread)
{
        return duct_thread_create_internal2 (task, new_thread, FALSE);
}

static kern_return_t duct_thread_create_internal (task_t parent_task, integer_t priority, thread_continue_t continuation, int options, thread_t * out_thread)
{
#define TH_OPTION_NONE        0x00
#define TH_OPTION_NOCRED    0x01
#define TH_OPTION_NOSUSP    0x02

        thread_t                new_thread;
        static thread_t         first_thread = THREAD_NULL;

        /*
         *    Allocate a thread and initialize static fields
         */
        if (first_thread == THREAD_NULL)
                new_thread = first_thread = current_thread();

        new_thread      = (thread_t) duct_zalloc(thread_zone);
        if (new_thread == THREAD_NULL)
                return (KERN_RESOURCE_SHORTAGE);

        if (new_thread != first_thread)
                *new_thread = thread_template;


        // WC - todo: compat_uthread_alloc
#warning compat_uthread disabled
#if 0
		new_thread->compat_uthread = (void *) compat_uthread_alloc (parent_task, new_thread);
#endif

// #ifdef MACH_BSD
//     new_thread->uthread = uthread_alloc(parent_task, new_thread, (options & TH_OPTION_NOCRED) != 0);
//     if (new_thread->uthread == NULL) {
//         zfree(thread_zone, new_thread);
//         return (KERN_RESOURCE_SHORTAGE);
//     }
// #endif  /* MACH_BSD */

//     if (machine_thread_create(new_thread, parent_task) != KERN_SUCCESS) {
// #ifdef MACH_BSD
//         void *ut = new_thread->uthread;
//
//         new_thread->uthread = NULL;
//         /* cred free may not be necessary */
//         uthread_cleanup(parent_task, ut, parent_task->bsd_info);
//         uthread_cred_free(ut);
//         uthread_zone_free(ut);
// #endif  /* MACH_BSD */
//
//         zfree(thread_zone, new_thread);
//         return (KERN_FAILURE);
//     }

        new_thread->task = parent_task;

        thread_lock_init(new_thread);
        // wake_lock_init(new_thread);

        lck_mtx_init(&new_thread->mutex, &thread_lck_grp, &thread_lck_attr);

        ipc_thread_init(new_thread);

#if defined (__DARLING__)
#else
        // queue_init(&new_thread->held_ulocks);
        // new_thread->continuation = continuation;
#endif

        lck_mtx_lock(&tasks_threads_lock);

        task_lock(parent_task);

        if (!parent_task->active || parent_task->halting ||
            ((options & TH_OPTION_NOSUSP) != 0 &&
            parent_task->suspend_count > 0)    ||
            (parent_task->thread_count >= task_threadmax &&
            parent_task != kernel_task)        ) {

                task_unlock(parent_task);
                lck_mtx_unlock(&tasks_threads_lock);

// #ifdef MACH_BSD
//         {
//             void *ut = new_thread->uthread;
//
//             new_thread->uthread = NULL;
//             uthread_cleanup(parent_task, ut, parent_task->bsd_info);
//             /* cred free may not be necessary */
//             uthread_cred_free(ut);
//             uthread_zone_free(ut);
//         }
// #endif  /* MACH_BSD */
                ipc_thread_disable(new_thread);
                ipc_thread_terminate(new_thread);
                lck_mtx_destroy(&new_thread->mutex, &thread_lck_grp);
                // machine_thread_destroy(new_thread);
                zfree(thread_zone, new_thread);
                return (KERN_FAILURE);
        }

        // /* New threads inherit any default state on the task */
        // machine_thread_inherit_taskwide(new_thread, parent_task);

        task_reference_internal (parent_task);
//
#if defined (__DARLING__)
#else
        if (new_thread->task->rusage_cpu_flags & TASK_RUSECPU_FLAGS_PERTHR_LIMIT) {
                /*
                 * This task has a per-thread CPU limit; make sure this new thread
                 * gets its limit set too, before it gets out of the kernel.
                 */
                set_astledger(new_thread);
        }


        new_thread->t_threadledger = LEDGER_NULL;    /* per thread ledger is not inherited */
        new_thread->t_ledger = new_thread->task->ledger;

        if (new_thread->t_ledger)
                ledger_reference(new_thread->t_ledger);
#endif

        /* Cache the task's map */
        new_thread->map = parent_task->map;

//
//         /* Chain the thread onto the task's list */
         queue_enter(&parent_task->threads, new_thread, thread_t, task_threads);
         parent_task->thread_count++;
//
//         /* So terminating threads don't need to take the task lock to decrement */
         hw_atomic_add(&parent_task->active_thread_count, 1);
//
//         /* Protected by the tasks_threads_lock */
//         new_thread->thread_id = ++thread_unique_id;
//
//         queue_enter(&threads, new_thread, thread_t, threads);
//         threads_count++;
//
// #if defined (__DARLING__)
// #else
//         timer_call_setup(&new_thread->wait_timer, thread_timer_expire, new_thread);
//         timer_call_setup(&new_thread->depress_timer, thread_depress_expire, new_thread);
// #endif

// #if CONFIG_COUNTERS
//     /*
//      * If parent task has any reservations, they need to be propagated to this
//      * thread.
//      */
//     new_thread->t_chud = (TASK_PMC_FLAG == (parent_task->t_chud & TASK_PMC_FLAG)) ?
//         THREAD_PMC_FLAG : 0U;
// #endif


    // /* Set the thread's scheduling parameters */
    // new_thread->sched_mode = SCHED(initial_thread_sched_mode)(parent_task);
    // new_thread->sched_flags = 0;
    // new_thread->max_priority = parent_task->max_priority;
    // new_thread->task_priority = parent_task->priority;
    // new_thread->priority = (priority < 0)? parent_task->priority: priority;
    // if (new_thread->priority > new_thread->max_priority)
    //     new_thread->priority = new_thread->max_priority;
// #if CONFIG_EMBEDDED
//     if (new_thread->priority < MAXPRI_THROTTLE) {
//         new_thread->priority = MAXPRI_THROTTLE;
//     }
// #endif /* CONFIG_EMBEDDED */
    // new_thread->importance =
    //                 new_thread->priority - new_thread->task_priority;
// #if CONFIG_EMBEDDED
//     new_thread->saved_importance = new_thread->importance;
//     /* apple ios daemon starts all threads in darwin background */
//     if (parent_task->ext_appliedstate.apptype == PROC_POLICY_IOS_APPLE_DAEMON) {
//         /* Cannot use generic routines here so apply darwin bacground directly */
//         new_thread->policystate.hw_bg = TASK_POLICY_BACKGROUND_ATTRIBUTE_ALL;
//         /* set thread self backgrounding */
//         new_thread->appliedstate.hw_bg = new_thread->policystate.hw_bg;
//         /* priority will get recomputed suitably bit later */
//         new_thread->importance = INT_MIN;
//         /* to avoid changes to many pri compute routines, set the effect of those here */
//         new_thread->priority = MAXPRI_THROTTLE;
//     }
// #endif /* CONFIG_EMBEDDED */


// #if defined(CONFIG_SCHED_TRADITIONAL)
//     new_thread->sched_stamp = sched_tick;
//     new_thread->pri_shift = sched_pri_shift;
// #endif
//     SCHED(compute_priority)(new_thread, FALSE);
// #endif


        new_thread->active = TRUE;
        *out_thread = new_thread;

    // {
    //     long    dbg_arg1, dbg_arg2, dbg_arg3, dbg_arg4;
    //
    //     kdbg_trace_data(parent_task->bsd_info, &dbg_arg2);
    //
    //     KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
    //         TRACEDBG_CODE(DBG_TRACE_DATA, 1) | DBG_FUNC_NONE,
    //         (vm_address_t)(uintptr_t)thread_tid(new_thread), dbg_arg2, 0, 0, 0);
    //
    //     kdbg_trace_string(parent_task->bsd_info,
    //                         &dbg_arg1, &dbg_arg2, &dbg_arg3, &dbg_arg4);
    //
    //     KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
    //         TRACEDBG_CODE(DBG_TRACE_STRING, 1) | DBG_FUNC_NONE,
    //         dbg_arg1, dbg_arg2, dbg_arg3, dbg_arg4, 0);
    // }
    //
    // DTRACE_PROC1(lwp__create, thread_t, *out_thread);

        return (KERN_SUCCESS);
}


static kern_return_t duct_thread_create_internal2 (task_t task, thread_t * new_thread, boolean_t from_user)
{
        kern_return_t       result;
        thread_t            thread;

        if (task == TASK_NULL || task == kernel_task)
                return (KERN_INVALID_ARGUMENT);

        result  =
        duct_thread_create_internal (task, -1, (thread_continue_t) thread_bootstrap_return, TH_OPTION_NONE, &thread);
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
        task_unlock(task);
        lck_mtx_unlock(&tasks_threads_lock);

        *new_thread     = thread;

        return (KERN_SUCCESS);
}


#ifdef current_thread
#undef current_thread
#endif
thread_t current_thread (void)
{
        // kprintf ("calling current thread on linux task: 0x%x\n", (unsigned int) linux_current);
		return darling_thread_get_current();
}

void duct_thread_destroy(thread_t thread)
{
	task_t task;
	task = thread->task;
	
	thread->active = FALSE;
	hw_atomic_add(&task->active_thread_count, -1);
	
	duct_thread_deallocate(thread);
}

void duct_thread_deallocate (thread_t thread)
{
        task_t                task;

        if (thread == THREAD_NULL) {
                return;
        }

        task    = thread->task;

        if (thread_deallocate_internal (thread) > 0) {
                return;
        }

        ipc_thread_terminate (thread);

    // #ifdef MACH_BSD
    //     {
    //         void *ut = thread->uthread;
    //
    //         thread->uthread = NULL;
    //         uthread_zone_free(ut);
    //     }
    // #endif  /* MACH_BSD */

#if 0
        void      * uthread     = thread->compat_uthread;
        thread->compat_uthread  = NULL;
        // WC - todo check below: should use zone free (), not uthread_free
        compat_uthread_zone_free (uthread);
        // compat_uthread_free (thread->compat_uthread);
#endif

        // if (thread->t_ledger)
        //         ledger_dereference(thread->t_ledger);
        // if (thread->t_threadledger)
        //         ledger_dereference(thread->t_threadledger);

        // if (thread->kernel_stack != 0)
        //         stack_free (thread);

        // WC - todo check below
        // lck_mtx_destroy (&thread->mutex, &thread_lck_grp);
        // machine_thread_destroy (thread);

        // Remove itself from thread list
        task_lock(task);

        queue_remove(&task->threads, thread, thread_t, task_threads);
        task->thread_count--;

        task_unlock(task);

        task_deallocate (task);

        kprintf("Deallocating thread %p\n", thread);
        duct_zfree (thread_zone, thread);
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
