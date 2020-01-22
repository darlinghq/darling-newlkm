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
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
 *  processor.c: processor and processor_set manipulation routines.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <mach/boolean.h>
#include <mach/policy.h>
#include <mach/processor.h>
#include <mach/processor_info.h>
#include <mach/vm_param.h>
#include <kern/cpu_number.h>
#include <kern/host.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/processor.h>
#include <kern/sched.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/ipc_host.h>
#include <kern/ipc_tt.h>
#include <ipc/ipc_port.h>
#include <kern/kalloc.h>

/*
 * Exported interface
 */
#include <mach/mach_host_server.h>
#include <mach/processor_set_server.h>

struct processor_set    pset0;
struct pset_node        pset_node0;
decl_simple_lock_data(static,pset_node_lock)

queue_head_t            tasks;
queue_head_t            terminated_tasks;   /* To be used ONLY for stackshot. */
int                     tasks_count;
queue_head_t            threads;
int                     threads_count;
decl_lck_mtx_data(,tasks_threads_lock)

processor_t             processor_list;
unsigned int            processor_count;
static processor_t      processor_list_tail;
decl_simple_lock_data(,processor_list_lock)

uint32_t                processor_avail_count;

processor_t     master_processor;
int             master_cpu = 0;
boolean_t       sched_stats_active = FALSE;

/* Forwards */
kern_return_t    processor_set_things(
                        processor_set_t pset,
            void **thing_list,
            mach_msg_type_number_t *count,
            int type);

void
processor_bootstrap(void)
{
        kprintf("not implemented: processor_bootstrap()\n");
}

/*
 *  Initialize the given processor for the cpu
 *  indicated by cpu_id, and assign to the
 *  specified processor set.
 */
void
processor_init(
    processor_t         processor,
    int                 cpu_id,
    processor_set_t     pset)
{
        kprintf("not implemented: processor_init()\n");
}

void
processor_meta_init(
    processor_t     processor,
    processor_t     primary)
{
        kprintf("not implemented: processor_meta_init()\n");
}

processor_set_t
processor_pset(
    processor_t processor)
{
        kprintf("not implemented: processor_pset()\n");
        return KERN_FAILURE;
}

pset_node_t
pset_node_root(void)
{
        kprintf("not implemented: pset_node_root()\n");
        return KERN_FAILURE;
}

processor_set_t
pset_create(
    pset_node_t         node)
{
        kprintf("not implemented: pset_create()\n");
        return KERN_FAILURE;
}

/*
 *  Initialize the given processor_set structure.
 */
void
pset_init(
    processor_set_t     pset,
    pset_node_t         node)
{
        kprintf("not implemented: pset_init()\n");
}

processor_t current_processor(void)
{
        kprintf("not implemented: current_processor()\n");
        return NULL;
}

kern_return_t
processor_info_count(
    processor_flavor_t      flavor,
    mach_msg_type_number_t  *count)
{
        kprintf("not implemented: processor_info_count()\n");
        return KERN_FAILURE;
}


kern_return_t
processor_info(
    register processor_t    processor,
    processor_flavor_t      flavor,
    host_t                  *host,
    processor_info_t        info,
    mach_msg_type_number_t  *count)
{
        kprintf("not implemented: processor_info()\n");
        return KERN_FAILURE;
}

kern_return_t
processor_start(
    processor_t         processor)
{
        kprintf("not implemented: processor_start()\n");
        return KERN_FAILURE;
}

kern_return_t
processor_exit(
    processor_t processor)
{
        kprintf("not implemented: processor_exit()\n");
        return KERN_FAILURE;
}

kern_return_t
processor_control(
    processor_t     processor,
    processor_info_t    info,
    mach_msg_type_number_t  count)
{
        kprintf("not implemented: processor_control()\n");
        return KERN_FAILURE;
}

kern_return_t
processor_set_create(
    __unused host_t     host,
    __unused processor_set_t    *new_set,
    __unused processor_set_t    *new_name)
{
        kprintf("not implemented: processor_set_create()\n");
        return KERN_FAILURE;
}

kern_return_t
processor_set_destroy(
    __unused processor_set_t    pset)
{
        kprintf("not implemented: processor_set_destroy()\n");
        return KERN_FAILURE;
}

kern_return_t
processor_get_assignment(
    processor_t processor,
    processor_set_t *pset)
{
        kprintf("not implemented: processor_get_assignment()\n");
        return KERN_FAILURE;
}

kern_return_t
processor_set_info(
    processor_set_t     pset,
    int         flavor,
    host_t          *host,
    processor_set_info_t    info,
    mach_msg_type_number_t  *count)
{
        kprintf("not implemented: processor_set_info()\n");
        return KERN_FAILURE;
}

/*
 *  processor_set_statistics
 *
 *  Returns scheduling statistics for a processor set.
 */
kern_return_t
processor_set_statistics(
    processor_set_t         pset,
    int                     flavor,
    processor_set_info_t    info,
    mach_msg_type_number_t  *count)
{
        kprintf("not implemented: processor_set_statistics()\n");
        return KERN_FAILURE;
}

/*
 *  processor_set_max_priority:
 *
 *  Specify max priority permitted on processor set.  This affects
 *  newly created and assigned threads.  Optionally change existing
 *  ones.
 */
kern_return_t
processor_set_max_priority(
    __unused processor_set_t    pset,
    __unused int            max_priority,
    __unused boolean_t      change_threads)
{
        kprintf("not implemented: processor_set_max_priority()\n");
        return KERN_FAILURE;
}

/*
 *  processor_set_policy_enable:
 *
 *  Allow indicated policy on processor set.
 */

kern_return_t
processor_set_policy_enable(
    __unused processor_set_t    pset,
    __unused int            policy)
{
        kprintf("not implemented: processor_set_policy_enable()\n");
        return KERN_FAILURE;
}

/*
 *  processor_set_policy_disable:
 *
 *  Forbid indicated policy on processor set.  Time sharing cannot
 *  be forbidden.
 */
kern_return_t
processor_set_policy_disable(
    __unused processor_set_t    pset,
    __unused int            policy,
    __unused boolean_t      change_threads)
{
        kprintf("not implemented: processor_set_policy_disable()\n");
        return KERN_FAILURE;
}

#define THING_TASK  0
#define THING_THREAD    1

/*
 *  processor_set_things:
 *
 *  Common internals for processor_set_{threads,tasks}
 */
kern_return_t    processor_set_things(
                        processor_set_t pset,
            void **thing_list,
            mach_msg_type_number_t *count,
            int type)
{
        kprintf("not implemented: processor_set_things()\n");
        return KERN_FAILURE;
}


/*
 *  processor_set_tasks:
 *
 *  List all tasks in the processor set.
 */
kern_return_t
processor_set_tasks(
    processor_set_t     pset,
    task_array_t        *task_list,
    mach_msg_type_number_t  *count)
{
        kprintf("not implemented: processor_set_tasks()\n");
        return KERN_FAILURE;
}

/*
 *  processor_set_threads:
 *
 *  List all threads in the processor set.
 */
#if defined(SECURE_KERNEL)
kern_return_t
processor_set_threads(
    __unused processor_set_t        pset,
    __unused thread_array_t     *thread_list,
    __unused mach_msg_type_number_t *count)
{
        kprintf("not implemented: processor_set_threads()\n");
        return KERN_FAILURE;
}
#elif defined(CONFIG_EMBEDDED)
kern_return_t
processor_set_threads(
    __unused processor_set_t        pset,
    __unused thread_array_t     *thread_list,
    __unused mach_msg_type_number_t *count)
{
        kprintf("not implemented: processor_set_threads()\n");
        return KERN_FAILURE;
}
#else
kern_return_t
processor_set_threads(
    processor_set_t     pset,
    thread_array_t      *thread_list,
    mach_msg_type_number_t  *count)
{
        kprintf("not implemented: processor_set_threads()\n");
        return KERN_FAILURE;
}
#endif

/*
 *  processor_set_policy_control
 *
 *  Controls the scheduling attributes governing the processor set.
 *  Allows control of enabled policies, and per-policy base and limit
 *  priorities.
 */
kern_return_t
processor_set_policy_control(
    __unused processor_set_t        pset,
    __unused int                flavor,
    __unused processor_set_info_t   policy_info,
    __unused mach_msg_type_number_t count,
    __unused boolean_t          change)
{
        kprintf("not implemented: processor_set_policy_control()\n");
        return KERN_FAILURE;
}

#undef pset_deallocate
void pset_deallocate(processor_set_t pset);
void
pset_deallocate(
__unused processor_set_t    pset)
{
        kprintf("not implemented: pset_deallocate()\n");
}

#undef pset_reference
void pset_reference(processor_set_t pset);
void
pset_reference(
__unused processor_set_t    pset)
{
        kprintf("not implemented: pset_reference()\n");
}
