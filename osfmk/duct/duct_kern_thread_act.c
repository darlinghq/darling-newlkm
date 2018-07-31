/*
Copyright (c) 2014-2017, Wenqi Chen
COpyright (C) 2018 Lubos Dolezel

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
#include "duct_kern_thread_act.h"
#include "duct_kern_task.h"
#include "duct_kern_zalloc.h"
#include "duct_kern_printf.h"
#include "duct_vm_map.h"

#include <mach/mach_types.h>
#include <kern/mach_param.h>
#include <kern/thread.h>
#include <darling/task_registry.h>
#include <darling/debug_print.h>

#include "duct_post_xnu.h"

#define current linux_current
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
#include <linux/sched/mm.h>
#include <linux/sched/cputime.h>
#endif

extern wait_queue_head_t global_wait_queue_head;

static void fill_breakpoint(struct perf_event_attr* attr, __uint64_t dr7, int index);
static void watchpoint_callback(struct perf_event* pevent, struct perf_sample_data* data, struct pt_regs* regs);

kern_return_t duct_thread_terminate (thread_t thread)
{

        struct task_struct        * linux_task      = thread->linux_task;

        printk ( KERN_NOTICE "- duct_thread_terminate, thread: %p, linux_task: %p, pid: %d, state: %ld\n",
                 thread, linux_task, linux_task->pid, linux_task->state);

        kern_return_t   result;

        if (thread == THREAD_NULL) {
                return KERN_INVALID_ARGUMENT;
        }

        // if (    thread->task == kernel_task        &&
        //         thread != current_thread()            )
        //     return (KERN_FAILURE);

        result = KERN_SUCCESS;

        if (linux_task == linux_current) {
                // printk (KERN_NOTICE "- do_exit ()\n");
                do_exit (0);

                // WC - should not return never return
        }

        // other threads
        // CPH: refer to zap_other_threads () on how to kill other threads
        // sigaddset (&t->pending.signal, SIGKILL); signal_wake_up ();
        printk (KERN_NOTICE "- BUG: duct_thread_terminate can't terminate other thread now\n");
        return result;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
typedef u64 cputime_t;

// On new kernels, cputime is in nanoseconds
#define cputime_to_usecs(v) ((v) / 1000)
#endif

kern_return_t
thread_info(
    thread_t                thread,
    thread_flavor_t         flavor,
    thread_info_t           thread_info_out,
    mach_msg_type_number_t  *thread_info_count)
{
	if (thread == NULL)
		return KERN_INVALID_ARGUMENT;
	if (thread->linux_task == NULL)
		return KERN_FAILURE;

	switch (flavor)
	{
		case THREAD_IDENTIFIER_INFO:
		{
			if (*thread_info_count < THREAD_IDENTIFIER_INFO_COUNT)
				return KERN_INVALID_ARGUMENT;
			*thread_info_count = THREAD_IDENTIFIER_INFO_COUNT;

			thread_identifier_info_t id = (thread_identifier_info_t) thread_info_out;

			rcu_read_lock();
			id->thread_id = task_pid_vnr(thread->linux_task);

			// FIXME: should contain pthread_t for this thread, see macosx-nat-infthread.c
			// Also used for PROC_PIDTHREADINFO.
			id->thread_handle = task_pid_vnr(thread->linux_task);
			id->thread_handle |= ((uint64_t)task_tgid_vnr(thread->linux_task)) << 32;
			
			id->dispatch_qaddr = 0;

			rcu_read_unlock();

			return KERN_SUCCESS;
		}
		case THREAD_BASIC_INFO:
		{
			if (*thread_info_count < THREAD_BASIC_INFO_COUNT)
				return KERN_INVALID_ARGUMENT;
			*thread_info_count = THREAD_BASIC_INFO_COUNT;

			thread_basic_info_t out = (thread_basic_info_t) thread_info_out;
			uint64_t utimeus, stimeus;

			memset(out, 0, sizeof(*out));

			utimeus = cputime_to_usecs(thread->linux_task->utime);
			stimeus = cputime_to_usecs(thread->linux_task->stime);

			out->user_time.seconds = utimeus / USEC_PER_SEC;
			out->user_time.microseconds = utimeus % USEC_PER_SEC;
			out->system_time.seconds = stimeus / USEC_PER_SEC;
			out->system_time.microseconds = stimeus % USEC_PER_SEC;

			if (thread->linux_task->state & TASK_UNINTERRUPTIBLE)
				out->run_state = TH_STATE_UNINTERRUPTIBLE;
			else if (task_is_stopped(thread->linux_task))
			{
				out->run_state = TH_STATE_STOPPED;
				out->suspend_count = thread->suspend_count;
			}
			else
				out->run_state = TH_STATE_RUNNING;

			return KERN_SUCCESS;
		}
		default:
			printf("Unhandled thread_info(flavor=%d)\n", flavor);
			return KERN_INVALID_ARGUMENT;
	}
}

kern_return_t
thread_get_state(
    register thread_t       thread,
    int                     flavor,
    thread_state_t          state,          /* pointer to OUT array */
    mach_msg_type_number_t  *state_count)   /*IN/OUT*/
{
	struct task_struct* ltask = thread->linux_task;
	if (!ltask)
		return KERN_FAILURE;

#ifdef __x86_64__
	switch (flavor)
	{
		// The following flavors automatically select 32 or 64-bit state
		// based on process type.
		case x86_THREAD_STATE:
		{
			x86_thread_state_t* s = (x86_thread_state_t*) state;

			if (*state_count < x86_THREAD_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;

			if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
			{
				s->tsh.flavor = flavor = x86_THREAD_STATE64;
				s->tsh.count = x86_THREAD_STATE64_COUNT;
				state = (thread_state_t) &s->uts.ts64;
			}
			else
			{
				s->tsh.flavor = flavor = x86_THREAD_STATE32;
				s->tsh.count = x86_THREAD_STATE32_COUNT;
				state = (thread_state_t) &s->uts.ts32;
			}
			*state_count = x86_THREAD_STATE_COUNT;
			state_count = &s->tsh.count;

			break;
		}
		case x86_FLOAT_STATE:
		{
			x86_float_state_t* s = (x86_float_state_t*) state;

			if (*state_count < x86_FLOAT_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;

			if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
			{
				s->fsh.flavor = flavor = x86_FLOAT_STATE64;
				s->fsh.count = x86_FLOAT_STATE64_COUNT;
				state = (thread_state_t) &s->ufs.fs64;
			}
			else
			{
				s->fsh.flavor = flavor = x86_FLOAT_STATE32;
				s->fsh.count = x86_FLOAT_STATE32_COUNT;
				state = (thread_state_t) &s->ufs.fs32;
			}
			*state_count = x86_FLOAT_STATE_COUNT;
			state_count = &s->fsh.count;
			break;
		}
		case x86_DEBUG_STATE:
		{
			x86_debug_state_t* s = (x86_debug_state_t*) state;

			if (*state_count < x86_DEBUG_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;

			if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
			{
				s->dsh.flavor = flavor = x86_DEBUG_STATE64;
				s->dsh.count = x86_DEBUG_STATE64_COUNT;
				state = (thread_state_t) &s->uds.ds64;
			}
			else
			{
				s->dsh.flavor = flavor = x86_DEBUG_STATE32;
				s->dsh.count = x86_DEBUG_STATE32_COUNT;
				state = (thread_state_t) &s->uds.ds32;
			}
			*state_count = x86_DEBUG_STATE_COUNT;
			state_count = &s->dsh.count;
			break;
		}
	}

	switch (flavor)
	{
		case x86_THREAD_STATE32:
		{
			if (*state_count < x86_THREAD_STATE32_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
				return KERN_INVALID_ARGUMENT;

			x86_thread_state32_t* s = (x86_thread_state32_t*) state;

			*state_count = x86_THREAD_STATE32_COUNT;

			memcpy(s, &thread->thread_state.uts.ts32, sizeof(*s));

			return KERN_SUCCESS;
		}
		case x86_FLOAT_STATE32:
		{
			if (*state_count < x86_FLOAT_STATE32_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
				return KERN_INVALID_ARGUMENT;

			x86_float_state32_t* s = (x86_float_state32_t*) state;

			*state_count = x86_FLOAT_STATE32_COUNT;
			memcpy(s, &thread->float_state.ufs.fs32, sizeof(*s));

			return KERN_SUCCESS;
		}
		case x86_FLOAT_STATE64: // these two are practically identical
		{
			if (*state_count < x86_FLOAT_STATE64_COUNT)
				return KERN_INVALID_ARGUMENT;

			x86_float_state64_t* s = (x86_float_state64_t*) state;

			*state_count = x86_FLOAT_STATE64_COUNT;
			memcpy(s, &thread->float_state.ufs.fs64, sizeof(*s));

			return KERN_SUCCESS;
		}
		case x86_THREAD_STATE64:
		{
			if (*state_count < x86_THREAD_STATE64_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
				return KERN_INVALID_ARGUMENT;

			x86_thread_state64_t* s = (x86_thread_state64_t*) state;
			*state_count = x86_THREAD_STATE64_COUNT;

			memcpy(s, &thread->thread_state.uts.ts64, sizeof(*s));

			return KERN_SUCCESS;
		}
		case x86_DEBUG_STATE32:
		{
			if (*state_count < x86_DEBUG_STATE32_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
				return KERN_INVALID_ARGUMENT;

			x86_debug_state32_t* s = (x86_debug_state32_t*) state;
			*state_count = x86_DEBUG_STATE32_COUNT;

			// Call self and translate from 64-bit
			x86_debug_state64_t s64;
			mach_msg_type_number_t count = x86_DEBUG_STATE64_COUNT;

			kern_return_t kr = thread_get_state(thread, x86_DEBUG_STATE64,
					(thread_state_t) &s64, &count);

			if (kr != KERN_SUCCESS)
				return kr;

			s->dr0 = s64.dr0;
			s->dr1 = s64.dr1;
			s->dr2 = s64.dr2;
			s->dr3 = s64.dr3;
			s->dr4 = s64.dr4;
			s->dr5 = s64.dr5;
			s->dr6 = s64.dr6;
			s->dr7 = s64.dr7;

			return KERN_SUCCESS;
		}
		case x86_DEBUG_STATE64:
		{
			if (*state_count < x86_DEBUG_STATE64_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
				return KERN_INVALID_ARGUMENT;

			x86_debug_state64_t* s = (x86_debug_state64_t*) state;
			*state_count = x86_DEBUG_STATE64_COUNT;

			memset(s, 0, sizeof(*s));

			struct thread_struct *lthread = &ltask->thread;
			int i;

			for (i = 0; i < 4; i++)
			{
				if (lthread->ptrace_bps[i] != NULL)
				{
					const struct perf_event_attr* attr = &lthread->ptrace_bps[i]->attr;

					if (!attr->disabled && attr->bp_type != HW_BREAKPOINT_EMPTY)
						s->dr7 |= 1 << (2*i); // set local enable flag

					switch (attr->bp_type)
					{
						case HW_BREAKPOINT_W:
							s->dr7 |= 1 << (16 + i*4);
							break;
						case HW_BREAKPOINT_RW:
						case HW_BREAKPOINT_R:
							s->dr7 |= 3 << (16 + i*4);
							break;
						case HW_BREAKPOINT_X:
							break;
					}

					switch (attr->bp_len)
					{
						case HW_BREAKPOINT_LEN_1:
							break;
						case HW_BREAKPOINT_LEN_2:
							s->dr7 |= 1 << (18 + i*4);
							break;
						case HW_BREAKPOINT_LEN_4:
							s->dr7 |= 3 << (18 + i*4);
							break;
						case HW_BREAKPOINT_LEN_8:
							s->dr7 |= 2 << (18 + i*4);
							break;
					}

					(&s->dr0)[i] = attr->bp_addr;
				}
			}

			return KERN_SUCCESS;
		}
		default:
			return KERN_INVALID_ARGUMENT;
	}
#else
	return KERN_FAILURE;
#endif
}

/*
 *  Change thread's machine-dependent state.  Called with nothing
 *  locked.  Returns same way.
 */
// static kern_return_t
// thread_set_state_internal(
//     register thread_t       thread,
//     int                     flavor,
//     thread_state_t          state,
//     mach_msg_type_number_t  state_count,
//     boolean_t               from_user)
// {
//     return 0;
// }

/* No prototype, since thread_act_server.h has the _from_user version if KERNEL_SERVER */
kern_return_t
thread_set_state(
    register thread_t       thread,
    int                     flavor,
    thread_state_t          state,
    mach_msg_type_number_t  state_count);

kern_return_t
thread_set_state(
    register thread_t       thread,
    int                     flavor,
    thread_state_t          state,
    mach_msg_type_number_t  state_count)
{
	struct task_struct* ltask = thread->linux_task;
	if (!ltask)
		return KERN_FAILURE;

#ifdef __x86_64__
	switch (flavor)
	{
		case x86_THREAD_STATE:
		{
			x86_thread_state_t* s = (x86_thread_state_t*) state;

			if (state_count < x86_THREAD_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;

			if (s->tsh.flavor == x86_THREAD_STATE32)
			{
				if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
					return KERN_INVALID_ARGUMENT;

				state_count = s->tsh.count;
				state = (thread_state_t) &s->uts.ts32;
			}
			else if (s->tsh.flavor == x86_THREAD_STATE64)
			{
				if (test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
					return KERN_INVALID_ARGUMENT;

				state_count = s->tsh.count;
				state = (thread_state_t) &s->uts.ts64;
			}
			else
				return KERN_INVALID_ARGUMENT;

			flavor = s->tsh.flavor;
			break;
		}
		case x86_FLOAT_STATE:
		{
			x86_float_state_t* s = (x86_float_state_t*) state;

			if (state_count < x86_FLOAT_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;

			if (s->fsh.flavor == x86_FLOAT_STATE32)
			{
				if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
					return KERN_INVALID_ARGUMENT;

				state_count = s->fsh.count;
				state = (thread_state_t) &s->ufs.fs32;
			}
			else if (s->fsh.flavor == x86_FLOAT_STATE64)
			{
				if (test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
					return KERN_INVALID_ARGUMENT;

				state_count = s->fsh.count;
				state = (thread_state_t) &s->ufs.fs64;
			}
			else
				return KERN_INVALID_ARGUMENT;

			flavor = s->fsh.flavor;
			break;
		}
		case x86_DEBUG_STATE:
		{
			x86_debug_state_t* s = (x86_debug_state_t*) state;

			if (state_count < x86_DEBUG_STATE_COUNT)
				return KERN_INVALID_ARGUMENT;

			if (s->dsh.flavor == x86_DEBUG_STATE32)
			{
				if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
					return KERN_INVALID_ARGUMENT;

				state_count = s->dsh.count;
				state = (thread_state_t) &s->uds.ds32;
			}
			else if (s->dsh.flavor == x86_DEBUG_STATE64)
			{
				if (test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
					return KERN_INVALID_ARGUMENT;

				state_count = s->dsh.count;
				state = (thread_state_t) &s->uds.ds64;
			}
			else
				return KERN_INVALID_ARGUMENT;

			flavor = s->dsh.flavor;
			break;
		}
	}

	switch (flavor)
	{
		case x86_THREAD_STATE32:
		{
			if (state_count < x86_THREAD_STATE32_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
				return KERN_INVALID_ARGUMENT;

			const x86_thread_state32_t* s = (x86_thread_state32_t*) state;

			memcpy(&thread->thread_state.uts.ts32, s, sizeof(*s));
			return KERN_SUCCESS;
		}
		case x86_THREAD_STATE64:
		{
			if (state_count < x86_THREAD_STATE64_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
				return KERN_INVALID_ARGUMENT;

			const x86_thread_state64_t* s = (x86_thread_state64_t*) state;

			memcpy(&thread->thread_state.uts.ts64, s, sizeof(*s));
			return KERN_SUCCESS;
		}
		case x86_FLOAT_STATE32:
		{
			if (state_count < x86_FLOAT_STATE32_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
				return KERN_INVALID_ARGUMENT;

			const x86_float_state32_t* s = (x86_float_state32_t*) state;

			memcpy(&thread->float_state.ufs.fs32, s, sizeof(*s));
			return KERN_SUCCESS;
		}

		case x86_FLOAT_STATE64:
		{
			if (state_count < x86_FLOAT_STATE64_COUNT)
				return KERN_INVALID_ARGUMENT;
			if (darling_is_task_64bit())
				return KERN_INVALID_ARGUMENT;

			const x86_float_state64_t* s = (x86_float_state64_t*) state;

			memcpy(&thread->float_state.ufs.fs64, s, sizeof(*s));
			return KERN_SUCCESS;
		}
		case x86_DEBUG_STATE32:
		{
			if (!test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
				return KERN_INVALID_ARGUMENT;
			const x86_debug_state32_t* s = (x86_debug_state32_t*) state;
			x86_debug_state64_t s64;

			s64.dr0 = s->dr0;
			s64.dr1 = s->dr1;
			s64.dr2 = s->dr2;
			s64.dr3 = s->dr3;
			s64.dr4 = s->dr4;
			s64.dr5 = s->dr5;
			s64.dr6 = s->dr6;
			s64.dr7 = s->dr7;

			return thread_set_state(thread, x86_DEBUG_STATE64, (thread_state_t) &s,
					x86_DEBUG_STATE64_COUNT);
		}
		case x86_DEBUG_STATE64:
		{
			if (test_ti_thread_flag(task_thread_info(ltask), TIF_IA32))
				return KERN_INVALID_ARGUMENT;

			const x86_debug_state64_t* s = (x86_debug_state64_t*) state;

			struct thread_struct *lthread = &ltask->thread;
			int i;

			for (i = 0; i < 4; i++)
			{
				__uint64_t addr = (&s->dr0)[i];

				if (lthread->ptrace_bps[i] != NULL)
				{
					struct perf_event* pevent = lthread->ptrace_bps[i];
					struct perf_event_attr attr = pevent->attr;

					if (s->dr7 & (1 << (2*i)))
					{
						// Possibly modify an existing watchpoint
						fill_breakpoint(&attr, s->dr7, i);
						attr.bp_addr = addr;

						if (memcmp(&attr, &pevent->attr, sizeof(attr)) == 0)
							continue; // no change
					}
					else
					{
						// Disable the watchpoint
						if (attr.disabled)
							continue; // already disabled

						attr.disabled = true;
					}

					modify_user_hw_breakpoint(pevent, &attr);
				}
				else if (s->dr7 & (1 << (2*i)))
				{
					// Create a new watchpoint
					struct perf_event_attr attr;
					struct perf_event* pevent;

					fill_breakpoint(&attr, s->dr7, i);
					attr.bp_addr = addr;

					pevent = register_user_hw_breakpoint(&attr, watchpoint_callback, NULL, ltask);
					lthread->ptrace_bps[i] = pevent;
				}
			}

			return KERN_SUCCESS;
		}
		default:
			return KERN_INVALID_ARGUMENT;
	}
#else
	return KERN_FAILURE;
#endif
}

static void fill_breakpoint(struct perf_event_attr* attr, __uint64_t dr7, int index)
{
	unsigned int flags = (dr7 >> (16 + 4*index)) & 0xf;
	hw_breakpoint_init(attr);

	attr->exclude_kernel = true;

	switch ((flags >> 2) & 3)
	{
		case 0:
			attr->bp_len = HW_BREAKPOINT_LEN_1;
			break;
		case 1:
			attr->bp_len = HW_BREAKPOINT_LEN_2;
			break;
		case 3:
			attr->bp_len = HW_BREAKPOINT_LEN_4;
			break;
		case 2:
			attr->bp_len = HW_BREAKPOINT_LEN_8;
			break;

	}

	switch (flags & 3)
	{
		case 0:
			attr->bp_type = HW_BREAKPOINT_X;
			attr->bp_len = (sizeof(long) == 8) ? HW_BREAKPOINT_LEN_8 : HW_BREAKPOINT_LEN_4;
			break;
		case 1:
			attr->bp_type = HW_BREAKPOINT_W;
			break;
		case 2:
		case 3:
			attr->bp_type = HW_BREAKPOINT_RW;
			break;
	}
}

static void watchpoint_callback(struct perf_event* pevent, struct perf_sample_data* data, struct pt_regs* regs)
{
	struct thread_struct* lthread = &linux_current->thread;
	int i;

	for (i = 0; i < HBP_NUM; i++)
	{
		if (lthread->ptrace_bps[i] == pevent)
		{
			thread_t thread = darling_thread_get_current();
			thread->triggered_watchpoint_address = lthread->ptrace_bps[i]->attr.bp_addr;
			
			switch (lthread->ptrace_bps[i]->attr.bp_type)
			{
				case HW_BREAKPOINT_W:
					thread->triggered_watchpoint_operation = 1;
					break;
				case HW_BREAKPOINT_RW:
				case HW_BREAKPOINT_R:
					thread->triggered_watchpoint_operation = 3;
					break;
				case HW_BREAKPOINT_X:
					thread->triggered_watchpoint_operation = 0;
					break;
			}
			lthread->debugreg6 |= 1 << i;

			break;
		}
	}
}

