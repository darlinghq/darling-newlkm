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

void darling_task_init(void);

task_t darling_task_get_current(void);
thread_t darling_thread_get_current(void);

thread_t darling_thread_get(unsigned int tid);

void darling_task_register(task_t t);
void darling_task_deregister(task_t t); // argument unused

void darling_thread_register(thread_t t);
void darling_thread_deregister(thread_t t); // argument unused

#endif

