/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2015-2018 Lubos Dolezel
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

#include "pthread_kill.h"
#include <asm/siginfo.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/signal_types.h>
#include <asm/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>
#include <osfmk/kern/ipc_tt.h>
#include <duct/duct_post_xnu.h>

#define current linux_current

int pthread_kill_trap(task_t task,
		struct pthread_kill_args* in_args)
{
	struct pthread_kill_args args;
	int ret;
	pid_t tid;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,20,0)
	struct kernel_siginfo info;
#else
	struct siginfo info;
#endif
	struct task_struct *t;
	thread_t thread;
	
	if (copy_from_user(&args, in_args, sizeof(args)))
		return -LINUX_EFAULT;

	thread = port_name_to_thread(args.thread_port);

	if (thread == THREAD_NULL || !thread->linux_task)
		return -LINUX_ESRCH;
		
	rcu_read_lock();
	t = pid_task(find_pid_ns(thread->linux_task->pid, &init_pid_ns), PIDTYPE_PID);
	
	if (t == NULL)
	{
		rcu_read_unlock();
		ret = -LINUX_ESRCH;
		goto err;
	}

	if (args.sig > 0)
	{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,16,0)
		clear_siginfo(&info);
#else
		info.si_errno = 0;
#endif
		info.si_signo = args.sig;
		info.si_code = SI_TKILL;
		info.linux_si_pid = task_tgid_vnr(current);
		info.linux_si_uid = from_kuid_munged(current_user_ns(), current_uid());

		ret = send_sig_info(args.sig, &info, t);
	}
	else if (args.sig < 0)
		ret = -LINUX_EINVAL;
	else
		ret = 0;
	
	rcu_read_unlock();

err:
	thread_deallocate(thread);

	return ret;
}

