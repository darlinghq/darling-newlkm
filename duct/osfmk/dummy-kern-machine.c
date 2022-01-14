/*
 * Copyright (c) 2000-2009 Apple Inc. All rights reserved.
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
 */
/*
 *  File:   kern/machine.c
 *  Author: Avadis Tevanian, Jr.
 *  Date:   1987
 *
 *  Support for machine independent machine abstraction.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <string.h>

#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/machine.h>
#include <mach/host_info.h>
#include <mach/host_reboot.h>
#include <mach/host_priv_server.h>
#include <mach/processor_server.h>

#include <kern/kern_types.h>
#include <kern/counters.h>
#include <kern/cpu_data.h>
#include <kern/ipc_host.h>
#include <kern/host.h>
#include <kern/lock.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/processor.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/task.h>
#include <kern/thread.h>

#include <machine/commpage.h>

#if HIBERNATION
// #include <IOKit/IOHibernatePrivate.h>
#endif
#include <IOKit/IOPlatformExpert.h>

/*
 *  Exported variables:
 */

struct machine_info machine_info;

/* Forwards */
void            processor_doshutdown(
                    processor_t         processor);

/*
 *  processor_up:
 *
 *  Flag processor as up and running, and available
 *  for scheduling.
 */
void
processor_up(
    processor_t         processor)
{
        kprintf("not implemented: processor_up()\n");
}

kern_return_t
host_reboot(
    host_priv_t     host_priv,
    int             options)
{
        kprintf("not implemented: host_reboot()\n");
        return 0;
}

kern_return_t
processor_assign(
    __unused processor_t        processor,
    __unused processor_set_t    new_pset,
    __unused boolean_t      wait)
{
        kprintf("not implemented: processor_assign()\n");
        return 0;
}

kern_return_t
processor_shutdown(
    processor_t         processor)
{
        kprintf("not implemented: processor_shutdown()\n");
        return 0;
}

/*
 * Called with interrupts disabled.
 */
void
processor_doshutdown(
    processor_t         processor)
{
        kprintf("not implemented: processor_doshutdown()\n");
}

/*
 *Complete the shutdown and place the processor offline.
 *
 *  Called at splsched in the shutdown context.
 */
void
processor_offline(
    processor_t         processor)
{
        kprintf("not implemented: processor_offline()\n");
}

kern_return_t
host_get_boot_info(
        host_priv_t         host_priv,
        kernel_boot_info_t  boot_info)
{
        kprintf("not implemented: host_get_boot_info()\n");
        return 0;
}
