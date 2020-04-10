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

#ifndef LINUXMACH_H
#define LINUXMACH_H
#include <linux/fs.h>
#include "api.h"

typedef struct task *task_t;

// XNU kprintf
extern void kprintf(const char* format, ...);

int mach_dev_open(struct inode* ino, struct file* file);
int mach_dev_release(struct inode* ino, struct file* file);
long mach_dev_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_paramv);
int mach_dev_mmap(struct file* file, struct vm_area_struct *vma);

int mach_get_api_version(task_t task);
int mach_reply_port_entry(task_t task);
int _kernelrpc_mach_port_mod_refs_entry(task_t task, struct mach_port_mod_refs_args* args);
int task_self_trap_entry(task_t task);
int host_self_trap_entry(task_t task);
int thread_self_trap_entry(task_t task);
int _kernelrpc_mach_port_allocate_entry(task_t task, struct mach_port_allocate_args* args);
int mach_msg_overwrite_entry(task_t task, struct mach_msg_overwrite_args* args);
int _kernelrpc_mach_port_deallocate_entry(task_t task, struct mach_port_deallocate_args* args);
int _kernelrpc_mach_port_destroy(task_t task, struct mach_port_destroy_args* args);
int semaphore_signal_entry(task_t task, struct semaphore_signal_args* args);
int semaphore_signal_all_entry(task_t task, struct semaphore_signal_all_args* args);
int semaphore_wait_entry(task_t task, struct semaphore_wait_args* args);
int semaphore_wait_signal_entry(task_t task, struct semaphore_wait_signal_args* args);
int semaphore_timedwait_signal_entry(task_t task, struct semaphore_timedwait_signal_args* args);
int semaphore_timedwait_entry(task_t task, struct semaphore_timedwait_args* args);
int _kernelrpc_mach_port_move_member_entry(task_t task, struct mach_port_move_member_args* args);
int _kernelrpc_mach_port_extract_member_entry(task_t task, struct mach_port_extract_member_args* args);
int _kernelrpc_mach_port_insert_member_entry(task_t task, struct mach_port_insert_member_args* args);
int _kernelrpc_mach_port_insert_right_entry(task_t task, struct mach_port_insert_right_args* args);
int _kernelrpc_mach_vm_allocate_entry(task_t task, struct mach_vm_allocate_args* args);
int _kernelrpc_mach_vm_deallocate_entry(task_t task, struct mach_vm_deallocate_args* args);

int mk_timer_create_entry(task_t task);
int mk_timer_arm_entry(task_t task, struct mk_timer_arm_args* args);
int mk_timer_cancel_entry(task_t task, struct mk_timer_cancel_args* args);
int mk_timer_destroy_entry(task_t task, struct mk_timer_destroy_args* args);

int thread_death_announce_entry(task_t task);

int fork_wait_for_child_entry(task_t task);

int evproc_create_entry(task_t task, struct evproc_create* args);
int task_for_pid_entry(task_t task, struct task_for_pid* args);
int task_name_for_pid_entry(task_t task, struct task_name_for_pid* args);
int pid_for_task_entry(task_t task, struct pid_for_task* args);
int tid_for_thread_entry(task_t task, void* tport_in);

int set_dyld_info_entry(task_t task, struct set_dyld_info_args* args);
int stop_after_exec_entry(task_t task);
int kernel_printk_entry(task_t task, struct kernel_printk_args* args);

int evfilt_machport_open_entry(task_t task, struct evfilt_machport_open_args* args);
int path_at_entry(task_t task, struct path_at_args* args);

int psynch_mutexwait_trap(task_t task, struct psynch_mutexwait_args* args);
int psynch_mutexdrop_trap(task_t task, struct psynch_mutexdrop_args* args);
int psynch_cvwait_trap(task_t task, struct psynch_cvwait_args* args);
int psynch_cvsignal_trap(task_t task, struct psynch_cvsignal_args* args);
int psynch_cvbroad_trap(task_t task, struct psynch_cvbroad_args* args);
int psynch_rw_rdlock_trap(task_t task, struct psynch_rw_rdlock_args* args);
int psynch_rw_wrlock_trap(task_t task, struct psynch_rw_wrlock_args* args);
int psynch_rw_unlock_trap(task_t task, struct psynch_rw_unlock_args* args);
int psynch_cvclrprepost_trap(task_t task, struct psynch_cvclrprepost_args* args);

int getuidgid_entry(task_t task, struct uidgid* args);
int setuidgid_entry(task_t task, struct uidgid* args);

int get_tracer_entry(task_t, void* pid);
int set_tracer_entry(task_t, struct set_tracer_args* args);
int pid_get_state_entry(task_t task, void* pid_in);

int pthread_markcancel_entry(task_t task, void* arg);
int pthread_canceled_entry(task_t task, void* arg);
int started_suspended_entry(task_t task, void* arg);

int task_64bit_entry(task_t, void* pid_in);

unsigned long last_triggered_watchpoint_entry(task_t, struct last_triggered_watchpoint_args* args);

int vchroot_entry(task_t task, int fd_vchroot);
int vchroot_expand_entry(task_t task, struct vchroot_expand_args __user* path);
int vchroot_fdpath_entry(task_t task, struct vchroot_fdpath_args __user* args);

int handle_to_path_entry(task_t, struct handle_to_path_args* args);
int fileport_makeport_entry(task_t, struct fileport_makeport_args* args);
int fileport_makefd_entry(task_t, void* port_in);

int sigprocess_entry(task_t task, struct sigprocess_args* args);

#endif
