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
#include <kern/timer_call.h>
#include <kern/thread_call.h>
#include <duct/duct_post_xnu.h>
#include "duct_kern_printf.h"

// NOTE: I couldn't find and real reasons why timer_call and thread_call couldn't have the same implementation,
// hence the former will be implemented using the latter.


boolean_t timer_call_enter_with_leeway(timer_call_t call, timer_call_param_t param1, uint64_t deadline, uint64_t leeway, uint32_t flags, boolean_t ratelimited)
{
	printf("NOT IMPLEMENTED: timer_call_enter_with_leeway\n");
	return FALSE;
}

void timer_call_setup(timer_call_t call, timer_call_func_t func, timer_call_param_t param0)
{
	printf("NOT IMPLEMENTED: timer_call_setup\n");
}

boolean_t timer_call_cancel(timer_call_t call)
{
	printf("NOT IMPLEMENTED: timer_call_cancel\n");
	return FALSE;
}



