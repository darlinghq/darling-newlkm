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

#ifndef LKM_API_H
#define LKM_API_H

#ifdef KERNEL
#	include <linux/types.h>
#	include <asm/ucontext.h>
#	include <linux/signal.h>
#else
#	include <stdint.h>
#endif

#define darling_mach_xstr(a) darling_mach_str(a)
#define darling_mach_str(a) #a

#define DARLING_MACH_API_VERSION		19
#define DARLING_MACH_API_VERSION_STR	darling_mach_xstr(DARLING_MACH_API_VERSION)

#define DARLING_MACH_API_BASE		0x1000

// we pass these directly to their BSD functions; the BSD functions have their own padding format (see sysproto.h)
#define DARLING_BSD_ARG(_type, _name) _type _name; char _ ## _name ## _padding[(sizeof(uint64_t) <= sizeof(_type) ? 0 : sizeof(uint64_t) - sizeof(_type))]

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
	NR_fork_wait_for_child,
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
	NR_vchroot, // fd as paramv
	NR_vchroot_expand,
	NR_vchroot_fdpath,
	NR_handle_to_path,
	NR_fileport_makeport,
	NR_fileport_makefd,
	NR_sigprocess,
	NR_ptrace_thupdate,
	NR_ptrace_sigexc,
	NR_thread_suspended,
	NR_set_thread_handles,
	NR_thread_get_special_reply_port,
	NR__kernelrpc_mach_port_request_notification_trap,
	NR__kernelrpc_mach_port_type_trap,
	NR__kernelrpc_mach_port_get_attributes_trap,
	NR__kernelrpc_mach_port_construct_trap,
	NR__kernelrpc_mach_port_destruct_trap,
	NR__kernelrpc_mach_port_guard_trap,
	NR__kernelrpc_mach_port_unguard_trap,
	NR_kqueue_create,
	NR_kevent,
	NR_kevent64,
	NR_kevent_qos,
	NR_closing_descriptor,

	DARLING_MACH_API_ALMOST_COUNT, // don't use this directly (it's offset by the API base number); use `DARLING_MACH_API_COUNT`
};

#define DARLING_MACH_API_COUNT (DARLING_MACH_API_ALMOST_COUNT - DARLING_MACH_API_BASE)

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

struct mach_port_request_notification_args {
	unsigned int task_right_name;
	unsigned int port_right_name;
	int message_id;
	unsigned int make_send_count;
	unsigned int notification_destination_port_name;
	unsigned int message_type;
	unsigned int* previous_destination_port_name_out;
};

struct mach_port_get_attributes_args {
	unsigned int task_right_name;
	unsigned int port_right_name;
	int flavor;
	void* info_out;
	void* count_out;
};

struct mach_port_type_args {
	unsigned int task_right_name;
	unsigned int port_right_name;
	unsigned int* port_type_out;
};

struct mach_port_construct_args {
	unsigned int task_right_name;
	uint64_t options;
	uint64_t context;
	uint64_t port_right_name_out;
};

struct mach_port_destruct_args {
	unsigned int task_right_name;
	unsigned int port_right_name;
	uint64_t srdelta;
	uint64_t guard;
};

struct mach_port_guard_args {
	unsigned int task_right_name;
	unsigned int port_right_name;
	uint64_t guard;
	unsigned char strict;
};

struct mach_port_unguard_args {
	unsigned int task_right_name;
	unsigned int port_right_name;
	uint64_t guard;
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
	DARLING_BSD_ARG(uint64_t, stackaddr);
	DARLING_BSD_ARG(uint64_t, freesize);
	DARLING_BSD_ARG(uint32_t, port);
	DARLING_BSD_ARG(uint32_t, sem);
};

struct psynch_mutexwait_args
{
	DARLING_BSD_ARG(uint64_t, mutex);
	DARLING_BSD_ARG(uint32_t, mgen);
	DARLING_BSD_ARG(uint32_t, ugen);
	DARLING_BSD_ARG(uint64_t, tid);
	DARLING_BSD_ARG(uint32_t, flags);
};

struct psynch_mutexdrop_args
{
	DARLING_BSD_ARG(uint64_t, mutex);
	DARLING_BSD_ARG(uint32_t, mgen);
	DARLING_BSD_ARG(uint32_t, ugen);
	DARLING_BSD_ARG(uint64_t, tid);
	DARLING_BSD_ARG(uint32_t, flags);
};

struct pthread_kill_args
{
	int thread_port;
	int sig;
};

struct psynch_cvwait_args
{
	DARLING_BSD_ARG(uint64_t, cv);
	DARLING_BSD_ARG(uint64_t, cvlsgen);
	DARLING_BSD_ARG(uint32_t, cvugen);
	DARLING_BSD_ARG(uint64_t, mutex);
	DARLING_BSD_ARG(uint64_t, mugen);
	DARLING_BSD_ARG(uint32_t, flags);
	DARLING_BSD_ARG(int64_t, sec);
	DARLING_BSD_ARG(uint32_t, nsec);
};

struct psynch_cvsignal_args
{
	DARLING_BSD_ARG(uint64_t, cv);
	DARLING_BSD_ARG(uint64_t, cvlsgen);
	DARLING_BSD_ARG(uint32_t, cvugen);
	DARLING_BSD_ARG(int, thread_port);
	DARLING_BSD_ARG(uint64_t, mutex);
	DARLING_BSD_ARG(uint64_t, mugen);
	DARLING_BSD_ARG(uint64_t, tid);
	DARLING_BSD_ARG(uint32_t, flags);
};

struct psynch_cvbroad_args
{
	DARLING_BSD_ARG(uint64_t, cv);
	DARLING_BSD_ARG(uint64_t, cvlsgen);
	DARLING_BSD_ARG(uint64_t, cvudgen);
	DARLING_BSD_ARG(uint32_t, flags);
	DARLING_BSD_ARG(uint64_t, mutex);
	DARLING_BSD_ARG(uint64_t, mugen);
	DARLING_BSD_ARG(uint64_t, tid);
};

struct psynch_cvclrprepost_args
{
	DARLING_BSD_ARG(uint64_t, cv);
	DARLING_BSD_ARG(uint32_t, cvgen);
	DARLING_BSD_ARG(uint32_t, cvugen);
	DARLING_BSD_ARG(uint32_t, cvsgen);
	DARLING_BSD_ARG(uint32_t, prepocnt);
	DARLING_BSD_ARG(uint32_t, preposeq);
	DARLING_BSD_ARG(uint32_t, flags);
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
	DARLING_BSD_ARG(uint64_t, rwlock);
	DARLING_BSD_ARG(uint32_t, lgenval);
	DARLING_BSD_ARG(uint32_t, ugenval);
	DARLING_BSD_ARG(uint32_t, rw_wc);
	DARLING_BSD_ARG(int, flags);
};

struct psynch_rw_wrlock_args
{
	DARLING_BSD_ARG(uint64_t, rwlock);
	DARLING_BSD_ARG(uint32_t, lgenval);
	DARLING_BSD_ARG(uint32_t, ugenval);
	DARLING_BSD_ARG(uint32_t, rw_wc);
	DARLING_BSD_ARG(int, flags);
};

struct psynch_rw_unlock_args
{
	DARLING_BSD_ARG(uint64_t, rwlock);
	DARLING_BSD_ARG(uint32_t, lgenval);
	DARLING_BSD_ARG(uint32_t, ugenval);
	DARLING_BSD_ARG(uint32_t, rw_wc);
	DARLING_BSD_ARG(int, flags);
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

#define VCHROOT_FOLLOW	1
struct vchroot_expand_args
{
	char path[4096]; // contains evaluated path on return
	unsigned int flags;
	int dfd; // base directory when path is relative
};

struct vchroot_fdpath_args
{
	int fd;
	char* path;
	unsigned int maxlen;
};

// Like open_by_handle_at, but provides a path and doesn't need CAP_DAC_READ_SEARCH
struct handle_to_path_args
{
	int mfd; // in
	//int mntid; // in
	char fh[80]; // in
	char path[4096]; // out
};

struct fileport_makeport_args
{
	int fd;
	int port_out;
};

struct thread_state
{
	// x86_THREAD_STATE64, x86_THREAD_STATE32
	void* tstate;
	// x86_FLOAT_STATE64, x86_FLOAT_STATE32
	void* fstate;
};

#ifdef LINUX_SIGACTION_H
#	define siginfo linux_siginfo
#	define ucontext linux_ucontext
#endif

#if defined(LINUX_SIGACTION_H) || defined(KERNEL)
struct sigprocess_args
{
	int signum;
	// in/out
	struct thread_state state;

	struct siginfo siginfo;
};
#endif

#ifdef LINUX_SIGACTION_H
#	undef siginfo
#	undef ucontext
#endif

struct thread_suspended_args
{
	// in/out
	struct thread_state state;
};

struct ptrace_thupdate_args
{
	int tid;
	int signum;
};

struct ptrace_sigexc_args
{
	int pid;
	int sigexc;
};

struct set_thread_handles_args
{
	unsigned long long pthread_handle;
	unsigned long long dispatch_qaddr;
};

struct kevent_args {
	DARLING_BSD_ARG(int, fd);
	DARLING_BSD_ARG(uint64_t, changelist);
	DARLING_BSD_ARG(int, nchanges);
	DARLING_BSD_ARG(uint64_t, eventlist);
	DARLING_BSD_ARG(int, nevents);
	DARLING_BSD_ARG(uint64_t, timeout);
};

struct kevent64_args {
	DARLING_BSD_ARG(int, fd);
	DARLING_BSD_ARG(uint64_t, changelist);
	DARLING_BSD_ARG(int, nchanges);
	DARLING_BSD_ARG(uint64_t, eventlist);
	DARLING_BSD_ARG(int, nevents);
	DARLING_BSD_ARG(unsigned int, flags);
	DARLING_BSD_ARG(uint64_t, timeout);
};

struct kevent_qos_args {
	DARLING_BSD_ARG(int, fd);
	DARLING_BSD_ARG(uint64_t, changelist);
	DARLING_BSD_ARG(int, nchanges);
	DARLING_BSD_ARG(uint64_t, eventlist);
	DARLING_BSD_ARG(int, nevents);
	DARLING_BSD_ARG(uint64_t, data_out);
	DARLING_BSD_ARG(uint64_t, data_available);
	DARLING_BSD_ARG(unsigned int, flags);
};

struct closing_descriptor_args {
	int fd;
};

#pragma pack (pop)

#endif
