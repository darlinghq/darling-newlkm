/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2015-2020 Lubos Dolezel
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
#include "traps.h"
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
#include <linux/fs_struct.h>
#include <linux/moduleparam.h>
#include <linux/exportfs.h>
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
#include <kern/ipc_misc.h>
#include <vm/vm_map.h>
#include <sys/proc_internal.h>
#include <sys/user.h>
#include <sys/pthread_shims.h>
#include <duct/duct_ipc_pset.h>
#include <duct/duct_post_xnu.h>
#include <uapi/linux/ptrace.h>
#include <asm/signal.h>
#ifdef __x86_64__
#include <mach/i386/thread_status.h>
#endif
#include "task_registry.h"
#include "pthread_kill.h"
#include "debug_print.h"
#include "psynch_support.h"
#include "binfmt.h"
#include "commpage.h"
#include "foreign_mm.h"
#include "continuation.h"
#include "pthread_kext.h"
#include "procs.h"
#include "kqueue.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,11,0)
#define current linux_current
#include <linux/sched/mm.h>
#endif

#undef kfree

typedef long (*trap_handler)(task_t, ...);

static void *commpage32, *commpage64;
bool debug_output = 0;

struct trap_entry {
	trap_handler handler;
	const char* name;
};

struct file_operations mach_chardev_ops = {
	.open           = mach_dev_open,
	.release        = mach_dev_release,
	.unlocked_ioctl = mach_dev_ioctl,
	.compat_ioctl   = mach_dev_ioctl,
	.mmap           = mach_dev_mmap,
	.owner          = THIS_MODULE
};

#define TRAP(num, impl) [num - DARLING_MACH_API_BASE] = { (trap_handler) impl, #impl }

static const struct trap_entry mach_traps[DARLING_MACH_API_COUNT] = {
	// GENERIC
	TRAP(NR_get_api_version, mach_get_api_version),

	// INTERNAL
	TRAP(NR_thread_death_announce, thread_death_announce_entry),
	TRAP(NR_fork_wait_for_child, fork_wait_for_child_entry),
	TRAP(NR_set_dyld_info, set_dyld_info_entry),
	TRAP(NR_stop_after_exec, stop_after_exec_entry),
	TRAP(NR_started_suspended, started_suspended_entry),
	TRAP(NR_kernel_printk, kernel_printk_entry),
	TRAP(NR_path_at, path_at_entry),
	TRAP(NR_get_tracer, get_tracer_entry),
	TRAP(NR_set_tracer, set_tracer_entry),
	TRAP(NR_tid_for_thread, tid_for_thread_entry),
	TRAP(NR_pid_get_state, pid_get_state_entry),
	TRAP(NR_task_64bit, task_64bit_entry),
	TRAP(NR_last_triggered_watchpoint, last_triggered_watchpoint_entry),
	TRAP(NR_handle_to_path, handle_to_path_entry),
	TRAP(NR_thread_suspended, thread_suspended_entry),

	// KQUEUE
	TRAP(NR_kqueue_create, kqueue_create_entry),
	TRAP(NR_kevent, kevent_trap),
	TRAP(NR_kevent64, kevent64_trap),
	TRAP(NR_kevent_qos, kevent_qos_trap),
	TRAP(NR_closing_descriptor, closing_descriptor_entry),

	// PTHREAD
	TRAP(NR_pthread_kill_trap, pthread_kill_trap),
	TRAP(NR_pthread_markcancel, pthread_markcancel_entry),
	TRAP(NR_pthread_canceled, pthread_canceled_entry),

	// PSYNCH
	TRAP(NR_psynch_mutexwait_trap, psynch_mutexwait_trap),
	TRAP(NR_psynch_mutexdrop_trap, psynch_mutexdrop_trap),
	TRAP(NR_psynch_cvwait_trap, psynch_cvwait_trap),
	TRAP(NR_psynch_cvsignal_trap, psynch_cvsignal_trap),
	TRAP(NR_psynch_cvbroad_trap, psynch_cvbroad_trap),
	TRAP(NR_psynch_rw_rdlock, psynch_rw_rdlock_trap),
	TRAP(NR_psynch_rw_wrlock, psynch_rw_wrlock_trap),
	TRAP(NR_psynch_rw_unlock, psynch_rw_unlock_trap),
	TRAP(NR_psynch_cvclrprepost, psynch_cvclrprepost_trap),

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
	TRAP(NR__kernelrpc_mach_vm_allocate_trap, _kernelrpc_mach_vm_allocate_entry),
	TRAP(NR__kernelrpc_mach_vm_deallocate_trap, _kernelrpc_mach_vm_deallocate_entry),
	TRAP(NR_thread_get_special_reply_port, thread_get_special_reply_port_entry),
	TRAP(NR__kernelrpc_mach_port_request_notification_trap, _kernelrpc_mach_port_request_notification_entry),
	TRAP(NR__kernelrpc_mach_port_get_attributes_trap, _kernelrpc_mach_port_get_attributes_entry),
	TRAP(NR__kernelrpc_mach_port_type_trap, _kernelrpc_mach_port_type_entry),
	TRAP(NR__kernelrpc_mach_port_construct_trap, _kernelrpc_mach_port_construct_entry),
	TRAP(NR__kernelrpc_mach_port_destruct_trap, _kernelrpc_mach_port_destruct_entry),
	TRAP(NR__kernelrpc_mach_port_guard_trap, _kernelrpc_mach_port_guard_entry),
	TRAP(NR__kernelrpc_mach_port_unguard_trap, _kernelrpc_mach_port_unguard_entry),

	// MKTIMER
	TRAP(NR_mk_timer_create_trap, mk_timer_create_entry),
	TRAP(NR_mk_timer_arm_trap, mk_timer_arm_entry),
	TRAP(NR_mk_timer_cancel_trap, mk_timer_cancel_entry),
	TRAP(NR_mk_timer_destroy_trap, mk_timer_destroy_entry),

	// TASKS
	TRAP(NR_task_for_pid_trap, task_for_pid_entry),
	TRAP(NR_pid_for_task_trap, pid_for_task_entry),
	TRAP(NR_task_name_for_pid_trap, task_name_for_pid_entry),

	// BSD
	TRAP(NR_getuidgid, getuidgid_entry),
	TRAP(NR_setuidgid, setuidgid_entry),
	TRAP(NR_sigprocess, sigprocess_entry),
	TRAP(NR_ptrace_thupdate, ptrace_thupdate_entry),
	TRAP(NR_ptrace_sigexc, ptrace_sigexc_entry),
	TRAP(NR_fileport_makeport, fileport_makeport_entry),
	TRAP(NR_fileport_makefd, fileport_makefd_entry),
	TRAP(NR_sigprocess, sigprocess_entry),
	TRAP(NR_ptrace_thupdate, ptrace_thupdate_entry),
	TRAP(NR_set_thread_handles, set_thread_handles_entry),

	// VIRTUAL CHROOT
	TRAP(NR_vchroot, vchroot_entry),
	TRAP(NR_vchroot_expand, vchroot_expand_entry),
	TRAP(NR_vchroot_fdpath, vchroot_fdpath_entry),
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
	darling_pthread_kext_init();
	darling_procs_init();

	commpage32 = commpage_setup(false);
	commpage64 = commpage_setup(true);

	foreignmm_init();
	macho_binfmt_init();

	err = misc_register(&mach_dev);
	if (err < 0)
	 	goto fail;

	printk(KERN_INFO "Darling Mach: kernel emulation loaded, API version " DARLING_MACH_API_VERSION_STR "\n");
	return 0;

fail:
	printk(KERN_WARNING "Error loading Darling Mach: %d\n", err);

	commpage_free(commpage32);
	commpage_free(commpage64);
	foreignmm_exit();

	return err;
}
static void mach_exit(void)
{
	darling_procs_exit();
	darling_pthred_kext_exit();
	darling_xnu_deinit();
	misc_deregister(&mach_dev);
	printk(KERN_INFO "Darling Mach: kernel emulation unloaded\n");

	macho_binfmt_exit();
	foreignmm_exit();

	commpage_free(commpage32);
	commpage_free(commpage64);
}

int mach_dev_open(struct inode* ino, struct file* file)
{
	kern_return_t ret;
	task_t new_task, inherit_task, old_task, parent_task = NULL;
	thread_t new_thread;
	int ppid = -1;

	debug_msg("Setting up new XNU task for pid %d\n", linux_current->tgid);

	// Are we being opened after fork or execve?
	old_task = darling_task_get_current();
	if (old_task != NULL)
	{
		// execve case
		inherit_task = old_task;
	}
	else
	{
		// fork case
		ppid = linux_current->real_parent->tgid;
		inherit_task = parent_task = darling_task_get(ppid);

		// Try inheriting from VPID 1
		if (inherit_task == NULL)
		{
			unsigned int pid = 0;
			struct pid* pidobj;

			rcu_read_lock();

			pidobj = find_vpid(1);
			if (pidobj != NULL)
				pid = pid_nr(pidobj);

			rcu_read_unlock();

			if (pid != 0)
				inherit_task = darling_task_get(pid);
		}
	}

	ret = duct_task_create_internal(inherit_task, false, true, &new_task, linux_current);
	if (ret != KERN_SUCCESS)
		goto out;

	if (inherit_task != NULL)
	{
		new_task->tracer = inherit_task->tracer;

		task_lock(inherit_task);
		new_task->vchroot = mntget(inherit_task->vchroot);

		if (new_task->vchroot)
		{
			strcpy(new_task->vchroot_path, inherit_task->vchroot_path);
			debug_msg("- vchroot: %s\n", new_task->vchroot_path);
		}
		else
			debug_msg("- no vchroot\n");
		task_unlock(inherit_task);

		new_task->audit_token.val[1] = inherit_task->audit_token.val[1];
		new_task->audit_token.val[2] = inherit_task->audit_token.val[2];
	}
	else
	{
		// PID 1 case (or manually run Mach-O)
		new_task->tracer = 0;
		// UID and GID are left as 0, vchroot as NULL
	}
	new_task->audit_token.val[5] = task_pid_vnr(linux_current);
	file->private_data = new_task;

	debug_msg("mach_dev_open().0 refc %d\n", os_ref_get_count(&new_task->ref_count));

	// will automatically attach the BSD process to the Mach task
	proc_t new_proc = darling_proc_create(new_task);

	// Create a new thread_t
	ret = duct_thread_create(new_task, &new_thread);
	if (ret != KERN_SUCCESS)
	{
		duct_task_destroy(new_task);
		goto out;
	}

	debug_msg("thread %p refc %d at #1\n", new_thread, os_ref_get_count(&new_thread->ref_count));
	darling_task_register(new_task);
	darling_thread_register(new_thread);

	debug_msg("thread refc %d at #2\n", os_ref_get_count(&new_thread->ref_count));
	task_deallocate(new_task);
	thread_deallocate(new_thread);

out:
	if (old_task != NULL)
	{
		put_task_struct(old_task->map->linux_task);
		old_task->map->linux_task = NULL;
	}
	if (parent_task != NULL)
	{
		task_deallocate(parent_task);
		if (ret == KERN_SUCCESS)
		{
			darling_task_fork_child_done();
			darling_proc_post_notification(ppid, DTE_FORKED, linux_current->tgid);
			darling_proc_post_notification(linux_current->tgid, DTE_CHILD_ENTERING, ppid);
		}
	}

	if (ret != KERN_SUCCESS)
		return -LINUX_EINVAL;
	return 0;
}

kern_return_t xnu_kthread_register(void)
{
	thread_t new_thread;
	kern_return_t ret;

	// Create a new thread_t
	ret = duct_thread_create(kernel_task, &new_thread);
	if (ret != KERN_SUCCESS)
	{
		return ret;
	}

	darling_thread_register(new_thread);

	thread_deallocate(new_thread);
	return KERN_SUCCESS;
}

kern_return_t xnu_kthread_deregister(void)
{
	thread_t cur_thread = darling_thread_get_current();
	
	task_lock(kernel_task);
	queue_remove(&kernel_task->threads, cur_thread, thread_t, task_threads);
	kernel_task->thread_count--;
	task_unlock(kernel_task);

	if (cur_thread->linux_task != NULL)
	{
		put_task_struct(cur_thread->linux_task);
		cur_thread->linux_task = NULL;
	}
	darling_thread_deregister(cur_thread);

	return KERN_SUCCESS;
}

// No vma from 4.11
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
static vm_fault_t mach_mmap_fault(struct vm_fault *vmf)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
static int mach_mmap_fault(struct vm_fault *vmf)
#else
static int mach_mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
	struct vm_area_struct *vma = vmf->vma;
#endif
	uint8_t* commpage = (uint8_t*) vma->vm_private_data;

	vmf->page = vmalloc_to_page(commpage + (vmf->pgoff << PAGE_SHIFT));
	if (!vmf->page)
		return VM_FAULT_SIGBUS;

	get_page(vmf->page);

	return 0;
}

static const struct vm_operations_struct mach_mmap_ops = {
	.fault      = mach_mmap_fault,
};

int mach_dev_mmap(struct file* file, struct vm_area_struct *vma)
{
	unsigned long length = vma->vm_end - vma->vm_start;

	if (vma->vm_pgoff != 0)
		return -LINUX_EINVAL;

	if (test_thread_flag(TIF_IA32))
	{
		if (length != commpage_length(false))
			return -LINUX_EINVAL;

		vma->vm_private_data = commpage32;
	}
	else
	{
		if (length != commpage_length(true))
			return -LINUX_EINVAL;

		vma->vm_private_data = commpage64;
	}

	vma->vm_ops = &mach_mmap_ops;
	vma->vm_flags |= VM_DONTCOPY | VM_DONTEXPAND;
	vma->vm_flags &= ~VM_WRITE;

	return 0;
}

struct file* xnu_task_setup(void)
{
	struct file* file;
	int err;
	unsigned long addr;

	file = anon_inode_getfile("[commpage]", &mach_chardev_ops, NULL, O_RDWR);
	if (IS_ERR(file))
		return file;

	err = mach_dev_open(NULL, file);
	if (err != 0)
	{
		fput(file);
		return ERR_PTR(err);
	}

	return file;
}

int commpage_install(struct file* xnu_task)
{
	unsigned long addr;
	bool _64bit = !test_thread_flag(TIF_IA32);

	addr = vm_mmap(xnu_task, commpage_address(_64bit), commpage_length(_64bit), PROT_READ, MAP_SHARED | MAP_FIXED, 0);

	if (IS_ERR((void __force *)addr))
		return (long)addr;
	return 0;
}

int mach_dev_release(struct inode* ino, struct file* file)
{
	thread_t thread, cur_thread;
	task_t my_task = (task_t) file->private_data;
	bool exec_case;
	proc_t my_proc = (proc_t)my_task->bsd_info;
	
	// close(/dev/mach) may happen on any thread, even on a thread that
	// has never seen any ioctl() calls into LKM.
	if (!darling_thread_get_current())
	{
		debug_msg("No current thread!\n");
		// return -LINUX_EINVAL;
		thread_self_trap_entry(my_task);
	}
	
	exec_case = my_task->map->linux_task == NULL;
	if (!exec_case)
	{
		// Now that we're exiting, simulate the effects of ptrace(PTRACE_ATTACH)
		// as if it were called on us by our tracer, so that the tracer can use wait()
		// on us once we become a zombie, and retrieve the exit code.
		int tracer_pid = my_task->tracer;
		if (tracer_pid != 0 && list_empty(&linux_current->ptrace_entry))
		{
			// The following mimics the steps of ptrace_link().
			// ptrace_link() holds this spinlock, which is no longer exported from Linux.
			// I'd feel more comfortable holding it, however, since our process is already exiting,
			// it shouldn't be possible to ptrace() us and race with us by ordinary syscall means.

			//write_lock_irq(&tasklist_lock);
			rcu_read_lock();

			struct task_struct* tracer = pid_task(find_pid_ns(tracer_pid, &init_pid_ns), PIDTYPE_PID);

			if (tracer != NULL)
			{
				const struct cred* ptracer_cred = __task_cred(tracer);

				list_add(&linux_current->ptrace_entry, &tracer->ptraced);
				linux_current->ptrace = PT_PTRACED;
				linux_current->parent = tracer;
				linux_current->ptracer_cred = get_cred(ptracer_cred);
			}
			//write_unlock_irq(&tasklist_lock);

			rcu_read_unlock();
		}
		darling_proc_post_notification(my_task->map->linux_task->tgid, DTE_EXITED, current->exit_code);
	}

	// This works around an occasional race caused by the above trick when the debugger is terminating.
	// Without this, this BUG_ON sometimes fires: https://github.com/torvalds/linux/blob/master/include/linux/ptrace.h#L237
	INIT_LIST_HEAD(&current->ptraced);
	
	darling_task_deregister(my_task);
	// darling_thread_deregister(NULL);
	
	cur_thread = darling_thread_get_current();
	
	debug_msg("Destroying XNU task for pid %d, refc %d, exec_case %d\n", linux_current->tgid, os_ref_get_count(&my_task->ref_count), exec_case);
	
	task_lock(my_task);
	//queue_iterate(&my_task->threads, thread, thread_t, task_threads)
	while (!queue_empty(&my_task->threads))
	{
		thread = (thread_t) queue_first(&my_task->threads);
		
		// debug_msg("mach_dev_release() - thread refc %d\n", os_ref_get_count(&thread->ref_count));
		
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
	my_task->thread_count = 0;
	
	if (my_task->map->linux_task != NULL)
	{
		put_task_struct(my_task->map->linux_task);
		my_task->map->linux_task = NULL;
	}

	if (my_task->vchroot != NULL)
		mntput(my_task->vchroot);

	if (!exec_case && my_proc && cur_thread->uthread) {
		// prevent `darling_proc_destroy` from panicking when it sees a uthread still attached to the process
		proc_lock(my_proc);
		TAILQ_REMOVE(&my_proc->p_uthlist, (uthread_t)cur_thread->uthread, uu_list);
		proc_unlock(my_proc);
	}

	task_unlock(my_task);
	duct_task_destroy(my_task);

	//debug_msg("Destroying XNU task for pid %d, refc %d\n", linux_current->pid, os_ref_get_count(&my_task->ref_count));
	//duct_task_destroy(my_task);
	
	//duct_thread_destroy(cur_thread);
	if (!exec_case)
	{
		if (cur_thread->linux_task != NULL)
		{
			put_task_struct(cur_thread->linux_task);
			cur_thread->linux_task = NULL;
		}
		darling_thread_deregister(cur_thread);
	}

	return 0;
}

static struct file* mach_dev_afterfork(struct file* oldfile)
{
	if (unlikely(linux_current->mm->binfmt != &macho_format))
	{
		printk(KERN_NOTICE "Mach ioctl called from a non Mach-O process.\n");
		return NULL;
	}

	struct file* newfile = xnu_task_setup();
	if (IS_ERR(newfile))
	{
		printk(KERN_ERR "xnu_task_setup() failed\n");
		return NULL;
	}

	commpage_install(newfile);

	// Swap out the fd's file* for a new one
	spin_lock(&linux_current->files->file_lock);

	struct fdtable *fdt = files_fdtable(linux_current->files);
	int i;

	for (i = 0; i < fdt->max_fds; i++)
	{
		if (fdt->fd[i] == oldfile)
		{
			fput(oldfile);
			fdt->fd[i] = get_file(newfile);
		}
	}

	spin_unlock(&linux_current->files->file_lock);

	return newfile;
}

extern kern_return_t task_resume(task_t);
extern void thread_save_regs(thread_t);
long mach_dev_ioctl(struct file* file, unsigned int ioctl_num, unsigned long ioctl_paramv)
{
	const struct trap_entry* entry;
	long rv = 0;
	pid_t owner;
	bool need_fput = false;

	task_t task = (task_t) file->private_data;

	if (!task->map->linux_task)
		return -LINUX_ENXIO;

	rcu_read_lock();
	owner = task_tgid_nr(task->map->linux_task);
	rcu_read_unlock();

	if (owner != task_tgid_nr(linux_current))
	{
		if ((file = mach_dev_afterfork(file)) == NULL)
		{
			debug_msg("Your /dev/mach fd was opened in a different process!\n");
			return -LINUX_EINVAL;
		}
		else
		{
			// We need to hold an extra reference throughout this call, or else a bad guy's
			// thread might try to kill our fd while we execute.
			need_fput = true;
		}
	}

	ioctl_num -= DARLING_MACH_API_BASE;

	if (ioctl_num >= DARLING_MACH_API_COUNT)
	{
		rv = -LINUX_ENOSYS;
		goto out;
	}

	entry = &mach_traps[ioctl_num];
	if (!entry->handler)
	{
		rv = -LINUX_ENOSYS;
		goto out;
	}

	debug_msg("function %s (0x%x) called...\n", entry->name, ioctl_num);

	if (!darling_thread_get_current())
	{
		debug_msg("New thread! Registering.\n");
		thread_self_trap_entry(task);
	}

	rv = entry->handler(task, ioctl_paramv);

	debug_msg("...%s returned %ld\n", entry->name, rv);

out:
	if (need_fput)
	{
		debug_msg("Need fput!\n");
		fput(file);
	}

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

int thread_get_special_reply_port_entry(task_t task) {
	return thread_get_special_reply_port(NULL);
};

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
	// out.notify = args.notify;
	out.rcv_msg = (user_addr_t) args.rcv_msg;

	return XNU_CONTINUATION_ENABLED(mach_msg_overwrite_trap(&out));
}

int _kernelrpc_mach_port_deallocate_entry(task_t task, struct mach_port_deallocate_args* in_args)
{
	struct _kernelrpc_mach_port_deallocate_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;

	return XNU_CONTINUATION_ENABLED(_kernelrpc_mach_port_deallocate_trap(&out));
}

int _kernelrpc_mach_port_destroy(task_t task, struct mach_port_destroy_args* in_args)
{
	struct _kernelrpc_mach_port_destroy_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;

	return XNU_CONTINUATION_ENABLED(_kernelrpc_mach_port_destroy_trap(&out));
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

int _kernelrpc_mach_port_request_notification_entry(task_t task, struct mach_port_request_notification_args* in_args) {
	struct _kernelrpc_mach_port_request_notification_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;
	out.msgid = args.message_id;
	out.sync = args.make_send_count;
	out.notify = args.notification_destination_port_name;
	out.notifyPoly = args.message_type;
	out.previous = args.previous_destination_port_name_out;

	return _kernelrpc_mach_port_request_notification_trap(&out);
};

int _kernelrpc_mach_port_get_attributes_entry(task_t task, struct mach_port_get_attributes_args* in_args) {
	struct _kernelrpc_mach_port_get_attributes_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;
	out.flavor = args.flavor;
	out.info = args.info_out;
	out.count = args.count_out;

	return _kernelrpc_mach_port_get_attributes_trap(&out);
};

int _kernelrpc_mach_port_type_entry(task_t task, struct mach_port_type_args* in_args) {
	struct _kernelrpc_mach_port_type_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;
	out.ptype = args.port_type_out;

	return _kernelrpc_mach_port_type_trap(&out);
};

int _kernelrpc_mach_port_construct_entry(task_t task, struct mach_port_construct_args* in_args) {
	struct _kernelrpc_mach_port_construct_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.options = args.options;
	out.context = args.context;
	out.name = args.port_right_name_out;

	return _kernelrpc_mach_port_construct_trap(&out);
};

int _kernelrpc_mach_port_destruct_entry(task_t task, struct mach_port_destruct_args* in_args) {
	struct _kernelrpc_mach_port_destruct_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;
	out.srdelta = args.srdelta;
	out.guard = args.guard;

	return _kernelrpc_mach_port_destruct_trap(&out);
};

int _kernelrpc_mach_port_guard_entry(task_t task, struct mach_port_guard_args* in_args) {
	struct _kernelrpc_mach_port_guard_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;
	out.guard = args.guard;
	out.strict = args.strict;

	return _kernelrpc_mach_port_guard_trap(&out);
};

int _kernelrpc_mach_port_unguard_entry(task_t task, struct mach_port_unguard_args* in_args) {
	struct _kernelrpc_mach_port_unguard_args out;
	copyargs(args, in_args);

	out.target = args.task_right_name;
	out.name = args.port_right_name;
	out.guard = args.guard;

	return _kernelrpc_mach_port_unguard_trap(&out);
};

// This structure is also reused for mach_vm_deallocate
struct vm_allocate_params
{
	unsigned long address, size;
	int flags;
	int prot;
};

static void mach_vm_allocate_helper(struct vm_allocate_params* params)
{
	params->address = vm_mmap(NULL, params->address, params->size, params->prot, params->flags, 0);
}

int _kernelrpc_mach_vm_allocate_entry(task_t task_self, struct mach_vm_allocate_args* in_args)
{
	copyargs(args, in_args);

	int rv = KERN_SUCCESS;
	task_t task = port_name_to_task(args.target);

	if (!task || !task->map || !task->map->linux_task)
		return KERN_INVALID_TASK;

	struct vm_allocate_params params;

	params.flags = MAP_ANONYMOUS | MAP_PRIVATE;
	if (!(args.flags & VM_FLAGS_ANYWHERE))
		params.flags |= MAP_FIXED;

	params.prot = PROT_READ | PROT_WRITE | PROT_EXEC;
	params.address = args.address;
	params.size = args.size;

	struct mm_struct* mm = get_task_mm(task->map->linux_task);

	if (mm != NULL)
	{
		foreignmm_execute(mm, (foreignmm_cb_t) mach_vm_allocate_helper, &params);
		mmput(mm);

		if (IS_ERR_VALUE(params.address))
			rv = KERN_FAILURE;
		else
		{
			uint64_t addr64 = params.address;
			if (copy_to_user(&in_args->address, &addr64, sizeof(addr64)))
				rv = KERN_INVALID_ARGUMENT;
		}
	}
	else
		rv = KERN_INVALID_TASK; // It's a zombie or something

	task_deallocate(task);
	return rv;
}

static void mach_vm_deallocate_helper(struct vm_allocate_params* params)
{
	params->address = vm_munmap(params->address, params->size);
}

int _kernelrpc_mach_vm_deallocate_entry(task_t task_self, struct mach_vm_deallocate_args* in_args)
{
	copyargs(args, in_args);

	int rv = KERN_SUCCESS;
	task_t task = port_name_to_task(args.target);

	if (!task || !task->map || !task->map->linux_task)
		return KERN_INVALID_TASK;

	struct vm_allocate_params params;

	params.address = args.address;
	params.size = args.size;

	struct mm_struct* mm = get_task_mm(task->map->linux_task);

	if (mm != NULL)
	{
		foreignmm_execute(mm, (foreignmm_cb_t) mach_vm_deallocate_helper, &params);
		mmput(mm);

		if (IS_ERR_VALUE(params.address))
			rv = KERN_FAILURE;
	}
	else
		rv = KERN_INVALID_TASK; // It's a zombie or something

	task_deallocate(task);
	return rv;
}

int semaphore_signal_entry(task_t task, struct semaphore_signal_args* in_args)
{
	struct semaphore_signal_trap_args out;
	copyargs(args, in_args);

	out.signal_name = args.signal;

	return XNU_CONTINUATION_ENABLED(semaphore_signal_trap(&out));
}

int semaphore_signal_all_entry(task_t task, struct semaphore_signal_all_args* in_args)
{
	struct semaphore_signal_all_trap_args out;
	copyargs(args, in_args);

	out.signal_name = args.signal;

	return XNU_CONTINUATION_ENABLED(semaphore_signal_all_trap(&out));
}

int semaphore_wait_entry(task_t task, struct semaphore_wait_args* in_args)
{
	struct semaphore_wait_trap_args out;
	copyargs(args, in_args);

	out.wait_name = args.signal;

	return XNU_CONTINUATION_ENABLED(semaphore_wait_trap(&out));
}

int semaphore_wait_signal_entry(task_t task, struct semaphore_wait_signal_args* in_args)
{
	struct semaphore_wait_signal_trap_args out;
	copyargs(args, in_args);

	out.wait_name = args.wait;
	out.signal_name = args.signal;

	return XNU_CONTINUATION_ENABLED(semaphore_wait_signal_trap(&out));
}

int semaphore_timedwait_signal_entry(task_t task, struct semaphore_timedwait_signal_args* in_args)
{
	struct semaphore_timedwait_signal_trap_args out;
	copyargs(args, in_args);

	out.wait_name = args.wait;
	out.signal_name = args.signal;
	out.sec = args.sec;
	out.nsec = args.nsec;

	return XNU_CONTINUATION_ENABLED(semaphore_timedwait_signal_trap(&out));
}

int semaphore_timedwait_entry(task_t task, struct semaphore_timedwait_args* in_args)
{
	struct semaphore_timedwait_trap_args out;
	copyargs(args, in_args);

	out.wait_name = args.wait;
	out.sec = args.sec;
	out.nsec = args.nsec;

	return XNU_CONTINUATION_ENABLED(semaphore_timedwait_trap(&out));
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
		if (thread->linux_task != NULL)
		{
			put_task_struct(thread->linux_task);
			thread->linux_task = NULL;
		}

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

unsigned int vpid_to_pid(unsigned int vpid)
{
	struct pid* pidobj;
	unsigned int pid = 0;

	rcu_read_lock();

	pidobj = find_vpid(vpid);
	if (pidobj != NULL)
		pid = pid_nr(pidobj);

	rcu_read_unlock();
	return pid;
}

unsigned int pid_to_vpid(unsigned int pid)
{
	struct pid* pidobj;
	unsigned int vpid = 0;

	rcu_read_lock();

	pidobj = find_pid_ns(pid, &init_pid_ns);
	if (pidobj != NULL)
		vpid = pid_vnr(pidobj);

	rcu_read_unlock();
	return vpid;
}

int task_for_pid_entry(task_t task, struct task_for_pid* in_args)
{
	unsigned int pid;
	task_t task_out;
	int port_name;
	ipc_port_t sright;

	copyargs(args, in_args);

	// Convert virtual PID to global PID
	pid = vpid_to_pid(args.pid);

	debug_msg("- task_for_pid(): pid %d -> %d\n", args.pid, pid);
	if (pid == 0)
		return KERN_FAILURE;
	
	// Lookup task in task registry
	task_out = darling_task_get(pid);
	if (task_out == NULL)
		return KERN_FAILURE;
	
	// Turn it into a send right
	sright = convert_task_to_port(task_out);
	port_name = ipc_port_copyout_send(sright, task->itk_space);
	
	if (copy_to_user(args.task_port, &port_name, sizeof(port_name)))
		return KERN_INVALID_ADDRESS;

	return KERN_SUCCESS;
}

int task_name_for_pid_entry(task_t task, struct task_name_for_pid* in_args)
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

	debug_msg("- task_name_for_pid(): pid %d -> %d\n", args.pid, pid);
	if (pid == 0)
		return KERN_FAILURE;
	
	// Lookup task in task registry
	task_out = darling_task_get(pid);
	if (task_out == NULL)
		return KERN_FAILURE;
	
	// Turn it into a send right
	sright = convert_task_name_to_port(task_out);
	port_name = ipc_port_copyout_send(sright, task->itk_space);
	
	if (copy_to_user(&in_args->name_out, &port_name, sizeof(port_name)))
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

int tid_for_thread_entry(task_t task, void* tport_in)
{
	int tid;
	thread_t t = port_name_to_thread((int)(long) tport_in, PORT_TO_THREAD_NONE);

	if (!t || !t->linux_task)
		return -1;

	tid = task_pid_vnr(t->linux_task);
	thread_deallocate(t);

	return tid;
}

int task_pid(task_t task)
{
	return (task && task->map && task->map->linux_task) ? task_pid_vnr(task->map->linux_task) : -1;
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
	printk(KERN_DEBUG "Darling TID %d (PID %d) says: %s\n", current->pid, current->tgid, args.buf);

	return 0;
}

// Evaluate given path relative to provided fd (or AT_FDCWD).
int path_at_entry(task_t task, struct path_at_args* in_args)
{
	struct path path;
	int error = 0;

	copyargs(args, in_args);

	error = user_path_at(args.fd, args.path, LOOKUP_FOLLOW | LOOKUP_EMPTY, &path);
	if (error)
		return error;

	char* buf = (char*) __get_free_page(GFP_USER);
	char* name = d_path(&path, buf, PAGE_SIZE);
	unsigned long len;

	if (IS_ERR(name))
	{
		error = PTR_ERR(name);
		goto failure;
	}

	len = strlen(name) + 1;
	if (len > args.max_path_out)
		error = -LINUX_ENAMETOOLONG;
	else if (copy_to_user(args.path_out, name, len))
		error = -LINUX_EFAULT;

failure:
	free_page((unsigned long) buf);
	path_put(&path);
	return error;
}

// These ported BSD syscalls have a different way of reporting errors.
// This is because BSD (unlike Linux) doesn't report errors by returning a negative number.
#define BSD_ENTRY(name) \
	int name##_trap(task_t task, struct name##_args* in_args) \
	{ \
		copyargs(args, in_args); \
		uint32_t* retval = ((uthread_t)darling_thread_get_current()->uthread)->uu_rval; \
		int error = XNU_CONTINUATION_ENABLED(name((proc_t)task->bsd_info, &args, retval)); \
		if (error) \
			return -error; \
		return *retval; \
	}

BSD_ENTRY(psynch_mutexwait);
BSD_ENTRY(psynch_mutexdrop);
BSD_ENTRY(psynch_cvwait);
BSD_ENTRY(psynch_cvsignal);
BSD_ENTRY(psynch_cvbroad);
BSD_ENTRY(psynch_rw_rdlock);
BSD_ENTRY(psynch_rw_wrlock);
BSD_ENTRY(psynch_rw_unlock);
BSD_ENTRY(psynch_cvclrprepost);

int getuidgid_entry(task_t task, struct uidgid* in_args)
{
	struct uidgid rv = {
		.uid = task->audit_token.val[1],
		.gid = task->audit_token.val[2]
	};

	if (copy_to_user(in_args, &rv, sizeof(rv)))
		return -LINUX_EFAULT;
	return 0;
}

int setuidgid_entry(task_t task, struct uidgid* in_args)
{
	copyargs(args, in_args);

	if (args.uid != -1)
		task->audit_token.val[1] = args.uid;
	if (args.gid != -1)
		task->audit_token.val[2] = args.gid;

	return 0;
}

int ovl_darling_fake_fsuid(void)
{
	task_t task = darling_task_get_current();
	if (task == NULL)
		return from_kuid(&init_user_ns, current_fsuid());
	return task->audit_token.val[1];
}

int get_tracer_entry(task_t self, void* pid_in)
{
	task_t target_task;
	int rv;
	int pid = (int)(long) pid_in;

	if (pid == 0)
	{
		task_reference(self);
		target_task = self;
	}
	else
		target_task = darling_task_get(vpid_to_pid(pid));
	if (target_task == NULL)
		return -LINUX_ESRCH;

	rv = pid_to_vpid(target_task->tracer);
	task_deallocate(target_task);

	return rv;
}

int set_tracer_entry(task_t self, struct set_tracer_args* in_args)
{
	task_t target_task;
	int rv = 0;

	copyargs(args, in_args);
	if (args.target == 0)
	{
		task_reference(self);
		target_task = self;
	}
	else
		target_task = darling_task_get(vpid_to_pid(args.target));

	if (target_task == NULL)
		return -LINUX_ESRCH;

	if (args.tracer <= 0)
		target_task->tracer = 0;
	else if (target_task->tracer == 0)
		target_task->tracer = vpid_to_pid(args.tracer);
	else
		rv = -LINUX_EPERM; // already traced

	task_deallocate(target_task);
	return rv;
}

int pthread_markcancel_entry(task_t task, void* tport_in)
{
	// mark as canceled if cancelable
	thread_t t = port_name_to_thread((int)(long) tport_in, PORT_TO_THREAD_NONE);

	if (!t)
		return -LINUX_ESRCH;

	darling_thread_markcanceled(t->linux_task->pid);
	thread_deallocate(t);

	return 0;
}

int pthread_canceled_entry(task_t task, void* arg)
{
	switch ((int)(long) arg)
	{
		case 0:
			// is cancelable & canceled?
			if (darling_thread_canceled())
				return 0;
			else
				return -LINUX_EINVAL;
		case 1:
			// enable cancelation
			darling_thread_cancelable(true);
			return 0;
		case 2:
			// disable cancelation
			darling_thread_cancelable(false);
			return 0;
		default:
			return -LINUX_EINVAL;
	}
}

// The following 2 functions have been copied from kernel/pid.c,
// because these functions aren't exported to modules.
static struct task_struct *__find_task_by_pid_ns(pid_t nr, struct pid_namespace *ns)
{
	return pid_task(find_pid_ns(nr, ns), PIDTYPE_PID);
}

static struct task_struct *__find_task_by_vpid(pid_t vnr)
{
	return __find_task_by_pid_ns(vnr, task_active_pid_ns(linux_current));
}


int pid_get_state_entry(task_t task_self, void* pid_in)
{
	struct task_struct* task;
	int rv = 0;

	rcu_read_lock();

	task = __find_task_by_vpid((int)(long) pid_in);
	if (task != NULL)
		rv = task->state;

	rcu_read_unlock();
	return rv;
}

int started_suspended_entry(task_t task, void* arg)
{
	return darling_task_marked_start_suspended();
}

int task_64bit_entry(task_t task_self, void* pid_in)
{
	struct task_struct* task;
	int rv = -LINUX_ESRCH;

	rcu_read_lock();

	task = __find_task_by_vpid((int)(long) pid_in);
	if (task != NULL)
	{
		struct mm_struct* mm = get_task_mm(task);
		if (mm)
		{
			rv = mm->task_size > 0xffffffffull;
			mmput(mm);
		}
		else
			rv = 0;
	}

	rcu_read_unlock();
	return rv;
}

unsigned long last_triggered_watchpoint_entry(task_t task, struct last_triggered_watchpoint_args* in_args)
{
	thread_t thread = darling_thread_get_current();

	struct last_triggered_watchpoint_args out;
	out.address = thread->triggered_watchpoint_address;
	out.flags = thread->triggered_watchpoint_operation;

	if (copy_to_user(in_args, &out, sizeof(out)))
		return -LINUX_EFAULT;

	return 0;
}

int vchroot_entry(task_t task, int fd_vchroot)
{
	struct file* f;
	int err = 0;

	task_lock(task);
	f = fget(fd_vchroot);
	if (!f)
	{
		task_unlock(task);
		return -LINUX_EBADF;
	}

	if (task->vchroot)
	{
		mntput(task->vchroot);
		task->vchroot = NULL;
	}

	task->vchroot = clone_private_mount(&f->f_path);

	if (!IS_ERR(task->vchroot))
	{
		char* strpath = d_path(&f->f_path, task->vchroot_path, PAGE_SIZE);
		if (strpath != task->vchroot_path)
			memmove(task->vchroot_path, strpath, strlen(strpath)+1);
	}

	fput(f);

	if (IS_ERR(task->vchroot))
	{
		err = PTR_ERR(task->vchroot);
		task->vchroot = NULL;
	}
	task_unlock(task);

	return err;
}

// Helper function for binfmt.c
char* task_copy_vchroot_path(task_t task)
{
	char* rv = NULL;

	task_lock(task);
	if (task->vchroot)
	{
		const int len = strlen(task->vchroot_path);
		rv = kmalloc(len + 1, GFP_KERNEL);

		strcpy(rv, task->vchroot_path);
	}

	task_unlock(task);

	return rv;
}

// API exported by Linux, but not declared in the right headers
extern int vfs_path_lookup(struct dentry *, struct vfsmount *,
               const char *, unsigned int, struct path *);

int vchroot_expand_entry(task_t task, struct vchroot_expand_args __user* in_args)
{
	struct path path;
	struct file* frel = NULL;
	int err;
	struct dentry* relative_to;
	const char* append = NULL;
	struct vchroot_expand_args* args;

	struct vfsmount* vchroot = mntget(task->vchroot);

	if (vchroot == NULL)
		return 0; // no op

	args = kmalloc(sizeof(*args), GFP_KERNEL);
	if (IS_ERR(args))
		return PTR_ERR(args);

	if (copy_from_user(args, in_args, sizeof(*args)))
	{
		err = -LINUX_EFAULT;
		goto fail;
	}

	debug_msg("vchroot_expand %d %s\n", args->dfd, args->path);

	if (args->path[0] != '/')
	{
		if (args->dfd == AT_FDCWD)
			relative_to = linux_current->fs->pwd.dentry;
		else
		{
			frel = fget(args->dfd);

			if (!frel)
			{
				err = -LINUX_ENOENT;
				goto fail;
			}

			relative_to = frel->f_path.dentry;
		}
	}
	else
		relative_to = vchroot->mnt_root;

	err = vfs_path_lookup(relative_to, vchroot, args->path, (args->flags & VCHROOT_FOLLOW) ? LOOKUP_FOLLOW : 0, &path);
	if (err == -LINUX_ENOENT)
	{
		// Retry lookup w/o last path component
		char* slash = strrchr(args->path, '/');

		const char* lookup;
		if (slash != NULL)
		{
			*slash = '\0';
			append = slash + 1;
			lookup = args->path;
		}
		else if (args->path[0] && strcmp(args->path, ".") != 0 && strcmp(args->path, "..") != 0)
		{
			append = args->path;
			lookup = "";
		}
		else
			goto fail;

		err = vfs_path_lookup(relative_to, vchroot, lookup, (args->flags & VCHROOT_FOLLOW) ? LOOKUP_FOLLOW : 0, &path);
	}

	if (!err)
	{
		//path.mnt = linux_current->fs->root.mnt;

		char* str = (char*) __get_free_page(GFP_KERNEL);
		char* strpath = d_path(&path, str, PAGE_SIZE);
		// This would return the path within the filesystem where the file resides
		//char* strpath = dentry_path_raw(path.dentry, str, PAGE_SIZE);
		path_put(&path);

		if (IS_ERR(strpath))
		{
			free_page((unsigned long) str);
			err = PTR_ERR(strpath);
			goto fail;
		}

		debug_msg("- vchroot d_path returned %s\n", strpath);

		task_lock(task);
		int rootlen = strlen(task->vchroot_path);
		task_unlock(task);

		if (copy_to_user(in_args->path, task->vchroot_path, rootlen))
		{
			free_page((unsigned long) str);
			err = -LINUX_EFAULT;
			goto fail;
		}

		int pathlen = strlen(strpath);
		if (rootlen + pathlen + 1 > PAGE_SIZE)
		{
			free_page((unsigned long) str);
			err = -LINUX_ENAMETOOLONG;
			goto fail;
		}

		if (copy_to_user(in_args->path + rootlen, strpath, pathlen + 1))
		{
			free_page((unsigned long) str);
			err = -LINUX_EFAULT;
			goto fail;
		}
		free_page((unsigned long) str);

		// If the lookup only succeeded after removing the last path component, append it back now
		if (append != NULL)
		{
			if (rootlen + pathlen + strlen(append) + 2 > PAGE_SIZE)
			{
				err = -LINUX_ENAMETOOLONG;
				goto fail;
			}

			if (copy_to_user(in_args->path + pathlen, "/", 1)
					|| copy_to_user(in_args->path + pathlen + 1, append, strlen(append) + 1))
			{
				err = -LINUX_EFAULT;
				goto fail;
			}
		}

	}

fail:
	if (frel)
		fput(frel);
	mntput(vchroot);
	kfree(args);

	return err;
}

int vchroot_fdpath_entry(task_t task, struct vchroot_fdpath_args __user* in_args)
{
	int err = 0;
	struct file* f;
	char* str = NULL;

	copyargs(args, in_args);

	f = fget(args.fd);
	if (!f)
		return -LINUX_EBADF;

	struct path p;
	struct vfsmount* vchroot = mntget(task->vchroot);

	p.dentry = f->f_path.dentry;
	p.mnt = vchroot ? vchroot : f->f_path.mnt;

	str = (char*) __get_free_page(GFP_KERNEL);
	char* strpath = d_path(&p, str, PAGE_SIZE);

	if (IS_ERR(strpath))
	{
		err = PTR_ERR(strpath);
		goto fail;
	}

	if (vchroot && strcmp(strpath, "/") == 0 && p.dentry->d_sb != p.mnt->mnt_sb)
	{
		// Path resolution gave us /, but we know it's wrong, because it is not really / of our vchroot
		// -> we need to do this differently.

		// Try resolving this name with the standard system /
		strpath = d_path(&f->f_path, str, PAGE_SIZE);

		if (IS_ERR(strpath))
		{
			err = PTR_ERR(strpath);
			goto fail;
		}

		task_lock(task);
		int rootlen = strlen(task->vchroot_path);
		bool under_vchroot = strncmp(strpath, task->vchroot_path, rootlen) == 0;
		task_unlock(task);

		if (!under_vchroot)
		{
			// and prepend /sysroot to the result.
			const char root[] = "/Volumes/SystemRoot";
			int rootlen = sizeof(root) - 1;

			const int pathlen = strlen(strpath) + 1;
			if (rootlen + pathlen > args.maxlen)
			{
				err = -LINUX_ENAMETOOLONG;
				goto fail;
			}

			if (copy_to_user(args.path, root, rootlen))
			{
				err = -LINUX_ENAMETOOLONG;
				goto fail;
			}

			if (copy_to_user(args.path + rootlen, strpath, pathlen))
			{
				err = -LINUX_ENAMETOOLONG;
				goto fail;
			}
		}
		else
		{
			strpath += rootlen;

			const int pathlen = strlen(strpath) + 1;
			if (pathlen > args.maxlen)
			{
				err = -LINUX_ENAMETOOLONG;
				goto fail;
			}

			if (copy_to_user(args.path, strpath, pathlen))
			{
				err = -LINUX_ENAMETOOLONG;
				goto fail;
			}
		}
	}
	else
	{
		const int pathlen = strlen(strpath) + 1;
		if (pathlen > args.maxlen)
		{
			debug_msg("FDPATH #1 %d>%d", pathlen, args.maxlen);
			err = -LINUX_ENAMETOOLONG;
			goto fail;
		}

		debug_msg("FDPATH %d resolved to %s", args.fd, strpath);
		if (copy_to_user(args.path, strpath, pathlen))
		{
			err = -LINUX_EFAULT;
			goto fail;
		}
	}

fail:
	if (str)
		free_page((unsigned long) str);

	fput(f);

	if (vchroot)
		mntput(vchroot);
	return err;
}

// Callback for handle_to_path_entry()
static int all_acceptable(void* context, struct dentry* d)
{
	return 1;
}

// We need the original Linux functions
#undef task_lock
#undef task_unlock

int handle_to_path_entry(task_t t, struct handle_to_path_args* in_args)
{
	struct path path;
	struct file_handle* fh;
	int err = 0;
	char* str = NULL;

	copyargs(args, in_args);

	fh = (struct file_handle*) args.fh;

	spin_lock(&linux_current->fs->lock);
	struct fd f = fdget(args.mfd);

	if (!f.file)
	{
		spin_unlock(&linux_current->fs->lock);
		return ERR_PTR(-LINUX_EBADF);
	}

	path.mnt = mntget(f.file->f_path.mnt);
	path.dentry = NULL;

	fdput(f);
	spin_unlock(&linux_current->fs->lock);

	// Linux doesn't export the header files we'd need for this
#if 0
	path.mnt = NULL;
	task_lock(linux_current);

	struct mount* mnt;
	struct mnt_namespace* ns = linux_current->nsproxy->mnt_ns;
	list_for_each_entry(mnt, &ns->list, mnt_list)
	{
		if (mnt->mnt_id == args.mntid)
		{
			path.mnt = mntget(mnt->mnt);
			break;
		}
	}

	task_unlock(linux_current);
	
	if (path.mnt == NULL)
		return -LINUX_ENOENT;
#endif
	if (IS_ERR(path.mnt))
		return PTR_ERR(path.mnt);
	
	int handle_dwords = fh->handle_bytes >> 2;
	path.dentry = exportfs_decode_fh(path.mnt,
		(struct fid*) fh->f_handle, handle_dwords, fh->handle_type,
		all_acceptable, NULL);
	
	if (IS_ERR(path.dentry))
	{
		err = PTR_ERR(path.dentry);
		path.dentry = NULL;
		goto fail;
	}

	str = (char*) __get_free_page(GFP_KERNEL);
	char* strpath = d_path(&path, str, PAGE_SIZE);

	if (IS_ERR(strpath))
	{
		err = PTR_ERR(strpath);
		goto fail;
	}

	if (copy_to_user(in_args->path, strpath, strlen(strpath)+1))
	{
		err = -LINUX_EFAULT;
		goto fail;
	}

fail:
	if (str)
		free_page((unsigned long) str);

	mntput(path.mnt);
	if (path.dentry)
		dput(path.dentry);
		
	return err;
}

extern kern_return_t mach_port_deallocate(ipc_space_t, mach_port_name_t);
int fileport_makeport_entry(task_t task, struct fileport_makeport_args* in_args)
{
	copyargs(args, in_args);

	int err;

	struct file* f = fget(args.fd);
	if (f == NULL)
		return -LINUX_EBADF;
	
	ipc_port_t fileport = fileport_alloc((struct fileglob*) f);
	if (fileport == IPC_PORT_NULL)
	{
		fput(f);
		return -LINUX_EAGAIN;
	}

	mach_port_name_t name = ipc_port_copyout_send(fileport, task->itk_space);
	if (!MACH_PORT_VALID(name))
	{
		err = -LINUX_EINVAL;
		goto out;
	}

	if (copy_to_user(&in_args->port_out, &name, sizeof(name)))
	{
		err = -LINUX_EFAULT;
		goto out;
	}

	return 0;
out:
	if (MACH_PORT_VALID(name))
		mach_port_deallocate(task->itk_space, name);
	return err;
}

void fileport_releasefg(struct fileglob* fg)
{
	struct file* f = (struct file*) fg;
	fput(f);
}

int fileport_makefd_entry(task_t task, void* port_in)
{
	mach_port_name_t send = (mach_port_name_t)(uintptr_t) port_in;

 	struct file* f;
	ipc_port_t port = IPC_PORT_NULL;
	kern_return_t kr;
	int err;

	// NOTE(@facekapow): `ipc_object_copyin` got a few extra parameters in the last update; i just followed suit with what the XNU code passes in for them
	//                   we might need to revisit them later on (particularly `IPC_KMSG_FLAGS_ALLOW_IMMOVABLE_SEND`), but they seem pretty harmless
	kr = ipc_object_copyin(task->itk_space, send, MACH_MSG_TYPE_COPY_SEND, (ipc_object_t*) &port, 0, NULL, IPC_KMSG_FLAGS_ALLOW_IMMOVABLE_SEND);

	if (kr != KERN_SUCCESS)
	{
		err = -LINUX_EINVAL;
		goto out;
	}

	f = (struct file*) fileport_port_to_fileglob(port);
	if (f == NULL)
	{
		err = -LINUX_EINVAL;
		goto out;
	}

	int fd = get_unused_fd_flags(O_CLOEXEC);
	fd_install(fd, f);
	err = fd;

out:
	if (port != IPC_PORT_NULL)
		ipc_port_release_send(port);

	return err;
}

extern kern_return_t
thread_set_state(
    register thread_t       thread,
    int                     flavor,
    thread_state_t          state,
    mach_msg_type_number_t  state_count);
extern kern_return_t
thread_get_state(
	thread_t		thread,
	int						flavor,
	thread_state_t			state,
	mach_msg_type_number_t	*state_count);

#define LINUX_SI_USER 0
extern kern_return_t bsd_exception(
	exception_type_t	exception,
	mach_exception_data_t	code,
	mach_msg_type_number_t  codeCnt);

extern kern_return_t
thread_set_state(
    register thread_t       thread,
    int                     flavor,
    thread_state_t          state,
    mach_msg_type_number_t  state_count);
extern kern_return_t
thread_get_state(
    register thread_t       thread,
    int                     flavor,
    thread_state_t          state,          /* pointer to OUT array */
    mach_msg_type_number_t  *state_count);

static int state_to_kernel(const struct thread_state* state)
{
#ifdef __x86_64__
	if (!test_thread_flag(TIF_IA32))
	{
		x86_thread_state64_t tstate;
		x86_float_state64_t fstate;

		if (copy_from_user(&tstate, state->tstate, sizeof(tstate))
			|| copy_from_user(&fstate, state->fstate, sizeof(fstate)))
		{
			return -LINUX_EFAULT;
		}

		thread_set_state(current_thread(), x86_THREAD_STATE64, (thread_state_t) &tstate, x86_THREAD_STATE64_COUNT);
		thread_set_state(current_thread(), x86_FLOAT_STATE64, (thread_state_t) &fstate, x86_FLOAT_STATE64_COUNT);
	}
	else
	{
		x86_thread_state32_t tstate;
		x86_float_state32_t fstate;

		if (copy_from_user(&tstate, state->tstate, sizeof(tstate))
			|| copy_from_user(&fstate, state->fstate, sizeof(fstate)))
		{
			return -LINUX_EFAULT;
		}

		thread_set_state(current_thread(), x86_THREAD_STATE32, (thread_state_t) &tstate, x86_THREAD_STATE32_COUNT);
		thread_set_state(current_thread(), x86_FLOAT_STATE32, (thread_state_t) &fstate, x86_FLOAT_STATE32_COUNT);
	}
#else
#	warning Missing code for this arch!
#endif
	return 0;
}

static int state_from_kernel(struct thread_state* state)
{
#ifdef __x86_64__
	if (!test_thread_flag(TIF_IA32))
	{
		x86_thread_state64_t tstate;
		x86_float_state64_t fstate;
		mach_msg_type_number_t count;

		count = x86_THREAD_STATE64_COUNT;
		thread_get_state(current_thread(), x86_THREAD_STATE64, (thread_state_t) &tstate, &count);

		count = x86_FLOAT_STATE64_COUNT;
		thread_get_state(current_thread(), x86_FLOAT_STATE64, (thread_state_t) &fstate, &count);

		if (copy_to_user(state->tstate, &tstate, sizeof(tstate))
			|| copy_to_user(state->fstate, &fstate, sizeof(fstate)))
		{
			return -LINUX_EFAULT;
		}
	}
	else
	{
		x86_thread_state32_t tstate;
		x86_float_state32_t fstate;
		mach_msg_type_number_t count;

		count = x86_THREAD_STATE32_COUNT;
		thread_get_state(current_thread(), x86_THREAD_STATE32, (thread_state_t) &tstate, &count);

		count = x86_FLOAT_STATE32_COUNT;
		thread_get_state(current_thread(), x86_FLOAT_STATE32, (thread_state_t) &fstate, &count);

		if (copy_to_user(state->tstate, &tstate, sizeof(tstate))
			|| copy_to_user(state->fstate, &fstate, sizeof(fstate)))
		{
			return -LINUX_EFAULT;
		}
	}
#else
#	warning Missing code for this arch!
#endif
	return 0;
}

static void thread_suspended_logic(task_t task)
{
	while (current_thread()->suspend_count > 0 && !fatal_signal_pending(linux_current))
	{
		debug_msg("sigexc - thread(%p) susp: %d, task susp: %d\n",
			current_thread(),
			current_thread()->suspend_count, task->suspend_count);

		// Cannot use TASK_STOPPED, because Linux doesn't export wake_up_state()
		// and TASK_STOPPED isn't TASK_NORMAL.
		set_current_state(TASK_KILLABLE);
		schedule();
		set_current_state(TASK_RUNNING);
		debug_msg("sigexc - woken up\n");
	}

	if (signal_pending(linux_current) && task->suspend_count)
		task_resume(task);
}

// we want Linux's `si_pid` here
#undef si_pid

int sigprocess_entry(task_t task, struct sigprocess_args* in_args)
{
	thread_t thread = current_thread();
	proc_t proc = current_proc();
	copyargs(args, in_args);

	int err = state_to_kernel(&args.state);
	if (err != 0)
		return err;

	debug_msg("sigprocess_entry, linux_signal=%d\n", args.siginfo.si_signo);

	thread->pending_signal = 0;
	thread->in_sigprocess = TRUE;

	// if the signal was process-wide (i.e. we didn't send it to ourselves via pthread_kill)...
	if (proc && args.siginfo.linux_si_pid != task_pid(thread->task)) {
		// then notify any EVFILT_SIGNAL knotes on the process
		proc_knote(proc, NOTE_SIGNAL | args.signum);
	}

	mach_exception_data_type_t codes[EXCEPTION_CODE_MAX] = { 0, 0 };
	if (args.siginfo.si_code == LINUX_SI_USER)
	{
		if (task->sigexc)
		{
			codes[0] = EXC_SOFT_SIGNAL;	
			codes[1] = args.signum; // signum has the BSD signal number
			bsd_exception(EXC_SOFTWARE, codes, 2);
		}
		else
		{
			thread->pending_signal = args.signum;
		}

		goto out;
	}

	int mach_exception = 0;
	switch (args.siginfo.si_signo) // signo has the Linux signal number
	{
		case LINUX_SIGSEGV: // KERN_INVALID_ADDRESS
			mach_exception = EXC_BAD_ACCESS;
			codes[0] = KERN_INVALID_ADDRESS;
			codes[1] = args.siginfo._sifields._sigfault._addr;
			break;
		case LINUX_SIGBUS:
			mach_exception = EXC_BAD_ACCESS;
			codes[0] = EXC_I386_ALIGNFLT;
			break;
		case LINUX_SIGILL:
			mach_exception = EXC_BAD_INSTRUCTION;
			codes[0] = EXC_I386_INVOP;
			break;
		case LINUX_SIGFPE:
			mach_exception = EXC_ARITHMETIC;
			codes[0] = args.siginfo.si_code;
			break;
		case LINUX_SIGTRAP:
			mach_exception = EXC_BREAKPOINT;
			codes[0] = (args.siginfo.si_code == SI_KERNEL) ? EXC_I386_BPT : EXC_I386_SGL;

			if (args.siginfo.si_code == TRAP_HWBKPT)
				codes[1] = thread->triggered_watchpoint_address;
			break;
		/*
		case LINUX_SIGSYS:
			mach_exception = EXC_SOFTWARE;
			if (codes[0] == 0)
				codes[0] = EXC_UNIX_BAD_SYSCALL;
		case LINUX_SIGPIPE:
			mach_exception = EXC_SOFTWARE;
			if (codes[0] == 0)
				codes[0] = EXC_UNIX_BAD_PIPE;
		case LINUX_SIGABRT:
			mach_exception = EXC_SOFTWARE;
			if (codes[0] == 0)
				codes[0] = EXC_UNIX_ABORT;
		*/
		default:
			if (task->sigexc)
			{
				if (codes[0] == 0)
					codes[0] = EXC_SOFT_SIGNAL;
				codes[1] = args.signum;
				bsd_exception(EXC_SOFTWARE, codes, 2);
			}
			else
			{
				thread->pending_signal = args.signum;
			}
			goto out;
	}

	debug_msg("calling exception_triage_thread(%d, [%ld, %ld])\n", mach_exception, codes[0], codes[1]);

	exception_triage_thread(mach_exception, codes, EXCEPTION_CODE_MAX, thread);

	debug_msg("exception_triage_thread returned\n");

out:
	// LLDB commonly suspends the thread upon reception of an exception and assumes
	// that the thread will stay suspended after replying to the exception message,
	// until thread_resume() is called.
	thread_suspended_logic(task);

	err = state_from_kernel(&args.state);
	if (err != 0)
		return err;

	// Check and copy thread->pending_signal
	if (copy_to_user(&in_args->signum, &thread->pending_signal, sizeof(in_args->signum)))
		return -LINUX_EFAULT;

	thread->in_sigprocess = FALSE;
	return 0;
}

#define si_pid xnu_si_pid

int ptrace_thupdate_entry(task_t task, struct ptrace_thupdate_args* in_args)
{
	copyargs(args, in_args);

	thread_t target_thread;

	rcu_read_lock();
	target_thread = darling_thread_get(args.tid);
	if (target_thread != NULL)
		target_thread->pending_signal = args.signum;
	rcu_read_unlock();

	return 0;
}

int ptrace_sigexc_entry(task_t task, struct ptrace_sigexc_args* in_args)
{
	copyargs(args, in_args);

	unsigned int pid;
	task_t that_task;
	
	// Convert virtual PID to global PID
	pid = vpid_to_pid(args.pid);

	if (pid == 0)
		return KERN_FAILURE;
	
	// Lookup task in task registry
	that_task = darling_task_get(pid);
	if (that_task == NULL)
		return KERN_FAILURE;

	that_task->sigexc = args.sigexc;

	if (that_task->user_stop_count)
	{
		debug_msg("sigexc target task is stopped (%d), resuming\n", that_task->user_stop_count);
		task_resume(that_task);
	}

	task_deallocate(that_task);
	return KERN_SUCCESS;
}

int thread_suspended_entry(task_t task, struct thread_suspended_args* in_args)
{
	copyargs(args, in_args);

	int err = state_to_kernel(&args.state);
	if (err != 0)
		return err;

	thread_suspended_logic(task);

	err = state_from_kernel(&args.state);
	if (err != 0)
		return err;

	return 0;
}

int set_thread_handles_entry(task_t t, struct set_thread_handles_args* in_args)
{
	copyargs(args, in_args);

	thread_t thread = current_thread();

	thread->pthread_handle = args.pthread_handle;
	thread->dispatch_qaddr = args.dispatch_qaddr;

	return 0;
}

int kqueue_create_entry(task_t task) {
	return darling_kqueue_create(task);
};

extern int kevent(struct proc* p, struct kevent_args* uap, int32_t* retval);
extern int kevent64(struct proc* p, struct kevent64_args* uap, int32_t* retval);
extern int kevent_qos(struct proc* p, struct kevent_qos_args* uap, int32_t* retval);

BSD_ENTRY(kevent);
BSD_ENTRY(kevent64);
BSD_ENTRY(kevent_qos);

int closing_descriptor_entry(task_t task, struct closing_descriptor_args* in_args) {
	copyargs(args, in_args);
	return darling_closing_descriptor(task, args.fd);
};

module_init(mach_init);
module_exit(mach_exit);

