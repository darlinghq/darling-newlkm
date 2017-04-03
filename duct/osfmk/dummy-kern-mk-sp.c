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
 *
 */

/* The routines in this module are all obsolete */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <mach/boolean.h>
#include <mach/thread_switch.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>
#include <kern/ipc_kobject.h>
#include <kern/processor.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/spl.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <mach/policy.h>

#include <kern/syscall_subr.h>
#include <mach/mach_host_server.h>
#include <mach/mach_syscalls.h>

#include <kern/misc_protos.h>
#include <kern/spl.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/assert.h>
#include <kern/thread.h>
#include <mach/mach_host_server.h>
#include <mach/thread_act_server.h>
#include <mach/host_priv_server.h>

/*
 *  thread_policy_common:
 *
 *  Set scheduling policy & priority for thread.
 */
// static kern_return_t
// thread_policy_common(
//     thread_t        thread,
//     integer_t       policy,
//     integer_t       priority)
// {
//     return 0;
// }

/*
 *  thread_set_policy
 *
 *  Set scheduling policy and parameters, both base and limit, for
 *  the given thread. Policy can be any policy implemented by the
 *  processor set, whether enabled or not.
 */
kern_return_t
thread_set_policy(
    thread_t                thread,
    processor_set_t         pset,
    policy_t                policy,
    policy_base_t           base,
    mach_msg_type_number_t  base_count,
    policy_limit_t          limit,
    mach_msg_type_number_t  limit_count)
{
        kprintf("not implemented: thread_set_policy()\n");
        return 0;
}


/*
 *  thread_policy
 *
 *  Set scheduling policy and parameters, both base and limit, for
 *  the given thread. Policy must be a policy which is enabled for the
 *  processor set. Change contained threads if requested.
 */
kern_return_t
thread_policy(
    thread_t                thread,
    policy_t                policy,
    policy_base_t           base,
    mach_msg_type_number_t  count,
    boolean_t               set_limit)
{
        kprintf("not implemented: thread_policy()\n");
        return 0;
}
