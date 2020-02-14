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

#ifndef DEBUG_H
#define	DEBUG_H

extern int printk(const char *fmt, ...);

extern bool debug_output;

// \0015 is equivalent to KERN_NOTICE
#ifdef linux_current
#	define debug_msg(fmt, ...) if (debug_output) printk("\0015Darling Mach: <%d> " fmt, linux_current->pid, ##__VA_ARGS__)
#else
#	define debug_msg(fmt, ...) if (debug_output) printk("\0015Darling Mach: <%d> " fmt, current->pid, ##__VA_ARGS__)
#endif



#endif	/* DEBUG_H */

