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

#endif
