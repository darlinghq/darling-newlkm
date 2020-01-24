/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2020 Lubos Dolezel
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
#include <duct/duct_pre_xnu.h>
#include <osfmk/kern/thread.h>
#include <duct/duct_kern_debug.h>
#include <duct/duct_post_xnu.h>

#include "continuation.h"
#include "task_registry.h"

void cont_discard(struct cont_jmpbuf* buf)
{
	buf->__rip = 0;
}

void thread_syscall_return(kern_return_t ret)
{
	thread_t myself = darling_thread_get_current();
	if (((struct cont_jmpbuf*) myself->cont_jmpbuf)->__rip == 0)
		duct_panic("thread_syscall_return invoked, but XNU_CONTINUATION_ENABLED() was not used!");
	else
		cont_longjmp((struct cont_jmpbuf*) myself->cont_jmpbuf, ret);
}
