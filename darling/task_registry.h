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

thread_t darling_thread_get(unsigned int tid);

void darling_task_register(task_t t);
void darling_task_deregister(task_t t);

void darling_task_fork_wait_for_child(void);
void darling_task_fork_child_done(void);
void darling_task_set_dyld_info(unsigned long all_img_location, unsigned long all_img_length);
void darling_task_get_dyld_info(unsigned int pid, unsigned long long* all_img_location, unsigned long long* all_img_length);
void darling_task_mark_start_suspended(void);
_Bool darling_task_marked_start_suspended(void);

struct evprocfd_ctx;
_Bool darling_task_notify_register(unsigned int pid, struct evprocfd_ctx* efd);
_Bool darling_task_notify_deregister(unsigned int pid, struct evprocfd_ctx* efd);

// NOTE_EXEC and NOTE_EXIT are detected internally, only use this to deliver other events
void darling_task_post_notification(unsigned int pid, unsigned int event, unsigned int extra);

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

