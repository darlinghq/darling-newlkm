/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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

/*
 * @OSF_COPYRIGHT@
 */

/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 *  File:   kern/lock.c
 *  Author: Avadis Tevanian, Jr., Michael Wayne Young
 *  Date:   1985
 *
 *  Locking primitives implementation
 */

/*
 * ARM locks.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <mach_ldebug.h>

#include <kern/lock.h>
#include <kern/locks.h>
#include <kern/kalloc.h>
#include <kern/misc_protos.h>
#include <kern/thread.h>
#include <kern/processor.h>
#include <kern/cpu_data.h>
#include <kern/cpu_number.h>
#include <kern/sched_prim.h>
#include <kern/xpr.h>
#include <kern/debug.h>
#include <string.h>

#ifdef __arm__
#include <arm/machine_routines.h>
#include <arm/mp.h>
#include <arm/misc_protos.h>
#else
#include <i386/machine_routines.h>
#include <i386/mp.h>
#include <i386/misc_protos.h>
#endif
#include <machine/machine_cpu.h>

#include <sys/kdebug.h>
#include <mach/branch_predicates.h>

/*
 * This file works, don't mess with it.
 */

unsigned int lock_wait_time[2] = { (unsigned int) -1, 0 };

#define lck_mtx_data    lck_mtx_sw.lck_mtxd.lck_mtxd_data
#define lck_mtx_waiters lck_mtx_sw.lck_mtxd.lck_mtxd_waiters
#define lck_mtx_pri     lck_mtx_sw.lck_mtxd.lck_mtxd_pri

#warning Implement locks?

uint32_t LcksOpts;

void lck_rw_ilk_lock(lck_rw_t * lck)
{
        kprintf("not implemented: lck_rw_ilk_lock()\n");
}

void lck_rw_ilk_unlock(lck_rw_t * lck)
{
        kprintf("not implemented: lck_rw_ilk_unlock()\n");
}

// static void lck_mtx_ext_init(lck_mtx_ext_t * lck, lck_grp_t * grp,
//                              lck_attr_t * attr)
// {
//     ;
// }

lck_mtx_t *lck_mtx_alloc_init(lck_grp_t * grp, lck_attr_t * attr)
{
        kprintf("not implemented: lck_mtx_alloc_init()\n");
        return 0;
}

void lck_mtx_free(lck_mtx_t * lck, lck_grp_t * grp)
{
        // kprintf("not implemented: lck_mtx_free()\n");
}

void lck_mtx_lock_spin(lck_mtx_t* lck)
{
}

void lck_mtx_lock_spin_always(lck_mtx_t* lck) {
	kprintf("not implemented: lck_mtx_lock_spin_always()\n");
};

void lck_mtx_destroy(lck_mtx_t * lck, lck_grp_t * grp)
{
        kprintf("not implemented: lck_mtx_destroy()\n");
}

// void lck_mtx_init_ext(lck_mtx_t * lck, lck_mtx_ext_t * lck_ext, lck_grp_t * grp,
//                       lck_attr_t * attr)
// {
//         kprintf("not implemented: lck_mtx_init_ext()\n");
// }

void lck_rw_destroy(lck_rw_t * lck, lck_grp_t * grp)
{
        kprintf("not implemented: lck_rw_destroy()\n");
}

void lck_rw_free(lck_rw_t * lck, lck_grp_t * grp)
{
        // kprintf("not implemented: lck_rw_free()\n");
}

void lck_rw_init(lck_rw_t * lck, lck_grp_t * grp, lck_attr_t * attr)
{
        // kprintf("not implemented: lck_rw_init()\n");
}

void lck_spin_destroy(lck_spin_t * lck, lck_grp_t * grp)
{
        // kprintf("not implemented: lck_spin_destroy()\n");
}

void lck_spin_unlock(lck_spin_t* lck)
{
}

// void lck_spin_init(lck_spin_t * lck, lck_grp_t * grp,
//                    __unused lck_attr_t * attr)
// {
//         // kprintf("not implemented: lck_spin_init()\n");
// }

void lck_rw_assert(lck_rw_t * lck, unsigned int type)
{
        // kprintf("not implemented: lck_rw_assert()\n");
}

void lck_rw_lock(lck_rw_t * lck, lck_rw_type_t lck_rw_type)
{
        // kprintf("not implemented: lck_rw_lock()\n");
}

void lck_spin_free(lck_spin_t * lck, lck_grp_t * grp)
{
        // kprintf("not implemented: lck_spin_free()\n");
}

boolean_t lck_rw_try_lock(lck_rw_t * lck, lck_rw_type_t lck_rw_type)
{
        // kprintf("not implemented: lck_rw_try_lock()\n");
        return 0;
}

lck_spin_t *lck_spin_alloc_init(lck_grp_t * grp, lck_attr_t * attr)
{
        // kprintf("not implemented: lck_spin_alloc_init()\n");
        return 0;
}

extern void arm_usimple_lock_init(usimple_lock_t, unsigned short);

void lck_rw_unlock_shared(lck_rw_t * lck)
{
        // kprintf("not implemented: lck_rw_unlock_shared()\n");
}

void lck_rw_unlock_exclusive(lck_rw_t * lck)
{
        kprintf("not implemented: lck_rw_unlock_exclusive()\n");
}

void lck_rw_lock_shared_gen(lck_rw_t * lck)
{
        kprintf("not implemented: lck_rw_lock_shared_gen()\n");
}

void lck_mtx_lock_mark_destroyed(lck_mtx_t * mutex)
{
        kprintf("not implemented: lck_mtx_lock_mark_destroyed()\n");
}

boolean_t lck_rw_lock_shared_to_exclusive_gen(lck_rw_t * lck)
{
        kprintf("not implemented: lck_rw_lock_shared_to_exclusive_gen()\n");
        return 0;
}

lck_rw_type_t
lck_rw_done(
        lck_rw_t        *lck)
{
        kprintf("not implemented: lck_rw_done()\n");
        return 0;
}

void
lck_rw_lock_exclusive(
        lck_rw_t        *lck)
{
        kprintf("not implemented: lck_rw_lock_exclusive()\n");
}

void
lck_rw_lock_shared(
        lck_rw_t        *lck)
{
        kprintf("not implemented: lck_rw_lock_shared()\n");
}
