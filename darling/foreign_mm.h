/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2018 Lubos Dolezel
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

#ifndef _FOREIGNMM_H
#define _FOREIGNMM_H
#include <linux/mm.h>

void foreignmm_init(void);
void foreignmm_exit(void);

typedef void (*foreignmm_cb_t)(void*);

// Synchonously execute a callback function on a kthread where current->mm will point to the given mm.
// Remember to use mmget() and mmput() around foreignmm_execute() calls!
void foreignmm_execute(struct mm_struct* mm, foreignmm_cb_t cb, void* param);

#endif

