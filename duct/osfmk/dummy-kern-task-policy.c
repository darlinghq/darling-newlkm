/*
 * Copyright (c) 2000-2004 Apple Computer, Inc. All rights reserved.
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
#include <mach/task_server.h>

#include <kern/sched.h>
#include <kern/task.h>
#include <mach/thread_policy.h>
#include <sys/errno.h>
#include <sys/resource.h>
#include <machine/limits.h>
#include <kern/ledger.h>
#include <kern/thread_call.h>
#if CONFIG_EMBEDDED
#include <kern/kalloc.h>
#include <sys/errno.h>
#endif /* CONFIG_EMBEDDED */
#include <sys/kdebug.h>

#if CONFIG_MEMORYSTATUS
extern void memorystatus_on_suspend(int pid);
extern void memorystatus_on_resume(int pid);
#endif

// static int proc_apply_bgtaskpolicy_internal(task_t, int, int);
// static int proc_restore_bgtaskpolicy_internal(task_t, int, int, int);
// static int task_get_cpuusage(task_t task, uint32_t * percentagep, uint64_t * intervalp, uint64_t * deadlinep);
int task_set_cpuusage(task_t task, uint64_t percentage, uint64_t interval, uint64_t deadline, int scope);
// static int task_clear_cpuusage_locked(task_t task);
// static int task_apply_resource_actions(task_t task, int type);
// static void task_priority(task_t task, integer_t priority, integer_t max_priority);
// static kern_return_t task_role_default_handler(task_t task, task_role_t role);
void task_action_cpuusage(thread_call_param_t param0, thread_call_param_t param1);
// static int proc_apply_bgthreadpolicy_locked(thread_t thread, int selfset);
// static void restore_bgthreadpolicy_locked(thread_t thread, int selfset, int importance);
// static int proc_get_task_selfdiskacc_internal(task_t task, thread_t thread);
extern void unthrottle_thread(void * uthread);

#if CONFIG_EMBEDDED
// static void set_thread_appbg(thread_t thread, int setbg,int importance);
// static void apply_bgthreadpolicy_external(thread_t thread);
// static void add_taskwatch_locked(task_t task, task_watch_t * twp);
// static void remove_taskwatch_locked(task_t task, task_watch_t * twp);
// static void task_watch_lock(void);
// static void task_watch_unlock(void);
// static void apply_appstate_watchers(task_t task, int setbg);
void proc_apply_task_networkbg_internal(void *, thread_t);
void proc_restore_task_networkbg_internal(void *, thread_t);
int proc_pid(void * proc);

typedef struct thread_watchlist {
    thread_t thread;    /* thread being worked on for taskwatch action */
    int importance; /* importance to be restored if thread is being made active */
} thread_watchlist_t;

#endif /* CONFIG_EMBEDDED */


process_policy_t default_task_proc_policy = {0,
                         0,
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        0,
                        TASK_POLICY_HWACCESS_CPU_ATTRIBUTE_FULLACCESS,
                        TASK_POLICY_HWACCESS_NET_ATTRIBUTE_FULLACCESS,
                        TASK_POLICY_HWACCESS_GPU_ATTRIBUTE_FULLACCESS,
                        TASK_POLICY_HWACCESS_DISK_ATTRIBUTE_NORMAL,
                        TASK_POLICY_BACKGROUND_ATTRIBUTE_ALL
                        };

process_policy_t default_task_null_policy = {0,
                         0,
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        TASK_POLICY_RESOURCE_ATTRIBUTE_NONE, 
                        0,
                        TASK_POLICY_HWACCESS_GPU_ATTRIBUTE_NONE,
                        TASK_POLICY_HWACCESS_NET_ATTRIBUTE_NONE,
                        TASK_POLICY_HWACCESS_GPU_ATTRIBUTE_NONE,
                        TASK_POLICY_HWACCESS_DISK_ATTRIBUTE_NORMAL,
                        TASK_POLICY_BACKGROUND_ATTRIBUTE_NONE
                        };
            


/*
 * This routine should always be called with the task lock held.
 * This routine handles Default operations for TASK_FOREGROUND_APPLICATION 
 * and TASK_BACKGROUND_APPLICATION of task with no special app type.
 */
// static kern_return_t
// task_role_default_handler(task_t task, task_role_t role)
// {
//     return 0;
// }


kern_return_t
task_policy_set(
    task_t                  task,
    task_policy_flavor_t    flavor,
    task_policy_t           policy_info,
    mach_msg_type_number_t  count)
{
        kprintf("not implemented: task_policy_set()\n");
        return 0;
}

// static void
// task_priority(
//     task_t          task,
//     integer_t       priority,
//     integer_t       max_priority)
// {
//     ;
// }

kern_return_t
task_importance(
    task_t              task,
    integer_t           importance)
{
        kprintf("not implemented: task_importance()\n");
        return 0;
}
        
kern_return_t
task_policy_get(
    task_t                  task,
    task_policy_flavor_t    flavor,
    task_policy_t           policy_info,
    mach_msg_type_number_t  *count,
    boolean_t               *get_default)
{
        kprintf("not implemented: task_policy_get()\n");
        return 0;
}

/* task Darwin BG enforcement/settings related routines */
int 
proc_get_task_bg_policy(task_t task)
{
        kprintf("not implemented: proc_get_task_bg_policy()\n");
        return 0;
}


int 
proc_get_thread_bg_policy(task_t task, uint64_t tid)
{
        kprintf("not implemented: proc_get_thread_bg_policy()\n");
        return 0;
}

int
proc_get_self_isbackground(void)
{
        kprintf("not implemented: proc_get_self_isbackground()\n");
        return 0;
}

int proc_get_selfthread_isbackground(void)
{
        kprintf("not implemented: proc_get_selfthread_isbackground()\n");
        return 0;
}


int 
proc_set_bgtaskpolicy(task_t task, int intval)
{
        kprintf("not implemented: proc_set_bgtaskpolicy()\n");
        return 0;
}

/* set and apply as well , handles reset of NONUI due to setprio() task app state implmn side effect */
int 
proc_set_and_apply_bgtaskpolicy(task_t task, int prio)
{
        kprintf("not implemented: proc_set_and_apply_bgtaskpolicy()\n");
        return 0;
}


int 
proc_set_bgthreadpolicy(task_t task, uint64_t tid, int prio)
{
        kprintf("not implemented: proc_set_bgthreadpolicy()\n");
        return 0;
}

int 
proc_set_and_apply_bgthreadpolicy(task_t task, uint64_t tid, int prio)
{
        kprintf("not implemented: proc_set_and_apply_bgthreadpolicy()\n");
        return 0;
}

int 
proc_add_bgtaskpolicy(task_t task, int val)
{
        kprintf("not implemented: proc_add_bgtaskpolicy()\n");
        return 0;
}

int 
proc_add_bgthreadpolicy(task_t task, uint64_t tid, int val)
{
        kprintf("not implemented: proc_add_bgthreadpolicy()\n");
        return 0;
}

int 
proc_remove_bgtaskpolicy(task_t task, int intval)
{
        kprintf("not implemented: proc_remove_bgtaskpolicy()\n");
        return 0;
}

int 
proc_remove_bgthreadpolicy(task_t task, uint64_t tid, int val)
{
        kprintf("not implemented: proc_remove_bgthreadpolicy()\n");
        return 0;
}

int
proc_apply_bgtask_selfpolicy(void)
{
        kprintf("not implemented: proc_apply_bgtask_selfpolicy()\n");
        return 0;
}

int 
proc_apply_bgtaskpolicy(task_t task)
{
        kprintf("not implemented: proc_apply_bgtaskpolicy()\n");
        return 0;
}

int 
proc_apply_bgtaskpolicy_external(task_t task)
{
        kprintf("not implemented: proc_apply_bgtaskpolicy_external()\n");
        return 0;
}

// static int
// proc_apply_bgtaskpolicy_internal(task_t task, int locked, int external)
// {
//     return 0;
// }

/* apply the self backgrounding even if the thread is not current thread */
int 
proc_apply_workq_bgthreadpolicy(thread_t thread)
{
        kprintf("not implemented: proc_apply_workq_bgthreadpolicy()\n");
        return 0;
}

int 
proc_apply_bgthreadpolicy(task_t task, uint64_t tid)
{
        kprintf("not implemented: proc_apply_bgthreadpolicy()\n");
        return 0;
}

// static int 
// proc_apply_bgthreadpolicy_locked(thread_t thread, int selfset)
// {
//     return 0;
// }

#if CONFIG_EMBEDDED
/* set external application of background */
// static void 
// apply_bgthreadpolicy_external(thread_t thread)
// {
//     ;
// }
#endif /* CONFIG_EMBEDDED */

int
proc_apply_bgthread_selfpolicy(void)
{
        kprintf("not implemented: proc_apply_bgthread_selfpolicy()\n");
        return 0;
}

int 
proc_restore_bgtaskpolicy(task_t task)
{
        kprintf("not implemented: proc_restore_bgtaskpolicy()\n");
        return 0;
}

// static int
// proc_restore_bgtaskpolicy_internal(task_t task, int locked, int external, int pri)
// {
//     return 0;
// }

/* restore the self backgrounding even if the thread is not current thread */
int 
proc_restore_workq_bgthreadpolicy(thread_t thread)
{
        kprintf("not implemented: proc_restore_workq_bgthreadpolicy()\n");
        return 0;
}

int 
proc_restore_bgthread_selfpolicy(void)
{
        kprintf("not implemented: proc_restore_bgthread_selfpolicy()\n");
        return 0;
}

int 
proc_restore_bgthreadpolicy(task_t task, uint64_t tid)
{
        kprintf("not implemented: proc_restore_bgthreadpolicy()\n");
        return 0;
}

// static void
// restore_bgthreadpolicy_locked(thread_t thread, int selfset, int importance)
// {
//     ;
// }

void 
#if CONFIG_EMBEDDED
proc_set_task_apptype(task_t task, int type, thread_t thread)
#else
proc_set_task_apptype(task_t task, int type, __unused thread_t thread)
#endif
{
        kprintf("not implemented: proc_set_task_apptype()\n");
}

/* update the darwin backdground action state in the flags field for libproc */
#define PROC_FLAG_DARWINBG      0x8000  /* process in darwin background */
#define PROC_FLAG_EXT_DARWINBG  0x10000 /* process in darwin background - external enforcement */
#define PROC_FLAG_IOS_APPLEDAEMON  0x20000 /* process is apple ios daemon */

int
proc_get_darwinbgstate(task_t task, uint32_t * flagsp)
{
        kprintf("not implemented: proc_get_darwinbgstate()\n");
        return 0;
}

/* 
 * HW disk access realted routines, they need to return 
 * IOPOL_XXX equivalents for spec_xxx/throttle updates.
 */

int 
proc_get_task_disacc(task_t task)
{
        kprintf("not implemented: proc_get_task_disacc()\n");
        return 0;
}

int
proc_get_task_selfdiskacc_internal(task_t task, thread_t thread)
{
        kprintf("not implemented: proc_get_task_selfdiskacc_internal()\n");
        return 0;
}


int
proc_get_task_selfdiskacc(void)
{
        kprintf("not implemented: proc_get_task_selfdiskacc()\n");
        return 0;
}


int
proc_get_diskacc(thread_t thread)
{
        kprintf("not implemented: proc_get_diskacc()\n");
        return 0;
}


int
proc_get_thread_selfdiskacc(void)
{
        kprintf("not implemented: proc_get_thread_selfdiskacc()\n");
        return 0;
}

int 
proc_apply_task_diskacc(task_t task, int policy)
{
        kprintf("not implemented: proc_apply_task_diskacc()\n");
        return 0;
}

int 
proc_apply_thread_diskacc(task_t task, uint64_t tid, int policy)
{
        kprintf("not implemented: proc_apply_thread_diskacc()\n");
        return 0;
}

void
proc_task_remove_throttle(task_t task)
{
        kprintf("not implemented: proc_task_remove_throttle()\n");
}



int
proc_apply_thread_selfdiskacc(int policy)
{
        kprintf("not implemented: proc_apply_thread_selfdiskacc()\n");
        return 0;
}

int 
proc_denyinherit_policy(__unused task_t task)
{
        kprintf("not implemented: proc_denyinherit_policy()\n");
        return 0;
}

int 
proc_denyselfset_policy(__unused task_t task)
{
        kprintf("not implemented: proc_denyselfset_policy()\n");
        return 0;
}

/* HW GPU access related routines */
int
proc_get_task_selfgpuacc_deny(void)
{
        kprintf("not implemented: proc_get_task_selfgpuacc_deny()\n");
        return 0;
}

int
proc_apply_task_gpuacc(task_t task, int policy)
{
        kprintf("not implemented: proc_apply_task_gpuacc()\n");
        return 0;
}

/* Resource usage , CPU realted routines */
int 
proc_get_task_ruse_cpu(task_t task, uint32_t * policyp, uint32_t * percentagep, uint64_t * intervalp, uint64_t * deadlinep)
{
        kprintf("not implemented: proc_get_task_ruse_cpu()\n");
        return 0;
}

/*
 * Currently supported configurations for CPU limits.
 *
 *                  Deadline-based CPU limit        Percentage-based CPU limit
 * PROC_POLICY_RSRCACT_THROTTLE     ENOTSUP             Task-wide scope only
 * PROC_POLICY_RSRCACT_SUSPEND      Task-wide scope only        ENOTSUP
 * PROC_POLICY_RSRCACT_TERMINATE    Task-wide scope only        ENOTSUP
 * PROC_POLICY_RSRCACT_NOTIFY_KQ    Task-wide scope only        ENOTSUP
 * PROC_POLICY_RSRCACT_NOTIFY_EXC   ENOTSUP             Per-thread scope only
 *
 * A deadline-based CPU limit is actually a simple wallclock timer - the requested action is performed
 * after the specified amount of wallclock time has elapsed.
 *
 * A percentage-based CPU limit performs the requested action after the specified amount of actual CPU time
 * has been consumed -- regardless of how much wallclock time has elapsed -- by either the task as an
 * aggregate entity (so-called "Task-wide" or "Proc-wide" scope, whereby the CPU time consumed by all threads
 * in the task are added together), or by any one thread in the task (so-called "per-thread" scope).
 *
 * We support either deadline != 0 OR percentage != 0, but not both. The original intention in having them
 * share an API was to use actual CPU time as the basis of the deadline-based limit (as in: perform an action
 * after I have used some amount of CPU time; this is different than the recurring percentage/interval model)
 * but the potential consumer of the API at the time was insisting on wallclock time instead.
 *
 * Currently, requesting notification via an exception is the only way to get per-thread scope for a
 * CPU limit. All other types of notifications force task-wide scope for the limit.
 */
int 
proc_set_task_ruse_cpu(task_t task, uint32_t policy, uint32_t percentage, uint64_t interval, uint64_t deadline)
{
        kprintf("not implemented: proc_set_task_ruse_cpu()\n");
        return 0;
}

int 
proc_clear_task_ruse_cpu(task_t task)
{
        kprintf("not implemented: proc_clear_task_ruse_cpu()\n");
        return 0;
}

/* For ledger hookups */
// static int
// task_get_cpuusage(task_t task, uint32_t * percentagep, uint64_t * intervalp, uint64_t * deadlinep)
// {
//     return 0;
// }

int
task_set_cpuusage(task_t task, uint64_t percentage, uint64_t interval, uint64_t deadline, int scope)
{
        kprintf("not implemented: task_set_cpuusage()\n");
        return 0;
}

int
task_clear_cpuusage(task_t task)
{
        kprintf("not implemented: task_clear_cpuusage()\n");
        return 0;
}

int
task_clear_cpuusage_locked(task_t task)
{
        kprintf("not implemented: task_clear_cpuusage_locked()\n");
        return 0;
}

/* called by ledger unit to enforce action due to  resource usage criteria being met */
void
task_action_cpuusage(thread_call_param_t param0, __unused thread_call_param_t param1)
{
        kprintf("not implemented: task_action_cpuusage()\n");
}

#if CONFIG_EMBEDDED
/* return the appstate of a task */
int
proc_lf_getappstate(task_t task)
{
        kprintf("not implemented: proc_lf_getappstate()\n");
        return 0;
}

// static void
// task_watch_lock(void)
// {
//     ;
// }

// static void
// task_watch_unlock(void)
// {
//     ;
// }

// static void
// add_taskwatch_locked(task_t task, task_watch_t * twp)
// {
//     ;
// }


int 
proc_lf_pidbind(task_t curtask, uint64_t tid, task_t target_task, int bind)
{
        kprintf("not implemented: proc_lf_pidbind()\n");
        return 0;
}

// static void
// set_thread_appbg(thread_t thread, int setbg,int importance)
// {
//     ;
// }

// static void
// apply_appstate_watchers(task_t task, int setbg)
// {
//     ;
// }

void
thead_remove_taskwatch(thread_t thread)
{
        kprintf("not implemented: thead_remove_taskwatch()\n");
}

void
task_removewatchers(task_t task)
{
        kprintf("not implemented: task_removewatchers()\n");
}
#endif /* CONFIG_EMBEDDED */

int 
proc_disable_task_apptype(task_t task, int policy_subtype)
{
        kprintf("not implemented: proc_disable_task_apptype()\n");
        return 0;
}

int 
proc_enable_task_apptype(task_t task, int policy_subtype)
{
        kprintf("not implemented: proc_enable_task_apptype()\n");
        return 0;
}

#if CONFIG_EMBEDDED
int
proc_setthread_saved_importance(thread_t thread, int importance)
{
        kprintf("not implemented: proc_setthread_saved_importance()\n");
        return 0;
}
#endif /* CONFIG_EMBEDDED */
