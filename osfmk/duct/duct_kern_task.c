/*
Copyright (c) 2014-2017, Wenqi Chen
Copyright (C) 2018 Lubos Dolezel

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
#include "duct_kern_task.h"
#include "duct_kern_zalloc.h"
#include "duct_vm_map.h"
// #include "duct_machine_routines.h"

#include <kern/mach_param.h>
#include <kern/task.h>
#include <kern/locks.h>
#include <kern/ipc_tt.h>

#include "duct_post_xnu.h"

#include <vm/vm_map.h>
#include <darling/task_registry.h>
#define current linux_current
#include <linux/mm.h>
#include <linux/sched.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
#include <linux/sched/mm.h>
#include <linux/sched/cputime.h>
#endif

task_t            kernel_task;
zone_t            task_zone;
lck_attr_t      task_lck_attr;
lck_grp_t       task_lck_grp;
lck_grp_attr_t  task_lck_grp_attr;

extern void duct_vm_map_deallocate(vm_map_t map);

void duct_task_init (void)
{
#if defined (__DARLING__)
#else
        lck_grp_attr_setdefault(&task_lck_grp_attr);
        lck_grp_init(&task_lck_grp, "task", &task_lck_grp_attr);
        lck_attr_setdefault(&task_lck_attr);
#endif
        lck_mtx_init(&tasks_threads_lock, &task_lck_grp, &task_lck_attr);

#if defined (__DARLING__)
#else
    #if CONFIG_EMBEDDED
        lck_mtx_init(&task_watch_mtx, &task_lck_grp, &task_lck_attr);
    #endif /* CONFIG_EMBEDDED */
#endif


        task_zone = duct_zinit(
                sizeof(struct task),
                task_max * sizeof(struct task),
                TASK_CHUNK * sizeof(struct task),
                "tasks");

        duct_zone_change(task_zone, Z_NOENCRYPT, TRUE);

        // init_task_ledgers();
        /*
         * Create the kernel task as the first task.
         */
    #ifdef __LP64__
        if (duct_task_create_internal(TASK_NULL, FALSE, TRUE, &kernel_task, NULL) != KERN_SUCCESS)
    #else
        if (duct_task_create_internal(TASK_NULL, FALSE, FALSE, &kernel_task, NULL) != KERN_SUCCESS)
    #endif
            panic("task_init\n");


#if defined (__DARLING__)
#else
        duct_vm_map_deallocate(kernel_task->map);
        kernel_task->map = kernel_map;
        lck_spin_init(&dead_task_statistics_lock, &task_lck_grp, &task_lck_attr);
#endif

}

extern void pth_proc_hashinit(task_t t);
extern void pth_proc_hashdelete(task_t t);

void duct_task_destroy(task_t task)
{
        if (task == TASK_NULL) {
                return;
        }

        ipc_space_terminate (task->itk_space);
        pth_proc_hashdelete(task);
        task_deallocate(task);

}

kern_return_t duct_task_create_internal (task_t parent_task, boolean_t inherit_memory, boolean_t is_64bit, task_t * child_task, struct task_struct* ltask)
{
        task_t            new_task;

    #if defined (__DARLING__)
    #else
        vm_shared_region_t    shared_region;

        ledger_t        ledger = NULL;
    #endif

        new_task = (task_t) duct_zalloc(task_zone);
        pth_proc_hashinit(new_task);

        // printk (KERN_NOTICE "task create internal's new task: 0x%x", (unsigned int) new_task);

        if (new_task == TASK_NULL)
            return(KERN_RESOURCE_SHORTAGE);

        /* one ref for just being alive; one for our caller */
        new_task->ref_count = 2;

        // /* allocate with active entries */
        // assert(task_ledger_template != NULL);
        // if ((ledger = ledger_instantiate(task_ledger_template,
        //         LEDGER_CREATE_ACTIVE_ENTRIES)) == NULL) {
        //     zfree(task_zone, new_task);
        //     return(KERN_RESOURCE_SHORTAGE);
        // }
        // new_task->ledger = ledger;


    #if defined (__DARLING__)
    #else
        // /* if inherit_memory is true, parent_task MUST not be NULL */
        // if (inherit_memory)
        //     new_task->map = vm_map_fork(ledger, parent_task->map);
        // else
        //     new_task->map = vm_map_create(pmap_create(ledger, 0, is_64bit),
        //             (vm_map_offset_t)(VM_MIN_ADDRESS),
        //             (vm_map_offset_t)(VM_MAX_ADDRESS), TRUE);
        //
        // /* Inherit memlock limit from parent */
        // if (parent_task)
        //     vm_map_set_user_wire_limit(new_task->map, (vm_size_t)parent_task->map->user_wire_limit);
    #endif
    //
        lck_mtx_init(&new_task->lock, &task_lck_grp, &task_lck_attr);
        queue_init(&new_task->threads);
        new_task->suspend_count = 0;
        new_task->thread_count = 0;
        new_task->active_thread_count = 0;
        new_task->user_stop_count = 0;
        // new_task->role = TASK_UNSPECIFIED;
        new_task->active = TRUE;
        new_task->halting = FALSE;
        new_task->user_data = NULL;
        new_task->faults = 0;
        new_task->cow_faults = 0;
        new_task->pageins = 0;
        new_task->messages_sent = 0;
        new_task->messages_received = 0;
        new_task->syscalls_mach = 0;
        new_task->priv_flags = 0;
        new_task->syscalls_unix=0;
        new_task->c_switch = new_task->p_switch = new_task->ps_switch = 0;
        // new_task->taskFeatures[0] = 0;                /* Init task features */
        // new_task->taskFeatures[1] = 0;                /* Init task features */

        // zinfo_task_init(new_task);

    // #ifdef MACH_BSD
    //     new_task->bsd_info = NULL;
    // #endif /* MACH_BSD */

    // #if defined(__i386__) || defined(__x86_64__)
    //     new_task->i386_ldt = 0;
    //     new_task->task_debug = NULL;
    // #endif


        queue_init(&new_task->semaphore_list);
        // queue_init(&new_task->lock_set_list);
        new_task->semaphores_owned = 0;
        // new_task->lock_sets_owned = 0;

        if (ltask != NULL)
        {
            new_task->map = duct_vm_map_create(ltask);
        }

    #if CONFIG_MACF_MACH
        new_task->label = labelh_new(1);
        mac_task_label_init (&new_task->maclabel);
    #endif

        ipc_task_init(new_task, parent_task);

        new_task->total_user_time = 0;
        new_task->total_system_time = 0;

        new_task->vtimers = 0;

        new_task->shared_region = NULL;

        new_task->affinity_space = NULL;
    //
    // #if CONFIG_COUNTERS
    //     new_task->t_chud = 0U;
    // #endif
    //
    //     new_task->pidsuspended = FALSE;
    //     new_task->frozen = FALSE;
    //     new_task->rusage_cpu_flags = 0;
    //     new_task->rusage_cpu_percentage = 0;
    //     new_task->rusage_cpu_interval = 0;
    //     new_task->rusage_cpu_deadline = 0;
    //     new_task->rusage_cpu_callt = NULL;
    //     new_task->proc_terminate = 0;
    // #if CONFIG_EMBEDDED
    //     queue_init(&new_task->task_watchers);
    //     new_task->appstate = TASK_APPSTATE_ACTIVE;
    //     new_task->num_taskwatchers  = 0;
    //     new_task->watchapplying  = 0;
    // #endif /* CONFIG_EMBEDDED */

    //     new_task->uexc_range_start = new_task->uexc_range_size = new_task->uexc_handler = 0;
    //
        if (parent_task != TASK_NULL) {
                new_task->sec_token     = parent_task->sec_token;
                new_task->audit_token   = parent_task->audit_token;

                // printk (KERN_NOTICE "- new task audit[5]: 0x%x\n", new_task->audit_token.val[5]);

            #if defined (__DARLING__)
            #else
                /* inherit the parent's shared region */
                // shared_region = vm_shared_region_get(parent_task);
                // vm_shared_region_set(new_task, shared_region);
            #endif
    //
    //         if(task_has_64BitAddr(parent_task))
    //             task_set_64BitAddr(new_task);
    //         new_task->all_image_info_addr = parent_task->all_image_info_addr;
    //         new_task->all_image_info_size = parent_task->all_image_info_size;
    //
    // #if defined (__DARLING__)
    // #else
    // // #if defined(__i386__) || defined(__x86_64__)
    // //         if (inherit_memory && parent_task->i386_ldt)
    // //             new_task->i386_ldt = user_ldt_copy(parent_task->i386_ldt);
    // // #endif
    //         if (inherit_memory && parent_task->affinity_space)
    //             task_affinity_create(parent_task, new_task);
    // #endif
    //
    // #if defined (__DARLING__)
    // #else
    //         new_task->pset_hint = parent_task->pset_hint = task_choose_pset(parent_task);
    // #endif
    //
    //         new_task->policystate = parent_task->policystate;
    //         /* inherit the self action state */
    //         new_task->appliedstate = parent_task->appliedstate;
    //         new_task->ext_policystate = parent_task->ext_policystate;
    // #if NOTYET
    //         /* till the child lifecycle is cleared do not inherit external action */
    //         new_task->ext_appliedstate = parent_task->ext_appliedstate;
    // #else
    //         new_task->ext_appliedstate = default_task_null_policy;
    // #endif
        }
        else {
                new_task->sec_token     = KERNEL_SECURITY_TOKEN;
                new_task->audit_token   = KERNEL_AUDIT_TOKEN;

    // // #ifdef __LP64__
    // //         if(is_64bit)
    // //             task_set_64BitAddr(new_task);
    // // #endif
    //         new_task->all_image_info_addr = (mach_vm_address_t)0;
    //         new_task->all_image_info_size = (mach_vm_size_t)0;
    //
    //         new_task->pset_hint = PROCESSOR_SET_NULL;
    //         new_task->policystate = default_task_proc_policy;
    //         new_task->ext_policystate = default_task_proc_policy;
    //         new_task->appliedstate = default_task_null_policy;
    //         new_task->ext_appliedstate = default_task_null_policy;
        }

    //
    //     if (kernel_task == TASK_NULL) {
    //         new_task->priority = BASEPRI_KERNEL;
    //         new_task->max_priority = MAXPRI_KERNEL;
    //     }
    //     else {
    //         new_task->priority = BASEPRI_DEFAULT;
    //         new_task->max_priority = MAXPRI_USER;
    //     }
    //
    //     bzero(&new_task->extmod_statistics, sizeof(new_task->extmod_statistics));
    //     new_task->task_timer_wakeups_bin_1 = new_task->task_timer_wakeups_bin_2 = 0;
    //
    //     lck_mtx_lock(&tasks_threads_lock);
    //     queue_enter(&tasks, new_task, task_t, tasks);
    //     tasks_count++;
    //     lck_mtx_unlock(&tasks_threads_lock);
    //
        #if defined (__DARLING__)
        #else
            // if (vm_backing_store_low && parent_task != NULL)
            //     new_task->priv_flags |= (parent_task->priv_flags&VM_BACKING_STORE_PRIV);
        #endif

        ipc_task_enable (new_task);

        *child_task = new_task;
        return(KERN_SUCCESS);
}


#undef duct_current_task
task_t duct_current_task (void);

task_t duct_current_task (void)
{
        return (current_task_fast ());
}

void task_reference_wrapper(task_t t)
{
	task_reference(t);
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
typedef u64 cputime_t;

// On new kernels, cputime is in nanoseconds
#define cputime_to_usecs(v) ((v) / 1000)
#endif

// The following two functions are copied from kernel/sched/cputime.c
// because the are not made available to kernel modules
/*
 * Accumulate raw cputime values of dead tasks (sig->[us]time) and live
 * tasks (sum on group iteration) belonging to @tsk's group.
 */
void __thread_group_cputime(struct task_struct *tsk, struct task_cputime *times)
{
        struct signal_struct *sig = tsk->signal;
        cputime_t utime, stime;
        struct task_struct *t;
        unsigned int seq, nextseq;
        unsigned long flags;

        /*
         * Update current task runtime to account pending time since last
         * scheduler action or thread_group_cputime() call. This thread group
         * might have other running tasks on different CPUs, but updating
         * their runtime can affect syscall performance, so we skip account
         * those pending times and rely only on values updated on tick or
         * other scheduler action.
         */
#if 0
        if (same_thread_group(linux_current, tsk))
                (void) task_sched_runtime(linux_current);
#endif

        rcu_read_lock();
        /* Attempt a lockless read on the first round. */
        nextseq = 0;
        do {
                seq = nextseq;
                flags = read_seqbegin_or_lock_irqsave(&sig->stats_lock, &seq);
                times->utime = sig->utime;
                times->stime = sig->stime;
                times->sum_exec_runtime = sig->sum_sched_runtime;

                for_each_thread(tsk, t) {
                        // task_cputime(t, &utime, &stime); // not inlined under certain configs (and not exported)
                        utime = t->utime;
                        stime = t->stime;
                        times->utime += utime;
                        times->stime += stime;
                        times->sum_exec_runtime += t->se.sum_exec_runtime;
                }
                /* If lockless access failed, take the lock. */
                nextseq = 1;
        } while (need_seqretry(&sig->stats_lock, seq));
        done_seqretry_irqrestore(&sig->stats_lock, seq, flags);
        rcu_read_unlock();
}


static void __thread_group_cputime_adjusted(struct task_struct *p, cputime_t *ut, cputime_t *st)
{
        struct task_cputime cputime;

        __thread_group_cputime(p, &cputime);

        *ut = cputime.utime;
        *st = cputime.stime;
}

#ifndef TASK_VM_INFO

// Copied from newer XNU
#define TASK_VM_INFO		22
#define TASK_VM_INFO_PURGEABLE	23
struct task_vm_info {
        mach_vm_size_t  virtual_size;	    /* virtual memory size (bytes) */
	integer_t	region_count;	    /* number of memory regions */
	integer_t	page_size;
        mach_vm_size_t  resident_size;	    /* resident memory size (bytes) */
        mach_vm_size_t  resident_size_peak; /* peak resident size (bytes) */

	mach_vm_size_t	device;
	mach_vm_size_t	device_peak;
	mach_vm_size_t	internal;
	mach_vm_size_t	internal_peak;
	mach_vm_size_t	external;
	mach_vm_size_t	external_peak;
	mach_vm_size_t	reusable;
	mach_vm_size_t	reusable_peak;
	mach_vm_size_t	purgeable_volatile_pmap;
	mach_vm_size_t	purgeable_volatile_resident;
	mach_vm_size_t	purgeable_volatile_virtual;
	mach_vm_size_t	compressed;
	mach_vm_size_t	compressed_peak;
	mach_vm_size_t	compressed_lifetime;
};
typedef struct task_vm_info	task_vm_info_data_t;
typedef struct task_vm_info	*task_vm_info_t;
#define TASK_VM_INFO_COUNT	((mach_msg_type_number_t) \
		(sizeof (task_vm_info_data_t) / sizeof (natural_t)))
#endif

kern_return_t
task_info(
    task_t                  task,
    task_flavor_t           flavor,
    task_info_t             task_info_out,
    mach_msg_type_number_t  *task_info_count)
{
	kern_return_t error = KERN_SUCCESS;

	switch (flavor) {
	case TASK_DYLD_INFO:
	{
		task_dyld_info_t info;

		/*
		 * We added the format field to TASK_DYLD_INFO output.  For
		 * temporary backward compatibility, accept the fact that
		 * clients may ask for the old version - distinquished by the
		 * size of the expected result structure.
		 */
#define TASK_LEGACY_DYLD_INFO_COUNT \
		offsetof(struct task_dyld_info, all_image_info_format)/sizeof(natural_t)

		if (*task_info_count < TASK_LEGACY_DYLD_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		// DARLING:
		// This call may block, waiting for Darling to provide this information
		// shortly after startup.

		struct task_struct* ltask = task->map->linux_task;
		if (!ltask) {
			error = KERN_FAILURE;
			break;
		}
		darling_task_get_dyld_info(ltask->pid,
				&task->all_image_info_addr,
				&task->all_image_info_size);

		info = (task_dyld_info_t)task_info_out;
		info->all_image_info_addr = task->all_image_info_addr;
		info->all_image_info_size = task->all_image_info_size;

		/* only set format on output for those expecting it */
		if (*task_info_count >= TASK_DYLD_INFO_COUNT) {
			struct mm_struct* mm = get_task_mm(ltask);
			
			if (mm != NULL)
			{
				bool is64bit = mm->task_size >= 0x100000000ull;
				info->all_image_info_format = is64bit ?
					                 TASK_DYLD_ALL_IMAGE_INFO_64 : 
					                 TASK_DYLD_ALL_IMAGE_INFO_32 ;
				mmput(mm);
			}
			else
				info->all_image_info_format = 0;
			*task_info_count = TASK_DYLD_INFO_COUNT;
		} else {
			*task_info_count = TASK_LEGACY_DYLD_INFO_COUNT;
		}
		break;
	}
	case TASK_BASIC_INFO_64:
	case TASK_BASIC_INFO_32:
	{
		struct task_struct* ltask = task->map->linux_task;

		if (!ltask)
		{
			error = KERN_FAILURE;
			break;
		}

		struct mm_struct* mm = get_task_mm(ltask);
		unsigned long anon, file, shmem = 0, total_rss = 0;
		//cputime_t utime, stime;
		uint64_t utimeus, stimeus;

		if (mm != NULL)
		{
			anon = get_mm_counter(mm, MM_ANONPAGES);
			file = get_mm_counter(mm, MM_FILEPAGES);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0)
			shmem = get_mm_counter(mm, MM_SHMEMPAGES);
#endif

			total_rss = anon + file + shmem;
		}

		//__thread_group_cputime_adjusted(ltask, &utime, &stime);
		//utimeus = cputime_to_usecs(utime);
		//stimeus = cputime_to_usecs(stime);
		utimeus = stimeus = 0; // Dunno how this is accounted on Linux

		if (flavor == TASK_BASIC_INFO_64)
		{
			if (*task_info_count < TASK_BASIC_INFO_64_COUNT)
			{
				error = KERN_INVALID_ARGUMENT;
				break;
			}

			*task_info_count = TASK_BASIC_INFO_64_COUNT;

			struct task_basic_info_64* info = (struct task_basic_info_64*) task_info_out;

			info->suspend_count = task_is_stopped(ltask) ? 1 : 0;
			info->virtual_size = mm ? (PAGE_SIZE * mm->total_vm) : 0;
			info->resident_size = PAGE_SIZE * total_rss;
			info->user_time.seconds = utimeus / USEC_PER_SEC;
			info->user_time.microseconds = utimeus % USEC_PER_SEC;
			info->system_time.seconds = stimeus / USEC_PER_SEC;
			info->system_time.microseconds = stimeus % USEC_PER_SEC;
			info->policy = 0;
		}
		else
		{
			if (*task_info_count < TASK_BASIC_INFO_32_COUNT)
			{
				error = KERN_INVALID_ARGUMENT;
				break;
			}

			*task_info_count = TASK_BASIC_INFO_32_COUNT;

			struct task_basic_info_32* info = (struct task_basic_info_32*) task_info_out;

			info->suspend_count = task_is_stopped(ltask) ? 1 : 0;
			info->virtual_size = mm ? (PAGE_SIZE * mm->total_vm) : 0;
			info->resident_size = PAGE_SIZE * total_rss;
			info->user_time.seconds = utimeus / USEC_PER_SEC;
			info->user_time.microseconds = utimeus % USEC_PER_SEC;
			info->system_time.seconds = stimeus / USEC_PER_SEC;
			info->system_time.microseconds = stimeus % USEC_PER_SEC;
			info->policy = 0;
		}

		if (mm != NULL)
			mmput(mm);
		error = KERN_SUCCESS;
		break;
	}
	case TASK_THREAD_TIMES_INFO:
	{
		struct task_struct* ltask = task->map->linux_task;
		cputime_t utime, stime;
		uint64_t utimeus, stimeus;
		task_thread_times_info_data_t* info = (task_thread_times_info_data_t*) task_info_out;

		if (*task_info_count < TASK_THREAD_TIMES_INFO_COUNT)
		{
			error = KERN_INVALID_ARGUMENT;
			break;
		}
		if (ltask == NULL)
		{
			error = KERN_FAILURE;
			break;
		}

		*task_info_count = TASK_THREAD_TIMES_INFO_COUNT;

		__thread_group_cputime_adjusted(ltask, &utime, &stime);

		utimeus = cputime_to_usecs(utime);
		stimeus = cputime_to_usecs(stime);

		info->user_time.seconds = utimeus / USEC_PER_SEC;
		info->user_time.microseconds = utimeus % USEC_PER_SEC;
		info->system_time.seconds = stimeus / USEC_PER_SEC;
		info->system_time.microseconds = stimeus % USEC_PER_SEC;

		error = KERN_SUCCESS;
		break;
	}
	case TASK_VM_INFO:
	{
		if (*task_info_count < 32)
		{
			// kprintf("Bad task_info_count, got %d, expected %d\n", *task_info_count, TASK_VM_INFO_COUNT);
			error = KERN_INVALID_ARGUMENT;
			break;
		}
		else if (*task_info_count > TASK_VM_INFO_COUNT)
			*task_info_count = TASK_VM_INFO_COUNT;

		struct task_struct* ltask = task->map->linux_task;

		if (!ltask)
		{
			error = KERN_FAILURE;
			break;
		}

		struct mm_struct* mm = get_task_mm(ltask);

		task_vm_info_data_t* out = (task_vm_info_data_t*) task_info_out;
		memset(out, 0, sizeof(*task_info_count));

		unsigned long anon, file, shmem = 0, total_rss;
		
		if (mm != NULL)
		{
			anon = get_mm_counter(mm, MM_ANONPAGES);
			file = get_mm_counter(mm, MM_FILEPAGES);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,5,0)
			shmem = get_mm_counter(mm, MM_SHMEMPAGES);
#endif

			total_rss = anon + file + shmem;
		}
		else
			total_rss = 0;

		out->page_size = PAGE_SIZE;
		// out->max_address = mm->task_size;
		out->resident_size = out->resident_size_peak = total_rss * PAGE_SIZE;

		if (mm != NULL)
			out->virtual_size = mm->total_vm * PAGE_SIZE;
		// TODO: There are more fields...

		if (mm != NULL)
			mmput(mm);

		error = KERN_SUCCESS;
		break;
	}
	default:
		kprintf("not implemented: task_info(flavor=%d)\n", flavor);
		error = KERN_FAILURE;
	}

	return error;
}
