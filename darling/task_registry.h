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
#ifndef _TASK_REGISTRY_H
#define _TASK_REGISTRY_H

typedef struct task *task_t;
typedef struct thread *thread_t;
typedef struct uthread *uthread_t;

enum {
	TASK_KEY_PSYNCH_MUTEX = 0,
	TASK_KEY_PSYNCH_CV,
	TASK_KEY_COUNT = TASK_KEY_PSYNCH_CV + 1
};

void darling_task_init(void);

task_t darling_task_get_current(void);

// Adds reference to the returned task!
task_t darling_task_get(int pid);
thread_t darling_thread_get_current(void);

// Caller MUST hold a RCU lock!
thread_t darling_thread_get(unsigned int tid);

void darling_task_register(task_t t);
void darling_task_deregister(task_t t);

void darling_task_fork_wait_for_child(void);
void darling_task_fork_child_done(void);
void darling_task_set_dyld_info(unsigned long all_img_location, unsigned long all_img_length);
void darling_task_get_dyld_info(unsigned int pid, unsigned long long* all_img_location, unsigned long long* all_img_length);
void darling_task_mark_start_suspended(void);
_Bool darling_task_marked_start_suspended(void);

typedef enum darling_proc_event {
	/**
	 * @brief An unknown event (usually indicates an error/bug).
	 */
	DTE_UNKNOWN        = 0x00,

	/**
	 * @brief Sent when the process exits.
	 * `extra` will contain the process' exit code.
	 */
	DTE_EXITED         = 0x01,

	/**
	 * @brief Sent on the parent process when it's forked.
	 * `extra` will contain the child process' Linux PID.
	 */
	DTE_FORKED         = 0x02,

	/**
	 * @brief Sent when the process is replaced by another (e.g. via the `exec()` family of system calls).
	 * `extra` will contain `0`.
	 */
	DTE_REPLACED       = 0x04,

	/**
	 * @brief Sent when the process is being reaped (e.g. via the `wait()` family of system calls).
	 * `extra` will contain the Linux PID of the process reaping it.
	 */
	DTE_REAPED         = 0x08,

	/**
	 * @brief Sent on the child process of a fork when it's created.
	 * `extra` will contain the parent process' Linux PID.
	 *
	 * Note the this event is sent immediately upon child creation.
	 * As such, it is very likely that only listeners of the parent process will have a chance to
	 * attach a listener onto the child process in time to catch the event.
	 */
	DTE_CHILD_ENTERING = 0x10,
} darling_proc_event_t;

/**
 * @brief Indicates that you want to listen for all possible events.
 */
#define DTE_ALL_EVENTS (DTE_EXITED | DTE_FORKED | DTE_REPLACED | DTE_REAPED | DTE_CHILD_ENTERING)

/**
 * @brief Convenient bitmask that can be used to check if an event indicates that the PID you're listening to no longer refers to the same process.
 */
#define DTE_DIED (DTE_EXITED | DTE_REPLACED)

/**
 * @brief A process notification listener
 * @param pid The PID that the event is about
 * @param context Additional data passed to `darling_proc_notify_register` when registering this listener
 * @param event The event that occurred on the process
 * @param extra Additional event-dependent data
 */
typedef void (*darling_proc_notification_listener)(int pid, void* context, darling_proc_event_t event, unsigned long int extra);

/**
 * @brief Registers a notification listener for the given PID
 *
 * Listeners are automatically removed when a process with the given PID exits.
 * Listeners are NOT automatically removed when a process with the given PID is replaced (e.g. via `exec()` and friends).
 *
 * @param pid The PID to listen to
 * @param listener The listener function to call when an event is posted on the process
 * @param context Additional data passed to the listener when a notification is posted
 * @param event_mask List of events you want to listen for (ORed together). You can pass `DTE_ALL_EVENTS` to listen for all possible events.
 * @returns A positive registration ID that can be used to uniquely identify this listener.
 *          If an error occurred, the negated value of the error is returned.
 */
long int darling_proc_notify_register(int pid, darling_proc_notification_listener listener, void* context, darling_proc_event_t event_mask);

/**
 * @brief Unregisters a notification listener with the given ID
 * @param pid The PID that the listener is listening to
 * @param registration_id The registration ID of the listener, returned by `darling_proc_notify_register`
 * @returns `true` if the listener was unregistered, `false` otherwise
 */
_Bool darling_proc_notify_deregister(int pid, long int registration_id);

/**
 * @brief Posts an event for the given PID
 *
 * Note that you should *not* deliver multiple events at once by ORing them together.
 * Call this function separately for each event you have ready.
 *
 * @param pid The PID to post the event for
 * @param event The event to post
 * @param extra Additional event-dependent data
 * @returns `true` if the event was posted, 
 */
_Bool darling_proc_post_notification(int pid, darling_proc_event_t event, unsigned long int extra);

void darling_thread_register(thread_t t);
void darling_thread_deregister(thread_t t);
_Bool darling_thread_canceled(void);
void darling_thread_markcanceled(unsigned int pid);
void darling_thread_cancelable(_Bool cancelable);

// Poor man's task-local storage
typedef void(*task_key_dtor)(void*);
_Bool darling_task_key_set(unsigned int key, void* value, task_key_dtor dtor);
void* darling_task_key_get(unsigned int key);

#endif

