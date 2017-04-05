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
#include <linux/syscalls.h>
#include "traps.h"
#include <duct/duct_pre_xnu.h>
#include <duct/duct_kern_task.h>
#include <duct/duct_kern_thread.h>
#include <mach/mach_types.h>
#include <mach/mach_traps.h>
#include <duct/duct_post_xnu.h>
#include "task_registry.h"
#include "psynch/pthread_kill.h"
#include "psynch/psynch_cv.h"
#include "psynch/psynch_mutex.h"
#include "debug_print.h"

typedef long (*trap_handler)(task_t, ...);

static struct file_operations mach_chardev_ops = {
	.open           = mach_dev_open,
	.release        = mach_dev_release,
	.unlocked_ioctl = mach_dev_ioctl,
	.compat_ioctl   = mach_dev_ioctl,
	.owner          = THIS_MODULE
};

#define TRAP(num, impl) [num - DARLING_MACH_API_BASE] = (trap_handler) impl

static const trap_handler mach_traps[40] = {
	// GENERIC
	TRAP(NR_get_api_version, mach_get_api_version),

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

	// MKTIMER
	TRAP(NR_mk_timer_create_trap, mk_timer_create_entry),
	TRAP(NR_mk_timer_arm_trap, mk_timer_arm_entry),
	TRAP(NR_mk_timer_cancel_trap, mk_timer_cancel_entry),
	TRAP(NR_mk_timer_destroy_trap, mk_timer_destroy_entry),
};
#undef TRAP

static struct miscdevice mach_dev = {
	MISC_DYNAMIC_MINOR,
	"mach",
	&mach_chardev_ops,
};

extern void darling_xnu_init(void);
static int mach_init(void)
{
	int err = 0;

	darling_xnu_init();
	darling_task_init();

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
	misc_deregister(&mach_dev);
	printk(KERN_INFO "Darling Mach: kernel emulation unloaded\n");
}

int mach_dev_open(struct inode* ino, struct file* file)
{
	kern_return_t ret;
	task_t new_task;
	thread_t new_thread;
	
	debug_msg("Setting up new XNU task for pid %d\n", linux_current->pid);

	// Create a new task_t
	ret = duct_task_create_internal(NULL, false, true, &new_task);
	if (ret != KERN_SUCCESS)
		return -LINUX_EINVAL;

	file->private_data = new_task;

	// Create a new thread_t
	ret = duct_thread_create(new_task, &new_thread);
	if (ret != KERN_SUCCESS)
	{
		duct_task_destroy(new_task);
		return -LINUX_EINVAL;
	}

	darling_task_register(new_task);
	darling_thread_register(new_thread);
	return 0;
}

int mach_dev_release(struct inode* ino, struct file* file)
{
	task_t my_task = (task_t) file->private_data;
	
	darling_task_deregister(my_task);

	darling_thread_deregister(NULL);

	debug_msg("Destroying XNU task for pid %d\n", linux_current->pid);
	duct_task_destroy(my_task);

	return 0;
}

long mach_dev_ioctl(struct file* file, unsigned int ioctl_num, unsigned long ioctl_paramv)
{
	const unsigned int num_traps = sizeof(mach_traps) / sizeof(mach_traps[0]);

	task_t task = (task_t) file->private_data;

	kprintf("function 0x%x called...\n", ioctl_num);

	ioctl_num -= DARLING_MACH_API_BASE;

	if (ioctl_num >= num_traps)
		return -LINUX_ENOSYS;

	if (!mach_traps[ioctl_num])
		return -LINUX_ENOSYS;

	return mach_traps[ioctl_num](task, ioctl_paramv);
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
	return thread_self_trap(NULL);
}

int _kernelrpc_mach_port_allocate_entry(task_t task, struct mach_port_allocate_args* in_args)
{
	struct _kernelrpc_mach_port_allocate_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.right = args.right_type;
	out.name = args.out_right_name;

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

module_init(mach_init);
module_exit(mach_exit);
