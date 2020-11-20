/*
 * Copyright (c) 2000-2003 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef _SYS_PTHREAD_INTERNAL_H_
#define _SYS_PTHREAD_INTERNAL_H_

#include <linux/list.h>
#include <linux/semaphore.h>
#include <stdint.h>

struct ksyn_waitq_element {
	struct list_head kwe_list;	/* link to other list members */
	void *          kwe_kwqqueue;            	/* queue blocked on */
	uint32_t	kwe_flags;			/* flags */
	uint32_t        kwe_lockseq;			/* the sequence of the entry */
	uint32_t	kwe_count;			/* upper bound on number of matches still pending */
	uint32_t 	kwe_psynchretval;		/* thread retval */
	void		*kwe_uth;			/* uthread */
#if defined (__DARLING__)
	struct linux_semaphore linux_sem;
#endif
};
typedef struct ksyn_waitq_element * ksyn_waitq_element_t;

/* kew_flags defns */
#define KWE_THREAD_INWAIT       1
#define KWE_THREAD_PREPOST      2
#define KWE_THREAD_BROADCAST    4

void
pth_global_hashinit(void);

void
pth_global_hashexit(void);

#endif /* _SYS_PTHREAD_INTERNAL_H_ */

