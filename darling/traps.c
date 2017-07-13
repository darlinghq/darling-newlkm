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
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/file.h>
#include <linux/dcache.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/eventfd.h>
#include <linux/fdtable.h>
#include <linux/syscalls.h>
#include "traps.h"
#include <duct/duct_pre_xnu.h>
#include <duct/duct_kern_task.h>
#include <duct/duct_kern_thread.h>
#include <mach/mach_types.h>
#include <mach/mach_traps.h>
#include <mach/task_special_ports.h>
#include <ipc/ipc_port.h>
#include <vm/vm_protos.h>
#include <kern/task.h>
#include <kern/ipc_tt.h>
#include <kern/queue.h>
#include <duct/duct_ipc_pset.h>
#include <duct/duct_post_xnu.h>
#include "task_registry.h"
#include "psynch/pthread_kill.h"
#include "psynch/psynch_cv.h"
#include "psynch/psynch_mutex.h"
#include "debug_print.h"
#include "evprocfd.h"
#include "evpsetfd.h"

typedef long (*trap_handler)(task_t, ...);

struct trap_entry {
	trap_handler handler;
	const char* name;
};

static struct file_operations mach_chardev_ops = {
	.open           = mach_dev_open,
	.release        = mach_dev_release,
	.unlocked_ioctl = mach_dev_ioctl,
	.compat_ioctl   = mach_dev_ioctl,
	.owner          = THIS_MODULE
};

#define TRAP(num, impl) [num - DARLING_MACH_API_BASE] = { (trap_handler) impl, #impl }

static const struct trap_entry mach_traps[60] = {
	// GENERIC
	TRAP(NR_get_api_version, mach_get_api_version),

	// INTERNAL
	TRAP(NR_thread_death_announce, thread_death_announce_entry),
	TRAP(NR_fork_wait_for_child, fork_wait_for_child_entry),
	TRAP(NR_set_dyld_info, set_dyld_info_entry),
	TRAP(NR_stop_after_exec, stop_after_exec_entry),
	TRAP(NR_kernel_printk, kernel_printk_entry),

	// KQUEUE
	TRAP(NR_evproc_create, evproc_create_entry),
	TRAP(NR_evfilt_machport_open, evfilt_machport_open_entry),

	// PSYNCH
	TRAP(NR_pthread_kill_trap, pthread_kill_trap),
	TRAP(NR_psynch_mutexwait_trap, psynch_mutexwait_trap),
	TRAP(NR_psynch_mutexdrop_trap, psynch_mutexdrop_trap),
	TRAP(NR_psynch_cvwait_trap, psynch_cvwait_trap),
	TRAP(NR_psynch_cvsignal_trap, psynch_cvsignal_trap),
	TRAP(NR_psynch_cvbroad_trap, psynch_cvbroad_trap),

	// MACH IPC
	TRAP(NR_mach_reply_port, mach_reply_port_entry),
	TRAP(NR__kernelrpc_mach_port_mod_refs, _kernelrpc_mach_port_mod_refs_entry),
	TRAP(NR_task_self_trap, task_self_trap_entry),
	TRAP(NR_host_self_trap, host_self_trap_entry),
	TRAP(NR_thread_self_trap, thread_self_trap_entry),
	TRAP(NR__kernelrpc_mach_port_allocate, _kernelrpc_mach_port_allocate_entry),
	TRAP(NR_mach_msg_overwrite_trap, mach_msg_overwrite_entry),
	TRAP(NR__kernelrpc_mach_port_deallocate, _kernelrpc_mach_port_deallocate_entry),
	TRAP(NR__kernelrpc_mach_port_destroy, _kernelrpc_mach_port_destroy),
	TRAP(NR_semaphore_signal_trap, semaphore_signal_entry),
	TRAP(NR_semaphore_signal_all_trap, semaphore_signal_all_entry),
	TRAP(NR_semaphore_wait_trap, semaphore_wait_entry),
	TRAP(NR_semaphore_wait_signal_trap, semaphore_wait_signal_entry),
	TRAP(NR_semaphore_timedwait_signal_trap, semaphore_timedwait_signal_entry),
	TRAP(NR_semaphore_timedwait_trap, semaphore_timedwait_entry),
	TRAP(NR__kernelrpc_mach_port_move_member_trap, _kernelrpc_mach_port_move_member_entry),
	TRAP(NR__kernelrpc_mach_port_extract_member_trap, _kernelrpc_mach_port_extract_member_entry),
	TRAP(NR__kernelrpc_mach_port_insert_member_trap, _kernelrpc_mach_port_insert_member_entry),
	TRAP(NR__kernelrpc_mach_port_insert_right_trap, _kernelrpc_mach_port_insert_right_entry),

	// MKTIMER
	TRAP(NR_mk_timer_create_trap, mk_timer_create_entry),
	TRAP(NR_mk_timer_arm_trap, mk_timer_arm_entry),
	TRAP(NR_mk_timer_cancel_trap, mk_timer_cancel_entry),
	TRAP(NR_mk_timer_destroy_trap, mk_timer_destroy_entry),
	
	// TASKS
	TRAP(NR_task_for_pid_trap, task_for_pid_entry),
};
#undef TRAP

static struct miscdevice mach_dev = {
	MISC_DYNAMIC_MINOR,
	"mach",
	&mach_chardev_ops,
};

extern void darling_xnu_init(void);
extern void darling_xnu_deinit(void);
static int mach_init(void)
{
	int err = 0;

	darling_task_init();
	darling_xnu_init();

	err = misc_register(&mach_dev);
	if (err < 0)
	    goto fail;

	printk(KERN_INFO "Darling Mach: kernel emulation loaded, API version " DARLING_MACH_API_VERSION_STR "\n");
	return 0;

fail:
	printk(KERN_WARNING "Error loading Darling Mach: %d\n", err);
	return err;
}
static void mach_exit(void)
{
	darling_xnu_deinit();
	misc_deregister(&mach_dev);
	printk(KERN_INFO "Darling Mach: kernel emulation unloaded\n");
}

extern kern_return_t task_get_special_port(task_t, int which, ipc_port_t*);
extern kern_return_t task_set_special_port(task_t, int which, ipc_port_t);

int mach_dev_open(struct inode* ino, struct file* file)
{
	kern_return_t ret;
	task_t new_task, parent_task, old_task;
	thread_t new_thread;
	int fd_old = -1;
	int ppid_fork = -1;
	
	debug_msg("Setting up new XNU task for pid %d\n", linux_current->tgid);

	// Create a new task_t
	ret = duct_task_create_internal(NULL, false, true, &new_task, linux_current);
	if (ret != KERN_SUCCESS)
		return -LINUX_EINVAL;

	file->private_data = new_task;
	new_task->audit_token.val[5] = task_pid_vnr(linux_current);
	
	// Are we being opened after fork or execve?
	old_task = darling_task_get_current();
	if (old_task != NULL)
	{
		// execve case:
		// 1) Take over the previously used bootstrap port
		// 2) Find the old fd used before execve and close it
		
		ipc_port_t bootstrap_port;
		int fd;
		struct files_struct* files;
		
		task_get_bootstrap_port(old_task, &bootstrap_port);
		
		debug_msg("Setting bootstrap port from old task: %p\n", bootstrap_port);
		if (bootstrap_port != NULL)
			task_set_bootstrap_port(new_task, bootstrap_port);
		
		// find the old fd and close it (later)
		files = linux_current->files;
		
		spin_lock(&files->file_lock);
		
		for (fd = 0; fd < files_fdtable(files)->max_fds; fd++)
		{
			struct file* f = fcheck_files(files, fd);
			
			if (!f)
				continue;
			
			if (f != file && f->f_op == &mach_chardev_ops)
			{
				fd_old = fd;
				break;
			}
		}
		
		spin_unlock(&files->file_lock);
	}
	else
	{
		unsigned int ppid;

		// fork case:
		// 1) Take over parent's bootstrap port
		// 2) Signal the parent that it can continue running now
		
		ppid = linux_current->parent->tgid;

		parent_task = darling_task_get(ppid);
		debug_msg("- Fork case detected, ppid=%d, parent_task=%p\n", ppid, parent_task);
		if (parent_task != NULL)
		{
			ipc_port_t bootstrap_port;
			task_get_bootstrap_port(parent_task, &bootstrap_port);
		
			debug_msg("Setting bootstrap port from parent: %p\n", bootstrap_port);
			if (bootstrap_port != NULL)
				task_set_bootstrap_port(new_task, bootstrap_port);
		
			task_deallocate(parent_task);

			ppid_fork = ppid;
		}
		else
		{
			// PID 1 case (or manually run mldr)
			debug_msg("This task has no Darling parent\n");
		}
	}
	
	debug_msg("mach_dev_open().0 refc %d\n", new_task->ref_count);

	// Create a new thread_t
	ret = duct_thread_create(new_task, &new_thread);
	if (ret != KERN_SUCCESS)
	{
		duct_task_destroy(new_task);
		return -LINUX_EINVAL;
	}

	debug_msg("thread %p refc %d at #1\n", new_thread, new_thread->ref_count);
	darling_task_register(new_task);
	darling_thread_register(new_thread);
	
	debug_msg("thread refc %d at #2\n", new_thread->ref_count);
	task_deallocate(new_task);
	thread_deallocate(new_thread);
	
	// execve case only:
	// We do this after running darling_task_register & darling_thread_register
	// and avoid calling deregister beforehand, so that death notifications don't fire
	// in case of execve.
	if (fd_old != -1)
	{
		debug_msg("Closing old fd before execve: %d\n", fd_old);
		sys_close(fd_old);
	}
	
	// fork case only
	if (ppid_fork != -1)
	{
		darling_task_fork_child_done();
		darling_task_post_notification(ppid_fork, NOTE_FORK, task_pid_vnr(linux_current));
	}
	
	return 0;
}

int mach_dev_release(struct inode* ino, struct file* file)
{
	thread_t thread, cur_thread;
	task_t my_task = (task_t) file->private_data;
	
	// close(/dev/mach) may happen on any thread, even on a thread that
	// has never seen any ioctl() calls into LKM.
	if (!darling_thread_get_current())
	{
		debug_msg("No current thread!\n");
		// return -LINUX_EINVAL;
		thread_self_trap_entry(my_task);
	}
	
	darling_task_deregister(my_task);
	// darling_thread_deregister(NULL);
	
	cur_thread = darling_thread_get_current();
	
	debug_msg("Destroying XNU task for pid %d, refc %d\n", linux_current->tgid, my_task->ref_count);
	
	task_lock(my_task);
	//queue_iterate(&my_task->threads, thread, thread_t, task_threads)
	while (!queue_empty(&my_task->threads))
	{
		thread = (thread_t) queue_first(&my_task->threads);
		
		// debug_msg("mach_dev_release() - thread refc %d\n", thread->ref_count);
		
		// Because IPC space termination needs a current thread
		if (thread != cur_thread)
		{
			task_unlock(my_task);
			duct_thread_destroy(thread);
			darling_thread_deregister(thread);
			task_lock(my_task);
		}
		else
			queue_remove(&my_task->threads, thread, thread_t, task_threads);
	}
	task_unlock(my_task);
	duct_task_destroy(my_task);

	//debug_msg("Destroying XNU task for pid %d, refc %d\n", linux_current->pid, my_task->ref_count);
	//duct_task_destroy(my_task);
	
	//duct_thread_destroy(cur_thread);
	darling_thread_deregister(cur_thread);

	return 0;
}

long mach_dev_ioctl(struct file* file, unsigned int ioctl_num, unsigned long ioctl_paramv)
{
	const unsigned int num_traps = sizeof(mach_traps) / sizeof(mach_traps[0]);
	const struct trap_entry* entry;
	long rv;

	task_t task = (task_t) file->private_data;

	ioctl_num -= DARLING_MACH_API_BASE;

	if (ioctl_num >= num_traps)
		return -LINUX_ENOSYS;

	entry = &mach_traps[ioctl_num];
	if (!entry->handler)
		return -LINUX_ENOSYS;

	debug_msg("function %s (0x%x) called...\n", entry->name, ioctl_num);

	if (!darling_thread_get_current())
	{
		debug_msg("New thread! Registering.\n");
		thread_self_trap_entry(task);
	}

	rv = entry->handler(task, ioctl_paramv);

	debug_msg("...%s returned %ld\n", entry->name, rv);
	return rv;
}

int mach_get_api_version(task_t task)
{
	return DARLING_MACH_API_VERSION;
}

int mach_reply_port_entry(task_t task)
{
	return mach_reply_port(NULL);
}

#define copyargs(args, in_args) typeof(*in_args) args; \
	if (copy_from_user(&args, in_args, sizeof(args))) \
		return -LINUX_EFAULT;

int _kernelrpc_mach_port_mod_refs_entry(task_t task, struct mach_port_mod_refs_args* in_args)
{
	struct _kernelrpc_mach_port_mod_refs_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;
	out.right = args.right_type;
	out.delta = args.delta;

	return _kernelrpc_mach_port_mod_refs_trap(&out);
}

int task_self_trap_entry(task_t task)
{
	return task_self_trap(NULL);
}

int host_self_trap_entry(task_t task)
{
	return host_self_trap(NULL);
}

int thread_self_trap_entry(task_t task)
{
	if (darling_thread_get_current() == NULL)
	{
		thread_t thread;
		duct_thread_create(task, &thread);
		thread_deallocate(thread);

		darling_thread_register(thread);
	}
	return thread_self_trap(NULL);
}

int _kernelrpc_mach_port_allocate_entry(task_t task, struct mach_port_allocate_args* in_args)
{
	struct _kernelrpc_mach_port_allocate_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.right = args.right_type;
	out.name = (user_addr_t) args.out_right_name;

	return _kernelrpc_mach_port_allocate_trap(&out);
}

int mach_msg_overwrite_entry(task_t task, struct mach_msg_overwrite_args* in_args)
{
	struct mach_msg_overwrite_trap_args out;
	copyargs(args, in_args);

	out.msg = (user_addr_t) args.msg;
	out.option = args.option;
	out.send_size = args.send_size;
	out.rcv_size = args.recv_size;
	out.rcv_name = args.rcv_name;
	out.timeout = args.timeout;
	out.notify = args.notify;
	out.rcv_msg = (user_addr_t) args.rcv_msg;

	return mach_msg_overwrite_trap(&out);
}

int _kernelrpc_mach_port_deallocate_entry(task_t task, struct mach_port_deallocate_args* in_args)
{
	struct _kernelrpc_mach_port_deallocate_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;

	return _kernelrpc_mach_port_deallocate_trap(&out);
}

int _kernelrpc_mach_port_destroy(task_t task, struct mach_port_destroy_args* in_args)
{
	struct _kernelrpc_mach_port_destroy_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;

	return _kernelrpc_mach_port_destroy_trap(&out);
}

int _kernelrpc_mach_port_insert_right_entry(task_t task, struct mach_port_insert_right_args* in_args)
{
	struct _kernelrpc_mach_port_insert_right_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_name;
	out.poly = args.right_name;
	out.polyPoly = args.right_type;

	return _kernelrpc_mach_port_insert_right_trap(&out);
}

int _kernelrpc_mach_port_move_member_entry(task_t task, struct mach_port_move_member_args* in_args)
{
	struct _kernelrpc_mach_port_move_member_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.member = args.port_right_name;
	out.after = args.pset_right_name;

	return _kernelrpc_mach_port_move_member_trap(&out);
}

int _kernelrpc_mach_port_extract_member_entry(task_t task, struct mach_port_extract_member_args* in_args)
{
	struct _kernelrpc_mach_port_extract_member_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;
	out.pset = args.pset_right_name;

	return _kernelrpc_mach_port_extract_member_trap(&out);
}

int _kernelrpc_mach_port_insert_member_entry(task_t task, struct mach_port_insert_member_args* in_args)
{
	struct _kernelrpc_mach_port_insert_member_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;
	out.pset = args.pset_right_name;

	return _kernelrpc_mach_port_insert_member_trap(&out);
}

int semaphore_signal_entry(task_t task, struct semaphore_signal_args* in_args)
{
	struct semaphore_signal_trap_args out;
	copyargs(args, in_args);

	out.signal_name = args.signal;

	return semaphore_signal_trap(&out);
}

int semaphore_signal_all_entry(task_t task, struct semaphore_signal_all_args* in_args)
{
	struct semaphore_signal_all_trap_args out;
	copyargs(args, in_args);

	out.signal_name = args.signal;

	return semaphore_signal_all_trap(&out);
}

int semaphore_wait_entry(task_t task, struct semaphore_wait_args* in_args)
{
	struct semaphore_wait_trap_args out;
	copyargs(args, in_args);

	out.wait_name = args.signal;

	return semaphore_wait_trap(&out);
}

int semaphore_wait_signal_entry(task_t task, struct semaphore_wait_signal_args* in_args)
{
	struct semaphore_wait_signal_trap_args out;
	copyargs(args, in_args);

	out.wait_name = args.wait;
	out.signal_name = args.signal;

	return semaphore_wait_signal_trap(&out);
}

int semaphore_timedwait_signal_entry(task_t task, struct semaphore_timedwait_signal_args* in_args)
{
	struct semaphore_timedwait_signal_trap_args out;
	copyargs(args, in_args);

	out.wait_name = args.wait;
	out.signal_name = args.signal;
	out.sec = args.sec;
	out.nsec = args.nsec;

	return semaphore_timedwait_signal_trap(&out);
}

int semaphore_timedwait_entry(task_t task, struct semaphore_timedwait_args* in_args)
{
	struct semaphore_timedwait_trap_args out;
	copyargs(args, in_args);

	out.wait_name = args.wait;
	out.sec = args.sec;
	out.nsec = args.nsec;

	return semaphore_timedwait_trap(&out);
}

int mk_timer_create_entry(task_t task)
{
	return mk_timer_create_trap(NULL);
}

int mk_timer_arm_entry(task_t task, struct mk_timer_arm_args* in_args)
{
	struct mk_timer_arm_trap_args out;
	copyargs(args, in_args);

	out.name = args.timer_port;
	out.expire_time = args.expire_time;

	return mk_timer_arm_trap(&out);
}

int mk_timer_cancel_entry(task_t task, struct mk_timer_cancel_args* in_args)
{
	struct mk_timer_cancel_trap_args out;
	copyargs(args, in_args);

	out.name = args.timer_port;
	out.result_time = (user_addr_t) args.result_time;

	return mk_timer_cancel_trap(&out);
}

int mk_timer_destroy_entry(task_t task, struct mk_timer_destroy_args* in_args)
{
	struct mk_timer_destroy_trap_args out;
	copyargs(args, in_args);

	out.name = args.timer_port;

	return mk_timer_destroy_trap(&out);
}

int thread_death_announce_entry(task_t task)
{
	thread_t thread = darling_thread_get_current();
	if (thread != NULL)
	{
		duct_thread_destroy(thread);
		darling_thread_deregister(thread);
		return 0;
	}

	return -LINUX_ESRCH;
}

int fork_wait_for_child_entry(task_t task)
{
	// Wait until the fork() child re-opens /dev/mach
	darling_task_fork_wait_for_child();
	return 0;
}

int evproc_create_entry(task_t task, struct evproc_create* in_args)
{
	struct pid* pidobj;
	unsigned int pid = 0;

	copyargs(args, in_args);

	// Convert virtual PID to global PID
	rcu_read_lock();

	pidobj = find_vpid(args.pid);
	if (pidobj != NULL)
		pid = pid_nr(pidobj);

	rcu_read_unlock();

	if (pid == 0)
		return -LINUX_ESRCH;

	return evprocfd_create(pid, args.flags, NULL);
}

int task_for_pid_entry(task_t task, struct task_for_pid* in_args)
{
	struct pid* pidobj;
	unsigned int pid = 0;
	task_t task_out;
	int port_name;
	ipc_port_t sright;

	copyargs(args, in_args);

	// Convert virtual PID to global PID
	rcu_read_lock();

	pidobj = find_vpid(args.pid);
	if (pidobj != NULL)
		pid = pid_nr(pidobj);

	rcu_read_unlock();

	printk(KERN_DEBUG "- task_for_pid(): pid %d -> %d\n", args.pid, pid);
	if (pid == 0)
		return KERN_FAILURE;
	
	// Lookup task in task registry
	task_out = darling_task_get(pid);
	if (task_out == NULL)
		return KERN_FAILURE;
	
	// Turn it into a send right
	task_reference(task_out);
	sright = convert_task_to_port(task_out);
	port_name = ipc_port_copyout_send(sright, task->itk_space);
	
	if (copy_to_user(args.task_port, &port_name, sizeof(port_name)))
		return KERN_INVALID_ADDRESS;

	return KERN_SUCCESS;
}

int pid_for_task_entry(task_t task, struct pid_for_task* in_args)
{
	task_t that_task;
	int pid;
	
	copyargs(args, in_args);
	
	that_task = port_name_to_task(args.task_port);
	if (that_task == NULL)
		return -LINUX_ESRCH;
	
	pid = that_task->audit_token.val[5]; // contains vpid
	task_deallocate(that_task);

	if (copy_to_user(args.pid, &pid, sizeof(pid)))
		return KERN_INVALID_ADDRESS;
	
	return KERN_SUCCESS;
}

// This call exists only because we do Mach-O loading in user space.
// On XNU, this information is readily available in the kernel.
int set_dyld_info_entry(task_t task, struct set_dyld_info_args* in_args)
{
	copyargs(args, in_args);

	darling_task_set_dyld_info(args.all_images_address, args.all_images_length);
	return 0;
}

// Needed for POSIX_SPAWN_START_SUSPENDED
int stop_after_exec_entry(task_t task)
{
	darling_task_mark_start_suspended();
	return 0;
}

int kernel_printk_entry(task_t task, struct kernel_printk_args* in_args)
{
	copyargs(args, in_args);

	args.buf[sizeof(args.buf)-1] = '\0';
	debug_msg("%s\n", args.buf);

	return 0;
}

int evfilt_machport_open_entry(task_t task, struct evfilt_machport_open_args* in_args)
{
	copyargs(args, in_args);

	return evpsetfd_create(args.port_name, &args.opts);
}

module_init(mach_init);
module_exit(mach_exit);

