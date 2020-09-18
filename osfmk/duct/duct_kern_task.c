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
#include <kern/sync_sema.h>
#include <ipc/ipc_hash.h>

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

#include <darling/procs.h>

task_t            kernel_task;
zone_t            task_zone;
lck_attr_t      task_lck_attr;
lck_grp_t       task_lck_grp;
lck_grp_attr_t  task_lck_grp_attr;

os_refgrp_decl(static, task_refgrp, "task", NULL);

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
	task_deallocate(kernel_task);


#if defined (__DARLING__)
	kernel_task->map = kernel_map;
#else
        duct_vm_map_deallocate(kernel_task->map);
        kernel_task->map = kernel_map;
        lck_spin_init(&dead_task_statistics_lock, &task_lck_grp, &task_lck_attr);
#endif

}

void duct_task_destroy(task_t task)
{
        if (task == TASK_NULL) {
                return;
        }

        if (task->vchroot_path)
            free_page((unsigned long) task->vchroot_path);

        if (task->bsd_info) {
          darling_proc_destroy(task->bsd_info);
        }

        semaphore_destroy_all(task);
        ipc_space_terminate (task->itk_space);
        task_deallocate(task);

}

kern_return_t duct_task_create_internal (task_t parent_task, boolean_t inherit_memory, boolean_t is_64bit, task_t * child_task, struct task_struct* ltask)
{
	// this function is pretty much a heavy trim of the real `task_create_internal`,
	// with a few Darling-specific additions
	task_t new_task;

	new_task = (task_t) zalloc(task_zone);

	if (new_task == TASK_NULL) {
		return KERN_RESOURCE_SHORTAGE;
	}

	// take advantadge of C99 and initialize everything to zero
	// allows us to eliminate a lot of unnecessary initialization
	*new_task = (struct task){0};

	/* one ref for just being alive; one for our caller */
	os_ref_init_count(&new_task->ref_count, &task_refgrp, 2);

#if defined(CONFIG_SCHED_MULTIQ)
	new_task->sched_group = sched_group_create();
#endif

	lck_mtx_init(&new_task->lock, &task_lck_grp, &task_lck_attr);
	queue_init(&new_task->threads);
	new_task->active = TRUE;
	new_task->returnwait_inheritor = current_thread();

	queue_init(&new_task->semaphore_list);

#ifdef __DARLING__
	if (ltask != NULL && ltask->mm != NULL) {
		new_task->map = duct_vm_map_create(ltask);
	}
#endif

	ipc_task_init(new_task, parent_task);

	if (parent_task != TASK_NULL) {
		new_task->sec_token = parent_task->sec_token;
		new_task->audit_token = parent_task->audit_token;
	} else {
		new_task->sec_token = KERNEL_SECURITY_TOKEN;
		new_task->audit_token = KERNEL_AUDIT_TOKEN;
	}

	ipc_task_enable(new_task);

	// Darling-specific code
	new_task->vchroot_path = (char*)__get_free_page(GFP_KERNEL);

	*child_task = new_task;
	return KERN_SUCCESS;
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
	case MACH_TASK_BASIC_INFO:
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

			info->suspend_count = task->user_stop_count;
			info->virtual_size = mm ? (PAGE_SIZE * mm->total_vm) : 0;
			info->resident_size = PAGE_SIZE * total_rss;
			info->user_time.seconds = utimeus / USEC_PER_SEC;
			info->user_time.microseconds = utimeus % USEC_PER_SEC;
			info->system_time.seconds = stimeus / USEC_PER_SEC;
			info->system_time.microseconds = stimeus % USEC_PER_SEC;
			info->policy = 0;
		}
		else if (flavor == TASK_BASIC_INFO_32)
		{
			if (*task_info_count < TASK_BASIC_INFO_32_COUNT)
			{
				error = KERN_INVALID_ARGUMENT;
				break;
			}

			*task_info_count = TASK_BASIC_INFO_32_COUNT;

			struct task_basic_info_32* info = (struct task_basic_info_32*) task_info_out;

			info->suspend_count = task->user_stop_count;
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
			if (*task_info_count < MACH_TASK_BASIC_INFO_COUNT)
			{
				error = KERN_INVALID_ARGUMENT;
				break;
			}

			*task_info_count = MACH_TASK_BASIC_INFO_COUNT;
			struct mach_task_basic_info* info = (struct mach_task_basic_info*) task_info_out;

			info->suspend_count = task->user_stop_count;
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

// <copied from="xnu://6153.61.1/osfmk/kern/task.c">

kern_return_t
task_info_from_user(
	mach_port_t             task_port,
	task_flavor_t           flavor,
	task_info_t             task_info_out,
	mach_msg_type_number_t  *task_info_count)
{
	task_t task;
	kern_return_t ret;

	if (flavor == TASK_DYLD_INFO) {
		task = convert_port_to_task(task_port);
	} else {
		task = convert_port_to_task_name(task_port);
	}

	ret = task_info(task, flavor, task_info_out, task_info_count);

	task_deallocate(task);

	return ret;
}

// </copied>

void task_suspension_token_deallocate(task_suspension_token_t token)
{
    task_deallocate((task_t) token);
}

bool task_is_driver(task_t task) {
	kprintf("not implemented: task_is_driver\n");
	return FALSE;
}

boolean_t task_is_app(task_t t)
{
    kprintf("not implemented: task_is_app\n");
    return FALSE;
}

boolean_t task_is_daemon(task_t t)
{
    kprintf("not implemented: task_is_daemon\n");
    return FALSE;
}


void task_port_notify(mach_msg_header_t* m)
{
    kprintf("not implemented: task_port_notify\n");
}

boolean_t task_suspension_notify(mach_msg_header_t* m)
{
    kprintf("not implemented: task_suspension_notify\n");
    return FALSE;
}

#if 0
kern_return_t task_exception_notify(exception_type_t exc, mach_exception_data_type_t code, mach_exception_data_type_t subcode)
{
    kprintf("not implemented: task_exception_notify\n");
    return KERN_FAILURE;
}
#endif

/* Placeholders for the task set/get voucher interfaces */
kern_return_t 
task_get_mach_voucher(
	task_t			task,
	mach_voucher_selector_t __unused which,
	ipc_voucher_t		*voucher)
{
	if (TASK_NULL == task)
		return KERN_INVALID_TASK;

	*voucher = NULL;
	return KERN_SUCCESS;
}

kern_return_t 
task_set_mach_voucher(
	task_t			task,
	ipc_voucher_t		__unused voucher)
{
	if (TASK_NULL == task)
		return KERN_INVALID_TASK;

	return KERN_SUCCESS;
}

kern_return_t
task_swap_mach_voucher(
	task_t			task,
	ipc_voucher_t		new_voucher,
	ipc_voucher_t		*in_out_old_voucher)
{
	if (TASK_NULL == task)
		return KERN_INVALID_TASK;

	*in_out_old_voucher = new_voucher;
	return KERN_SUCCESS;
}

kern_return_t
task_get_dyld_image_infos(__unused task_t task,
                          __unused dyld_kernel_image_info_array_t * dyld_images,
                          __unused mach_msg_type_number_t * dyld_imagesCnt)
{
    printf("NOT IMPLEMENTED: task_get_dyld_image_infos\n");
	return KERN_NOT_SUPPORTED;
}

kern_return_t
task_register_dyld_image_infos(task_t task,
                               dyld_kernel_image_info_array_t infos_copy,
                               mach_msg_type_number_t infos_len)
{
    printf("NOT IMPLEMENTED: task_register_dyld_image_infos\n");
	return KERN_NOT_SUPPORTED;
}

kern_return_t
task_unregister_dyld_image_infos(task_t task,
                                 dyld_kernel_image_info_array_t infos_copy,
                                 mach_msg_type_number_t infos_len)
{
	printf("NOT IMPLEMENTED: task_unregister_dyld_image_infos\n");
	return KERN_NOT_SUPPORTED;
}

kern_return_t
task_register_dyld_set_dyld_state(__unused task_t task,
                                  __unused uint8_t dyld_state)
{
    printf("NOT IMPLEMENTED: task_register_dyld_set_dyld_state\n");
	return KERN_NOT_SUPPORTED;
}

kern_return_t
task_register_dyld_shared_cache_image_info(task_t task,
                                           dyld_kernel_image_info_t cache_img,
                                           __unused boolean_t no_cache,
                                           __unused boolean_t private_cache)
{
    printf("NOT IMPLEMENTED: task_register_dyld_shared_cache_image_info\n");
	return KERN_NOT_SUPPORTED;
}

kern_return_t
task_generate_corpse(
	task_t task,
	ipc_port_t *corpse_task_port)
{
    printf("NOT IMPLEMENTED: task_generate_corpse\n");
	return KERN_NOT_SUPPORTED;
}

kern_return_t
task_purgable_info(
	task_t			task,
	task_purgable_info_t	*stats)
{
    printf("NOT IMPLEMENTED: task_purgable_info\n");
	return KERN_NOT_SUPPORTED;
}

static void task_hold_locked(task_t task);
static void task_wait_locked(task_t task, boolean_t until_not_runnable) {}
static void task_release_locked(task_t task);


// The following stuff is copied from osfmk/kern/task.c

#define TASK_HOLD_NORMAL	0
#define TASK_HOLD_PIDSUSPEND	1
#define TASK_HOLD_LEGACY	2
#define TASK_HOLD_LEGACY_ALL	3

static kern_return_t
place_task_hold    (
	task_t task,
	int mode)
{    
	if (!task->active && !task_is_a_corpse(task)) {
		return (KERN_FAILURE);
	}

	/* Return success for corpse task */
	if (task_is_a_corpse(task)) {
		return KERN_SUCCESS;
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_IPC,MACH_TASK_SUSPEND) | DBG_FUNC_NONE,
	    task_pid(task), ((thread_t)queue_first(&task->threads))->thread_id,
	    task->user_stop_count, task->user_stop_count + 1, 0);

#if MACH_ASSERT
	current_task()->suspends_outstanding++;
#endif

	if (mode == TASK_HOLD_LEGACY)
		task->legacy_stop_count++;

	if (task->user_stop_count++ > 0) {
		/*
		 *	If the stop count was positive, the task is
		 *	already stopped and we can exit.
		 */
		return (KERN_SUCCESS);
	}

	/*
	 * Put a kernel-level hold on the threads in the task (all
	 * user-level task suspensions added together represent a
	 * single kernel-level hold).  We then wait for the threads
	 * to stop executing user code.
	 */
	task_hold_locked(task);
	task_wait_locked(task, FALSE);
	
	return (KERN_SUCCESS);
}

static kern_return_t
release_task_hold    (
	task_t		task,
	int           		mode)
{
	boolean_t release = FALSE;
    
	if (!task->active && !task_is_a_corpse(task)) {
		return (KERN_FAILURE);
	}

	/* Return success for corpse task */
	if (task_is_a_corpse(task)) {
		return KERN_SUCCESS;
	}
	
	if (mode == TASK_HOLD_PIDSUSPEND) {
	    if (task->pidsuspended == FALSE) {
		    return (KERN_FAILURE);
	    }
	    task->pidsuspended = FALSE;
	}

	if (task->user_stop_count > (task->pidsuspended ? 1 : 0)) {

		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    MACHDBG_CODE(DBG_MACH_IPC,MACH_TASK_RESUME) | DBG_FUNC_NONE,
		    task_pid(task), ((thread_t)queue_first(&task->threads))->thread_id,
		    task->user_stop_count, mode, task->legacy_stop_count);

#if MACH_ASSERT
		/*
		 * This is obviously not robust; if we suspend one task and then resume a different one,
		 * we'll fly under the radar. This is only meant to catch the common case of a crashed
		 * or buggy suspender.
		 */
		current_task()->suspends_outstanding--;
#endif

		if (mode == TASK_HOLD_LEGACY_ALL) {
			if (task->legacy_stop_count >= task->user_stop_count) {
				task->user_stop_count = 0;
				release = TRUE;
			} else {
				task->user_stop_count -= task->legacy_stop_count;
			}
			task->legacy_stop_count = 0;
		} else {
			if (mode == TASK_HOLD_LEGACY && task->legacy_stop_count > 0)
				task->legacy_stop_count--;
			if (--task->user_stop_count == 0)
				release = TRUE;
		}
	}
	else {
		return (KERN_FAILURE);
	}

	/*
	 *	Release the task if necessary.
	 */
	if (release)
		task_release_locked(task);
		
    return (KERN_SUCCESS);
}

/*
 *	task_release_locked:
 *
 *	Release a kernel hold on a task.
 *
 * 	CONDITIONS: the task is locked and active
 */
void
task_release_locked(
	task_t		task)
{
	thread_t	thread;

	assert(task->active);
	assert(task->suspend_count > 0);

	if (--task->suspend_count > 0)
		return;

	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		thread_mtx_lock(thread);
		thread_release(thread);
		thread_mtx_unlock(thread);
	}
}

/*
 *	task_release:
 *
 *	Same as the internal routine above, except that it must lock
 *	and verify that the task is active.
 *
 * 	CONDITIONS: The caller holds a reference to the task
 */
kern_return_t
task_release(
	task_t		task)
{
	if (task == TASK_NULL)
		return (KERN_INVALID_ARGUMENT);

	task_lock(task);

	if (!task->active) {
		task_unlock(task);

		return (KERN_FAILURE);
	}

	task_release_locked(task);
	task_unlock(task);

	return (KERN_SUCCESS);
}


/*
 *	task_suspend:
 *
 *	Implement an (old-fashioned) user-level suspension on a task.
 *
 *	Because the user isn't expecting to have to manage a suspension
 *	token, we'll track it for him in the kernel in the form of a naked
 *	send right to the task's resume port.  All such send rights
 *	account for a single suspension against the task (unlike task_suspend2()
 *	where each caller gets a unique suspension count represented by a
 *	unique send-once right).
 *
 * Conditions:
 * 	The caller holds a reference to the task
 */
kern_return_t
task_suspend(
	task_t		task)
{
	kern_return_t                   kr;
	mach_port_t                     port;
	mach_port_name_t                name;

	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);

	/*
	 * place a legacy hold on the task.
	 */
	kr = place_task_hold(task, TASK_HOLD_LEGACY);
	if (kr != KERN_SUCCESS) {
		task_unlock(task);
		return kr;
	}

	/*
	 * Claim a send right on the task resume port, and request a no-senders
	 * notification on that port (if none outstanding).
	 */
	(void)ipc_kobject_make_send_lazy_alloc_port(&task->itk_resume,
	    (ipc_kobject_t)task, IKOT_TASK_RESUME);
	port = task->itk_resume;

	task_unlock(task);

	/*
	 * Copyout the send right into the calling task's IPC space.  It won't know it is there,
	 * but we'll look it up when calling a traditional resume.  Any IPC operations that
	 * deallocate the send right will auto-release the suspension.
	 */
	if ((kr = ipc_kmsg_copyout_object(current_task()->itk_space, ip_to_object(port),
		MACH_MSG_TYPE_MOVE_SEND, NULL, NULL, &name)) != KERN_SUCCESS) {
#ifndef __DARLING__
		printf("warning: %s(%d) failed to copyout suspension token for pid %d with error: %d\n",
				proc_name_address(current_task()->bsd_info), proc_pid(current_task()->bsd_info),
				task_pid(task), kr);
#endif
		return kr;
	}

	return kr;
}

/*
 *	task_resume:
 *		Release a user hold on a task.
 *		
 * Conditions:
 *		The caller holds a reference to the task
 */
kern_return_t 
task_resume(
	task_t	task)
{
	kern_return_t	 kr;
	mach_port_name_t resume_port_name;
	ipc_entry_t		 resume_port_entry;
	ipc_space_t		 space = current_task()->itk_space;

	if (task == TASK_NULL || task == kernel_task )
		return (KERN_INVALID_ARGUMENT);

	/* release a legacy task hold */
	task_lock(task);
	kr = release_task_hold(task, TASK_HOLD_LEGACY);
	task_unlock(task);

	is_write_lock(space);
	if (is_active(space) && IP_VALID(task->itk_resume) &&
	    ipc_hash_lookup(space, (ipc_object_t)task->itk_resume, &resume_port_name, &resume_port_entry) == TRUE) {
		/*
		 * We found a suspension token in the caller's IPC space. Release a send right to indicate that
		 * we are holding one less legacy hold on the task from this caller.  If the release failed,
		 * go ahead and drop all the rights, as someone either already released our holds or the task
		 * is gone.
		 */
		if (kr == KERN_SUCCESS)
			ipc_right_dealloc(space, resume_port_name, resume_port_entry);
		else
			ipc_right_destroy(space, resume_port_name, resume_port_entry, FALSE, 0);
		/* space unlocked */
	} else {
		is_write_unlock(space);
#ifndef __DARLING__
		if (kr == KERN_SUCCESS)
			printf("warning: %s(%d) performed out-of-band resume on pid %d\n",
			       proc_name_address(current_task()->bsd_info), proc_pid(current_task()->bsd_info),
			       task_pid(task));
#endif
	}

	return kr;
}

/*
 * Suspend the target task.
 * Making/holding a token/reference/port is the callers responsibility.
 */
kern_return_t
task_suspend_internal(task_t task)
{
	kern_return_t	 kr;
       
	if (task == TASK_NULL || task == kernel_task)
		return (KERN_INVALID_ARGUMENT);

	task_lock(task);
	kr = place_task_hold(task, TASK_HOLD_NORMAL);
	task_unlock(task);
	return (kr);
}

/*
 * Suspend the target task, and return a suspension token. The token
 * represents a reference on the suspended task.
 */
kern_return_t
task_suspend2(
	task_t			task,
	task_suspension_token_t *suspend_token)
{
	kern_return_t	 kr;
 
	kr = task_suspend_internal(task);
	if (kr != KERN_SUCCESS) {
		*suspend_token = TASK_NULL;
		return (kr);
	}

	/*
	 * Take a reference on the target task and return that to the caller
	 * as a "suspension token," which can be converted into an SO right to
	 * the now-suspended task's resume port.
	 */
	task_reference_internal(task);
	*suspend_token = task;

	return (KERN_SUCCESS);
}

/*
 * Resume the task
 * (reference/token/port management is caller's responsibility).
 */
kern_return_t
task_resume_internal(
	task_suspension_token_t		task)
{
	kern_return_t kr;

	if (task == TASK_NULL || task == kernel_task)
		return (KERN_INVALID_ARGUMENT);

	task_lock(task);
	kr = release_task_hold(task, TASK_HOLD_NORMAL);
	task_unlock(task);
	return (kr);
}

/*
 * Resume the task using a suspension token. Consumes the token's ref.
 */
kern_return_t
task_resume2(
	task_suspension_token_t		task)
{
	kern_return_t kr;

	kr = task_resume_internal(task);
	task_suspension_token_deallocate(task);

	return (kr);
}

/*
 *	task_hold_locked:
 *
 *	Suspend execution of the specified task.
 *	This is a recursive-style suspension of the task, a count of
 *	suspends is maintained.
 *
 * 	CONDITIONS: the task is locked and active.
 */
void
task_hold_locked(
	task_t		task)
{
	thread_t	thread;

	assert(task->active);

	if (task->suspend_count++ > 0)
		return;

	/*
	 *	Iterate through all the threads and hold them.
	 */
	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		thread_mtx_lock(thread);
		thread_hold(thread);
		thread_mtx_unlock(thread);
	}
}

/*
 *	task_hold:
 *
 *	Same as the internal routine above, except that is must lock
 *	and verify that the task is active.  This differs from task_suspend
 *	in that it places a kernel hold on the task rather than just a 
 *	user-level hold.  This keeps users from over resuming and setting
 *	it running out from under the kernel.
 *
 * 	CONDITIONS: the caller holds a reference on the task
 */
kern_return_t
task_hold(
	task_t		task)
{
	if (task == TASK_NULL)
		return (KERN_INVALID_ARGUMENT);

	task_lock(task);

	if (!task->active) {
		task_unlock(task);

		return (KERN_FAILURE);
	}

	task_hold_locked(task);
	task_unlock(task);

	return (KERN_SUCCESS);
}

kern_return_t
task_register_dyld_get_process_state(__unused task_t task,
                                     __unused dyld_kernel_process_info_t * dyld_process_state)
{
    printf("NOT IMPLEMENTED: task_register_dyld_get_process_state\n");
	return KERN_NOT_SUPPORTED;
}

// <copied from="xnu://6153.61.1/osfmk/kern/task.c" modified="true">

/*
 *	task_watchports_deallocate:
 *		Deallocate task watchport struct.
 *
 *	Conditions:
 *		Nothing locked.
 */
static void
task_watchports_deallocate(
	struct task_watchports *watchports)
{
	uint32_t portwatch_count = watchports->tw_elem_array_count;

	task_deallocate(watchports->tw_task);
	thread_deallocate(watchports->tw_thread);
	kfree(watchports, sizeof(struct task_watchports) + portwatch_count * sizeof(struct task_watchport_elem));
}

/*
 *	task_watchport_elem_deallocate:
 *		Deallocate task watchport element and release its ref on task_watchport.
 *
 *	Conditions:
 *		Nothing locked.
 */
void
task_watchport_elem_deallocate(
	struct task_watchport_elem *watchport_elem)
{
	os_ref_count_t refs = TASK_MAX_WATCHPORT_COUNT;
	task_t task = watchport_elem->twe_task;
	struct task_watchports *watchports = NULL;
	ipc_port_t port = NULL;

	assert(task != NULL);

	/* Take the space lock to modify the elememt */
	is_write_lock(task->itk_space);

	watchports = task->watchports;
	assert(watchports != NULL);

	port = watchport_elem->twe_port;
	assert(port != NULL);

	task_watchport_elem_clear(watchport_elem);
	refs = task_watchports_release(watchports);

	if (refs == 0) {
		task->watchports = NULL;
	}

	is_write_unlock(task->itk_space);

	ip_release(port);
	if (refs == 0) {
		task_watchports_deallocate(watchports);
	}
}

boolean_t
task_is_exec_copy(task_t task)
{
	return task_is_exec_copy_internal(task);
}

void
task_inspect_deallocate(
	task_inspect_t          task_inspect)
{
	return task_deallocate((task_t)task_inspect);
}

kern_return_t
task_get_exc_guard_behavior(
	task_t task,
	task_exc_guard_behavior_t *behaviorp)
{
	if (task == TASK_NULL) {
		return KERN_INVALID_TASK;
	}
	*behaviorp = task->task_exc_guard;
	return KERN_SUCCESS;
}

int
pid_from_task(task_t task)
{
	int pid = -1;

	if (task->bsd_info) {
		pid = proc_pid(task->bsd_info);
	} else {
		pid = task_pid(task);
	}

	return pid;
}

/*
 * Set or clear per-task TF_CA_CLIENT_WI flag according to specified argument.
 * Returns "false" if flag is already set, and "true" in other cases.
 */
bool
task_set_ca_client_wi(
	task_t task,
	boolean_t set_or_clear)
{
	bool ret = true;
	task_lock(task);
	if (set_or_clear) {
		/* Tasks can have only one CA_CLIENT work interval */
		if (task->t_flags & TF_CA_CLIENT_WI) {
			ret = false;
		} else {
			task->t_flags |= TF_CA_CLIENT_WI;
		}
	} else {
		task->t_flags &= ~TF_CA_CLIENT_WI;
	}
	task_unlock(task);
	return ret;
}

#ifndef TASK_EXC_GUARD_ALL
/* Temporary define until two branches are merged */
#define TASK_EXC_GUARD_ALL (TASK_EXC_GUARD_VM_ALL | 0xf0)
#endif

kern_return_t
task_set_exc_guard_behavior(
	task_t task,
	task_exc_guard_behavior_t behavior)
{
	if (task == TASK_NULL) {
		return KERN_INVALID_TASK;
	}
	if (behavior & ~TASK_EXC_GUARD_ALL) {
		return KERN_INVALID_VALUE;
	}
	task->task_exc_guard = behavior;
	return KERN_SUCCESS;
}

// </copied>
