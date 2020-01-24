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

#ifndef _CONTINUATION_H
#define _CONTINUATION_H

// Yes, we brought setjmp/longjmp to the kernel space

#ifdef CONFIG_X86_64
struct cont_jmpbuf
{
	unsigned long __rip;
	unsigned long __rsp;
	unsigned long __rbp;
	unsigned long __rbx;
	unsigned long __r12;
	unsigned long __r13;
	unsigned long __r14;
	unsigned long __r15;
};
#endif

void cont_discard(struct cont_jmpbuf* buf);

// In assembly
int cont_setjmp(struct cont_jmpbuf* buf);
void cont_longjmp(struct cont_jmpbuf* buf, int v);

#define XNU_CONTINUATION_ENABLED(call) ({ \
	thread_t myself = darling_thread_get_current(); \
	int rv; \
	int v = cont_setjmp((struct cont_jmpbuf*)myself->cont_jmpbuf); \
	if (v == 0) { \
		rv = call; \
		/* No continuation used */ \
	} else { \
		rv = (v == 1) ? 0 : v; \
	} \
	cont_discard((struct cont_jmpbuf*) myself->cont_jmpbuf); \
	rv; \
})

#endif

