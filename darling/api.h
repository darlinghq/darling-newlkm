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

#ifndef LKM_API_H
#define LKM_API_H

#ifdef KERNEL
#	include <linux/types.h>
#else
#	include <stdint.h>
#endif

#define darling_mach_xstr(a) darling_mach_str(a)
#define darling_mach_str(a) #a

#define DARLING_MACH_API_VERSION		15
#define DARLING_MACH_API_VERSION_STR	darling_mach_xstr(DARLING_MACH_API_VERSION)

#define DARLING_MACH_API_BASE		0x1000

#pragma pack (push, 1)

enum { NR_get_api_version = DARLING_MACH_API_BASE,
	NR_mach_reply_port,
	NR__kernelrpc_mach_port_mod_refs,
	NR_task_self_trap,
	NR_host_self_trap,
	NR__kernelrpc_mach_port_allocate,
	NR_mach_msg_overwrite_trap,
	NR__kernelrpc_mach_port_deallocate,
	NR__kernelrpc_mach_port_destroy,
	NR_semaphore_signal_trap,
	NR_semaphore_signal_all_trap,
	NR_semaphore_wait_trap,
	NR_semaphore_wait_signal_trap,
	NR_semaphore_timedwait_signal_trap,
	NR_semaphore_timedwait_trap,
	NR_bsd_ioctl_trap,
	NR_thread_self_trap,
	NR_bsdthread_terminate_trap,
	NR_psynch_mutexwait_trap, // 0x12
	NR_psynch_mutexdrop_trap,
	NR_pthread_kill_trap,
	NR_psynch_cvwait_trap, // 0x15
	NR_psynch_cvsignal_trap,
	NR_psynch_cvbroad_trap,
	NR_mk_timer_create_trap,
	NR_mk_timer_arm_trap,
	NR_mk_timer_cancel_trap,
	NR_mk_timer_destroy_trap,
	NR__kernelrpc_mach_port_move_member_trap,
	NR__kernelrpc_mach_port_insert_member_trap,
	NR__kernelrpc_mach_port_extract_member_trap,
	NR_thread_death_announce,
	NR__kernelrpc_mach_port_insert_right_trap, // 0x20
	NR_evfilt_machport_open,
	NR_fork_wait_for_child,
	NR_evproc_create,
	NR_task_for_pid_trap,
	NR_pid_for_task_trap,
	NR_set_dyld_info,
	NR_stop_after_exec,
	NR_kernel_printk, // 0x28
	NR_path_at,
	NR_psynch_rw_rdlock,
	NR_psynch_rw_wrlock,
	NR_psynch_rw_unlock,
	NR_psynch_cvclrprepost,
	NR_get_tracer,
	NR_tid_for_thread,
	NR_getuidgid,
	NR_setuidgid,
	NR_task_name_for_pid_trap,
	NR_set_tracer,
	NR_pthread_markcancel,
	NR_pthread_canceled,
	NR_pid_get_state,
	NR_started_suspended,
	NR_task_64bit,
	NR__kernelrpc_mach_vm_allocate_trap,
	NR__kernelrpc_mach_vm_deallocate_trap,
	NR_last_triggered_watchpoint,
};

struct set_tracer_args
{
	int target;
	int tracer;
};

struct task_name_for_pid
{
	int task_port;
	int pid;
	int name_out;
};

struct uidgid
{
	int uid;
	int gid;
};

struct path_at_args
{
	int fd;
	const char* path;
	char* path_out;
	unsigned int max_path_out;
};

struct evproc_create
{
	unsigned int pid;
	unsigned int flags;
};

// What is read from the fd
struct evproc_event
{
	unsigned int event;
	unsigned int extra;
};

// What can be written into the fd (to update rcv buffer)
struct evpset_options
{
	void* rcvbuf;
#ifdef __i386__
	unsigned int pad1;
#endif
	unsigned long rcvbuf_size;
#ifdef __i386__
	unsigned int pad2;
#endif
	unsigned int sfflags;
};

struct evfilt_machport_open_args
{
	unsigned int port_name;
	struct evpset_options opts;
};

// What is read from the fd
struct evpset_event
{
	unsigned int flags; // kn_flags, override flags in libkqueue if non-zero
	unsigned int port; // kn_data
	unsigned long msg_size; // kn_ext[1]
#ifdef __i386__
	unsigned int pad1;
#endif
	unsigned int receive_status; // kn_fflags
};

struct mach_port_insert_right_args
{
	unsigned int task_right_name;
	unsigned int port_name;
	unsigned int right_name;
	int right_type;
};

struct mach_port_mod_refs_args
{
	unsigned int task_right_name;
	unsigned int port_right_name;
	int right_type;
	int delta;
};

struct mach_port_allocate_args
{
	unsigned int task_right_name;
	int right_type;
	unsigned int* out_right_name;
#ifdef __i386__
	uint32_t pad1;
#endif
};

struct mach_msg_overwrite_args
{
#ifdef __i386__
	void* msg;
	uint32_t pad1;
#else
	void* msg;
#endif
	unsigned int option;
	unsigned int send_size;
	unsigned int recv_size;
	unsigned int rcv_name;
	unsigned int timeout;
	unsigned int notify;
#ifdef __i386__
	void* rcv_msg;
	uint32_t pad2;
#else
	void* rcv_msg;
#endif
	unsigned int rcv_limit;
};

struct mach_port_deallocate_args
{
	unsigned int task_right_name;
	unsigned int port_right_name;
};

struct mach_port_destroy_args
{
	unsigned int task_right_name;
	unsigned int port_right_name;
};

struct mach_port_move_member_args
{
	unsigned int task_right_name;
	unsigned int port_right_name;
	unsigned int pset_right_name;
};

struct mach_port_insert_member_args
{
	unsigned int task_right_name;
	unsigned int port_right_name;
	unsigned int pset_right_name;
};

struct mach_port_extract_member_args
{
	unsigned int task_right_name;
	unsigned int port_right_name;
	unsigned int pset_right_name;
};

struct semaphore_signal_args
{
	unsigned int signal;
};

struct semaphore_signal_all_args
{
	unsigned int signal;
};

struct semaphore_wait_args
{
	unsigned int signal;
};

struct semaphore_wait_signal_args
{
	unsigned int wait;
	unsigned int signal;
};

struct semaphore_timedwait_signal_args
{
	unsigned int wait;
	unsigned int signal;
	unsigned int sec;
	unsigned int nsec;
};

struct semaphore_timedwait_args
{
	unsigned int wait;
	unsigned int sec;
	unsigned int nsec;
};

struct bsd_ioctl_args
{
	int fd;
	uint64_t request;
	uint64_t arg;
};

struct bsdthread_terminate_args
{
	uint64_t stackaddr;
	uint32_t freesize;
	unsigned int thread_right_name;
	unsigned int signal;
};

struct psynch_mutexwait_args
{
	uint64_t mutex;
	uint32_t mgen;
	uint32_t ugen;
	uint64_t tid;
	uint32_t flags;
};

struct psynch_mutexdrop_args
{
	uint64_t mutex;
	uint32_t mgen;
	uint32_t ugen;
	uint64_t tid;
	uint32_t flags;
};

struct pthread_kill_args
{
	int thread_port;
	int sig;
};

struct psynch_cvwait_args
{
	unsigned long cv;
	uint64_t cvlsgen;
	uint32_t cvugen;
	unsigned long mutex;
	uint64_t mugen;
	uint32_t flags;
	int64_t sec;
	uint32_t nsec;
};

struct psynch_cvsignal_args
{
	unsigned long cv;
	uint64_t cvlsgen;
	uint32_t cvugen;
	int thread_port;
	unsigned long mutex;
	uint64_t mugen;
	uint64_t tid;
	uint32_t flags;
};

struct psynch_cvbroad_args
{
	uint64_t cv;
	uint64_t cvlsgen;
	uint64_t cvudgen;
	uint32_t flags;
	uint64_t mutex;
	uint64_t mugen;
	uint64_t tid;
};

struct psynch_cvclrprepost_args
{
	uint64_t cv;
	uint32_t cvgen;
	uint32_t cvugen;
	uint32_t cvsgen;
	uint32_t prepocnt;
	uint32_t preposeq;
	uint32_t flags;
};

struct mk_timer_arm_args
{
	unsigned int timer_port;
	/* This is mach_absolute_time() based time */
	uint64_t expire_time;
};

struct mk_timer_cancel_args
{
	unsigned int timer_port;
	uint64_t* result_time;
#ifdef __i386__
	uint32_t pad1;
#endif
};

struct mk_timer_destroy_args
{
	unsigned int timer_port;
};

struct task_for_pid
{
	unsigned int pid;
	unsigned int* task_port;
};

struct pid_for_task
{
	unsigned int task_port;
	unsigned int* pid;
};

struct set_dyld_info_args
{
	uint64_t all_images_address;
	uint32_t all_images_length;
};

struct kernel_printk_args
{
	char buf[512];
};

struct psynch_rw_rdlock_args
{
	uint64_t rwlock;
	uint32_t lgenval;
	uint32_t ugenval;
	uint32_t rw_wc;
	int flags;
};

struct psynch_rw_wrlock_args
{
	uint64_t rwlock;
	uint32_t lgenval;
	uint32_t ugenval;
	uint32_t rw_wc;
	int flags;
};

struct psynch_rw_unlock_args
{
	uint64_t rwlock;
	uint32_t lgenval;
	uint32_t ugenval;
	uint32_t rw_wc;
	int flags;
};

struct mach_vm_allocate_args
{
	unsigned int target;
	uint64_t address;
	uint64_t size;
	unsigned int flags;
};

struct mach_vm_deallocate_args
{
	unsigned int target;
	uint64_t address;
	uint64_t size;
};

struct last_triggered_watchpoint_args
{
	uint64_t address;
	unsigned int flags;
};

#pragma pack (pop)

#endif
