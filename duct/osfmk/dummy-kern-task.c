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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
 *  File:   kern/task.c
 *  Author: Avadis Tevanian, Jr., Michael Wayne Young, David Golub,
 *      David Black
 *
 *  Task management primitives implementation.
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
/*
 * NOTICE: This file was modified by McAfee Research in 2004 to introduce
 * support for mandatory and extensible security protections.  This notice
 * is included in support of clause 2.2 (b) of the Apple Public License,
 * Version 2.0.
 * Copyright (c) 2005 SPARTA, Inc.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <fast_tas.h>
#include <platforms.h>

#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <mach/host_priv.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_param.h>
#include <mach/semaphore.h>
#include <mach/task_info.h>
#include <mach/task_special_ports.h>

#include <ipc/ipc_types.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_entry.h>

#include <kern/kern_types.h>
#include <kern/mach_param.h>
#include <kern/misc_protos.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/zalloc.h>
#include <kern/kalloc.h>
#include <kern/processor.h>
#include <kern/sched_prim.h>    /* for thread_wakeup */
#include <kern/ipc_tt.h>
#include <kern/host.h>
#include <kern/clock.h>
#include <kern/timer.h>
#include <kern/assert.h>
#include <kern/sync_lock.h>
#include <kern/affinity.h>

#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>     /* for kernel_map, ipc_kernel_map */
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>

/*
 * Exported interfaces
 */

#include <mach/task_server.h>
#include <mach/mach_host_server.h>
#include <mach/host_security_server.h>
#include <mach/mach_port_server.h>

#include <vm/vm_shared_region.h>

#if CONFIG_MACF_MACH
#include <security/mac_mach_internal.h>
#endif

#if CONFIG_COUNTERS
#include <pmc/pmc.h>
#endif /* CONFIG_COUNTERS */

#include <duct/duct_post_xnu.h>
#include <darling/debug_print.h>

// task_t          kernel_task;
extern zone_t          task_zone;
// lck_attr_t      task_lck_attr;
// lck_grp_t       task_lck_grp;
// lck_grp_attr_t  task_lck_grp_attr;
#if CONFIG_EMBEDDED
lck_mtx_t   task_watch_mtx;
#endif /* CONFIG_EMBEDDED */

zinfo_usage_store_t tasks_tkm_private;
zinfo_usage_store_t tasks_tkm_shared;

/* A container to accumulate statistics for expired tasks */
expired_task_statistics_t                dead_task_statistics;
lck_spin_t                dead_task_statistics_lock;

static ledger_template_t task_ledger_template = NULL;
struct _task_ledger_indices task_ledgers = {-1, -1, -1, -1, -1, -1, -1};
void init_task_ledgers(void);


int task_max = CONFIG_TASK_MAX; /* Max number of tasks */

/* externs for BSD kernel */
extern void proc_getexecutableuuid(void *, unsigned char *, unsigned long);

extern kern_return_t thread_suspend(thread_t thread);
extern kern_return_t thread_resume(thread_t thread);

/* Forwards */

void        task_hold_locked(
            task_t      task);
void        task_wait_locked(
            task_t      task,
            boolean_t   until_not_runnable);
void        task_release_locked(
            task_t      task);
void        task_free(
            task_t      task );
void        task_synchronizer_destroy_all(
            task_t      task);

int check_for_tasksuspend(
            task_t task);

void
task_backing_store_privileged(
            task_t task)
{
        kprintf("not implemented: task_backing_store_privileged()\n");
}


void
task_set_64bit(
        task_t task,
        boolean_t is64bit,
        boolean_t is_64bit_data)
{
        kprintf("not implemented: task_set_64bit()\n");
}


void
task_set_dyld_info(task_t task, mach_vm_address_t addr, mach_vm_size_t size)
{
        kprintf("not implemented: task_set_dyld_info()\n");
}

// void
// task_init(void)
// {
//     ;
// }

/*
 * Create a task running in the kernel address space.  It may
 * have its own map of size mem_size and may have ipc privileges.
 */
kern_return_t
kernel_task_create(
    __unused task_t     parent_task,
    __unused vm_offset_t        map_base,
    __unused vm_size_t      map_size,
    __unused task_t     *child_task)
{
        kprintf("not implemented: kernel_task_create()\n");
        return 0;
}

kern_return_t
task_create(
    task_t              parent_task,
    __unused ledger_port_array_t    ledger_ports,
    __unused mach_msg_type_number_t num_ledger_ports,
    __unused boolean_t      inherit_memory,
    __unused task_t         *child_task)    /* OUT */
{
        kprintf("not implemented: task_create()\n");
        return 0;
}

kern_return_t
host_security_create_task_token(
    host_security_t         host_security,
    task_t              parent_task,
    __unused security_token_t   sec_token,
    __unused audit_token_t      audit_token,
    __unused host_priv_t        host_priv,
    __unused ledger_port_array_t    ledger_ports,
    __unused mach_msg_type_number_t num_ledger_ports,
    __unused boolean_t      inherit_memory,
    __unused task_t         *child_task)    /* OUT */
{
        kprintf("not implemented: host_security_create_task_token()\n");
        return 0;
}

void
init_task_ledgers(void)
{
        kprintf("not implemented: init_task_ledgers()\n");
}




/*
 *  task_deallocate:
 *
 *  Drop a reference on a task.
 */
void
task_deallocate(
    task_t      task)
{
        int refc = task_deallocate_internal (task);
        if (refc > 0) {
			    debug_msg("Not freeing task, %d refs left\n", refc);
                return;
        }

        ipc_task_terminate (task);

        debug_msg("Freeing task %p\n", task);
        duct_zfree(task_zone, task);
}

/*
 *  task_name_deallocate:
 *
 *  Drop a reference on a task name.
 */
void
task_name_deallocate(
    task_name_t     task_name)
{
    task_deallocate((task_t) task_name);
}


/*
 *  task_terminate:
 *
 *  Terminate the specified task.  See comments on thread_terminate
 *  (kern/thread.c) about problems with terminating the "current task."
 */

kern_return_t
task_terminate(
    task_t      task)
{
        kprintf("not implemented: task_terminate()\n");
        return 0;
}

kern_return_t
task_terminate_internal(
    task_t          task)
{
        kprintf("not implemented: task_terminate_internal()\n");
        return 0;
}

/*
 * task_start_halt:
 *
 *  Shut the current task down (except for the current thread) in
 *  preparation for dramatic changes to the task (probably exec).
 *  We hold the task and mark all other threads in the task for
 *  termination.
 */
kern_return_t
task_start_halt(
    task_t      task)
{
        kprintf("not implemented: task_start_halt()\n");
        return 0;
}


/*
 * task_complete_halt:
 *
 *  Complete task halt by waiting for threads to terminate, then clean
 *  up task resources (VM, port namespace, etc...) and then let the
 *  current thread go in the (practically empty) task context.
 */
void
task_complete_halt(task_t task)
{
        kprintf("not implemented: task_complete_halt()\n");
}

/*
 *  task_hold_locked:
 *
 *  Suspend execution of the specified task.
 *  This is a recursive-style suspension of the task, a count of
 *  suspends is maintained.
 *
 *  CONDITIONS: the task is locked and active.
 */
 /*
void
task_hold_locked(
    register task_t     task)
{
        kprintf("not implemented: task_hold_locked()\n");
}
*/

/*
 *  task_hold:
 *
 *  Same as the internal routine above, except that is must lock
 *  and verify that the task is active.  This differs from task_suspend
 *  in that it places a kernel hold on the task rather than just a 
 *  user-level hold.  This keeps users from over resuming and setting
 *  it running out from under the kernel.
 *
 *  CONDITIONS: the caller holds a reference on the task
 */
 /*
kern_return_t
task_hold(
    register task_t     task)
{
        kprintf("not implemented: task_hold()\n");
        return 0;
}
*/

kern_return_t
task_wait(
        task_t      task,
        boolean_t   until_not_runnable)
{
        kprintf("not implemented: task_wait()\n");
        return 0;
}

/*
 *  task_wait_locked:
 *
 *  Wait for all threads in task to stop.
 *
 * Conditions:
 *  Called with task locked, active, and held.
 */
void
task_wait_locked(
    register task_t     task,
    boolean_t       until_not_runnable)
{
        kprintf("not implemented: task_wait_locked()\n");
}

/*
 *  task_release_locked:
 *
 *  Release a kernel hold on a task.
 *
 *  CONDITIONS: the task is locked and active
 */
 /*
void
task_release_locked(
    register task_t     task)
{
        kprintf("not implemented: task_release_locked()\n");
}
*/

/*
 *  task_release:
 *
 *  Same as the internal routine above, except that it must lock
 *  and verify that the task is active.
 *
 *  CONDITIONS: The caller holds a reference to the task
 */
 /*
kern_return_t
task_release(
    task_t      task)
{
        kprintf("not implemented: task_release()\n");
        return 0;
}
*/

kern_return_t
task_threads(
    task_t                  task,
    thread_act_array_t      *threads_out,
    mach_msg_type_number_t  *count)
{
	mach_msg_type_number_t	actual;
	thread_t				*thread_list;
	thread_t				thread;
	vm_size_t				size, size_needed;
	void					*addr;
	unsigned int			i, j;

	if (task == TASK_NULL)
		return (KERN_INVALID_ARGUMENT);
	if (task->map->linux_task == NULL)
		return KERN_FAILURE;

	size = 0; addr = NULL;

	for (;;) {
		debug_msg("Before lock\n");
		task_lock(task);
		debug_msg("After lock\n");
		if (!task->active) {
			task_unlock(task);

			if (size != 0)
				kfree(addr, size);

			return (KERN_FAILURE);
		}

		actual = task->thread_count;

		/* do we have the memory we need? */
		size_needed = actual * sizeof (mach_port_t);
		if (size_needed <= size)
			break;

		/* unlock the task and allocate more memory */
		task_unlock(task);

		if (size != 0)
			kfree(addr, size);

		assert(size_needed > 0);
		size = size_needed;

		addr = kalloc(size);
		if (addr == 0)
			return (KERN_RESOURCE_SHORTAGE);
	}

	/* OK, have memory and the task is locked & active */
	thread_list = (thread_t *)addr;

	i = j = 0;

	for (thread = (thread_t)queue_first(&task->threads); i < actual;
				++i, thread = (thread_t)queue_next(&thread->task_threads)) {
		thread_reference_internal(thread);
		thread_list[j++] = thread;
	}

	assert(queue_end(&task->threads, (queue_entry_t)thread));

	actual = j;
	size_needed = actual * sizeof (mach_port_t);

	/* can unlock task now that we've got the thread refs */
	task_unlock(task);

	if (actual == 0) {
		/* no threads, so return null pointer and deallocate memory */

		*threads_out = NULL;
		*count = 0;

		if (size != 0)
			kfree(addr, size);
	}
	else {
		/* if we allocated too much, must copy */

		if (size_needed < size) {
			void *newaddr;

			newaddr = kalloc(size_needed);
			if (newaddr == 0) {
				for (i = 0; i < actual; ++i)
					thread_deallocate(thread_list[i]);
				kfree(addr, size);
				return (KERN_RESOURCE_SHORTAGE);
			}

			bcopy(addr, newaddr, size_needed);
			kfree(addr, size);
			thread_list = (thread_t *)newaddr;
		}

		*threads_out = thread_list;
		*count = actual;

		/* do the conversion that Mig should handle */

		for (i = 0; i < actual; ++i)
			((ipc_port_t *) thread_list)[i] = convert_thread_to_port(thread_list[i]);
	}

	debug_msg("Going to return success\n");
	return (KERN_SUCCESS);
}


static kern_return_t state_to_task(task_t task, int state)
{
	struct task_struct *t, *ltask;
	kern_return_t rv;
	int vpid;

	if (task == TASK_NULL)
		return KERN_INVALID_ARGUMENT;

	ltask = task->map->linux_task;

	if (ltask == NULL)
		return KERN_FAILURE;

	// read_lock(&tasklist_lock);
	rcu_read_lock();
	t = ltask;

	do
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,14,0)
		smp_store_mb(t->__state, state);
#else
		smp_store_mb(t->state, state);
#endif
		if (state != TASK_STOPPED)
			wake_up_process(t);
	}
	while_each_thread(ltask, t);
	rcu_read_unlock();

	// read_unlock(&tasklist_lock);
	return KERN_SUCCESS;
}

/*
 *  task_suspend:
 *
 *  Implement a user-level suspension on a task.
 *
 * Conditions:
 *  The caller holds a reference to the task
 */
 /*
kern_return_t
task_suspend(
    register task_t     task)
{
        thread_t thread;
        queue_iterate(&task->threads, thread, thread_t, task_threads)
        {
                thread_suspend(thread);
        }
        task->suspend_count++;
	return KERN_SUCCESS;
}
*/

/*
 *  task_resume:
 *      Release a kernel hold on a task.
 *      
 * Conditions:
 *      The caller holds a reference to the task
 */
 /*
kern_return_t 
task_resume(
    register task_t task)
{
        thread_t thread;
	queue_iterate(&task->threads, thread, thread_t, task_threads)
        {
                thread_resume(thread);
        }
        task->suspend_count = 0;
	return KERN_SUCCESS;
}
*/

kern_return_t
task_map_corpse_info(
	task_t task,
	task_t corpse_task,
	vm_address_t *kcd_addr_begin,
	uint32_t *kcd_size)
{
        kprintf("not implemented: task_map_corpse_info()\n");
        return KERN_NOT_SUPPORTED;
}

kern_return_t
task_set_phys_footprint_limit(
	task_t task,
	int new_limit_mb,
	int *old_limit_mb)
{
        kprintf("not implemented: task_set_phys_footprint_limit()\n");
        return KERN_NOT_SUPPORTED;
}

kern_return_t
task_map_corpse_info_64(
	task_t task,
	task_t corpse_task,
	mach_vm_address_t *kcd_addr_begin,
	mach_vm_size_t *kcd_size)
{
        kprintf("not implemented: task_map_corpse_info_64()\n");
        return KERN_NOT_SUPPORTED;
}

kern_return_t
task_pidsuspend_locked(task_t task)
{
        kprintf("not implemented: task_pidsuspend_locked()\n");
        return 0;
}


/*
 *  task_pidsuspend:
 *
 *  Suspends a task by placing a hold on its threads.
 *
 * Conditions:
 *  The caller holds a reference to the task
 */
kern_return_t
task_pidsuspend(
    register task_t     task)
{
        kprintf("not implemented: task_pidsuspend()\n");
        return 0;
}

/* If enabled, we bring all the frozen pages back in prior to resumption; otherwise, they're faulted back in on demand */
#define THAW_ON_RESUME 1

/*
 *  task_pidresume:
 *      Resumes a previously suspended task.
 *      
 * Conditions:
 *      The caller holds a reference to the task
 */
kern_return_t 
task_pidresume(
    register task_t task)
{
        kprintf("not implemented: task_pidresume()\n");
        return 0;
}

#if CONFIG_FREEZE

/*
 *  task_freeze:
 *
 *  Freeze a task.
 *
 * Conditions:
 *  The caller holds a reference to the task
 */
kern_return_t
task_freeze(
    task_t task,
    uint32_t* purgeable_count,
    uint32_t* wired_count,
    uint32_t* clean_count,
    uint32_t* dirty_count,
    uint32_t dirty_budget,
    uint32_t* shared_count,
    int* freezer_error_code,
    boolean_t eval_only)
{
        kprintf("not implemented: task_freeze()\n");
        return 0;
}

/*
 *  task_thaw:
 *
 *  Thaw a currently frozen task.
 *
 * Conditions:
 *  The caller holds a reference to the task
 */
kern_return_t
task_thaw(
    register task_t     task)
{
        kprintf("not implemented: task_thaw()\n");
        return 0;
}

#endif /* CONFIG_FREEZE */

kern_return_t
host_security_set_task_token(
        host_security_t  host_security,
        task_t       task,
        security_token_t sec_token,
    audit_token_t    audit_token,
    host_priv_t  host_priv)
{
        kprintf("not implemented: host_security_set_task_token()\n");
        return 0;
}

/*
 * This routine was added, pretty much exclusively, for registering the
 * RPC glue vector for in-kernel short circuited tasks.  Rather than
 * removing it completely, I have only disabled that feature (which was
 * the only feature at the time).  It just appears that we are going to
 * want to add some user data to tasks in the future (i.e. bsd info,
 * task names, etc...), so I left it in the formal task interface.
 */
kern_return_t
task_set_info(
    task_t      task,
    task_flavor_t   flavor,
    __unused task_info_t    task_info_in,       /* pointer to IN array */
    __unused mach_msg_type_number_t task_info_count)
{
        kprintf("not implemented: task_set_info()\n");
        return 0;
}

#if 0
kern_return_t
task_info(
    task_t                  task,
    task_flavor_t           flavor,
    task_info_t             task_info_out,
    mach_msg_type_number_t  *task_info_count)
{
        kprintf("not implemented: task_info()\n");
        return 0;
}
#endif

void
task_vtimer_set(
    task_t      task,
    integer_t   which)
{
        kprintf("not implemented: task_vtimer_set()\n");
}

void
task_vtimer_clear(
    task_t      task,
    integer_t   which)
{
        kprintf("not implemented: task_vtimer_clear()\n");
}

void
task_vtimer_update(
__unused
    task_t      task,
    integer_t   which,
    uint32_t    *microsecs)
{
        kprintf("not implemented: task_vtimer_update()\n");
}

/*
 *  task_assign:
 *
 *  Change the assigned processor set for the task
 */
kern_return_t
task_assign(
    __unused task_t     task,
    __unused processor_set_t    new_pset,
    __unused boolean_t  assign_threads)
{
        kprintf("not implemented: task_assign()\n");
        return 0;
}

/*
 *  task_assign_default:
 *
 *  Version of task_assign to assign to default processor set.
 */
kern_return_t
task_assign_default(
    task_t      task,
    boolean_t   assign_threads)
{
        kprintf("not implemented: task_assign_default()\n");
        return 0;
}

/*
 *  task_get_assignment
 *
 *  Return name of processor set that task is assigned to.
 */
kern_return_t
task_get_assignment(
    task_t      task,
    processor_set_t *pset)
{
        kprintf("not implemented: task_get_assignment()\n");
        return 0;
}


/*
 *  task_policy
 *
 *  Set scheduling policy and parameters, both base and limit, for
 *  the given task. Policy must be a policy which is enabled for the
 *  processor set. Change contained threads if requested. 
 */
kern_return_t
task_policy(
    __unused task_t         task,
    __unused policy_t           policy_id,
    __unused policy_base_t      base,
    __unused mach_msg_type_number_t count,
    __unused boolean_t          set_limit,
    __unused boolean_t          change)
{
        kprintf("not implemented: task_policy()\n");
        return 0;
}

void task_bsdtask_kill(task_t task)
{
        kprintf("not implemented: task_bsdtask_kill()\n");
}

/*
 *  task_set_policy
 *
 *  Set scheduling policy and parameters, both base and limit, for 
 *  the given task. Policy can be any policy implemented by the
 *  processor set, whether enabled or not. Change contained threads
 *  if requested.
 */
kern_return_t
task_set_policy(
    __unused task_t         task,
    __unused processor_set_t        pset,
    __unused policy_t           policy_id,
    __unused policy_base_t      base,
    __unused mach_msg_type_number_t base_count,
    __unused policy_limit_t     limit,
    __unused mach_msg_type_number_t limit_count,
    __unused boolean_t          change)
{
        kprintf("not implemented: task_set_policy()\n");
        return 0;
}

#if FAST_TAS
kern_return_t
task_set_ras_pc(
    task_t      task,
    vm_offset_t pc,
    vm_offset_t endpc)
{
        kprintf("not implemented: task_set_ras_pc()\n");
        return 0;
}
#else   /* FAST_TAS */
kern_return_t
task_set_ras_pc(
    __unused task_t task,
    __unused vm_offset_t    pc,
    __unused vm_offset_t    endpc)
{
        kprintf("not implemented: task_set_ras_pc()\n");
        return 0;
}
#endif  /* FAST_TAS */

void
task_synchronizer_destroy_all(task_t task)
{
        kprintf("not implemented: task_synchronizer_destroy_all()\n");
}

/*
 * Install default (machine-dependent) initial thread state 
 * on the task.  Subsequent thread creation will have this initial
 * state set on the thread by machine_thread_inherit_taskwide().
 * Flavors and structures are exactly the same as those to thread_set_state()
 */
kern_return_t 
task_set_state(
    task_t task, 
    int flavor, 
    thread_state_t state, 
    mach_msg_type_number_t state_count)
{
        kprintf("not implemented: task_set_state()\n");
        return 0;
}

/*
 * Examine the default (machine-dependent) initial thread state 
 * on the task, as set by task_set_state().  Flavors and structures
 * are exactly the same as those passed to thread_get_state().
 */
kern_return_t 
task_get_state(
    task_t  task, 
    int flavor,
    thread_state_t state,
    mach_msg_type_number_t *state_count)
{
        kprintf("not implemented: task_get_state()\n");
        return 0;
}


/*
 * We need to export some functions to other components that
 * are currently implemented in macros within the osfmk
 * component.  Just export them as functions of the same name.
 */
boolean_t is_kerneltask(task_t t)
{
        kprintf("not implemented: is_kerneltask()\n");
        return 0;
}

int
check_for_tasksuspend(task_t task)
{
        kprintf("not implemented: check_for_tasksuspend()\n");
        return 0;
}

// #undef current_task
// task_t current_task(void);
// task_t current_task(void)
// {
//         kprintf("not implemented: current_task()\n");
//         return 0;
// }

#undef task_reference
void task_reference(task_t task);
void
task_reference(
    task_t      task)
{
        kprintf("not implemented: task_reference()\n");
}

#if 0
/* 
 * This routine is called always with task lock held.
 * And it returns a thread handle without reference as the caller
 * operates on it under the task lock held.
 */
thread_t
task_findtid(task_t task, uint64_t tid)
{
        kprintf("not implemented: task_findtid()\n");
        return 0;
}
#endif


#if CONFIG_MACF_MACH
/*
 * Protect 2 task labels against modification by adding a reference on
 * both label handles. The locks do not actually have to be held while
 * using the labels as only labels with one reference can be modified
 * in place.
 */

void
tasklabel_lock2(
    task_t a,
    task_t b)
{
        kprintf("not implemented: tasklabel_lock2()\n");
}

void
tasklabel_unlock2(
    task_t a,
    task_t b)
{
        kprintf("not implemented: tasklabel_unlock2()\n");
}

void
mac_task_label_update_internal(
    struct label    *pl,
    struct task *task)
{
        kprintf("not implemented: mac_task_label_update_internal()\n");
}

void
mac_task_label_modify(
    struct task *task,
    void        *arg,
    void (*f)   (struct label *l, void *arg))
{
        kprintf("not implemented: mac_task_label_modify()\n");
}

struct label *
mac_task_get_label(struct task *task)
{
        kprintf("not implemented: mac_task_get_label()\n");
        return 0;
}
#endif

kern_return_t task_violated_guard(mach_exception_code_t code, mach_exception_subcode_t subcode, void *reason) {
	kprintf("not implemented: task_violated_guard()\n");
	// for now, we emulate the behavior of `task_violated_guard` when it's called on PID 1 (the init process),
	// which is to effectively ignore the exception
	return KERN_NOT_SUPPORTED;
};

kern_return_t task_inspect(task_inspect_t task_insp, task_inspect_flavor_t flavor, task_inspect_info_t info_out, mach_msg_type_number_t* size_in_out) {
	kprintf("not implemented: task_inspect()\n");
	return KERN_NOT_SUPPORTED;
};
