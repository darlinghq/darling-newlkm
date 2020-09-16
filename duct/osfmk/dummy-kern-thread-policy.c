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

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <mach/mach_types.h>
#include <mach/thread_act_server.h>

#include <kern/kern_types.h>
#include <kern/processor.h>
#include <kern/thread.h>
#include <kern/affinity.h>

// static void
// thread_recompute_priority(
//     thread_t        thread);

#if CONFIG_EMBEDDED
// static void
// thread_throttle(
//     thread_t        thread,
//     integer_t       task_priority);

extern int mach_do_background_thread(thread_t thread, int prio);
#endif


kern_return_t
thread_policy_set(
    thread_t                thread,
    thread_policy_flavor_t  flavor,
    thread_policy_t         policy_info,
    mach_msg_type_number_t  count)
{
        kprintf("not implemented: thread_policy_set()\n");
        return 0;
}

kern_return_t
thread_policy_set_internal(
    thread_t                thread,
    thread_policy_flavor_t  flavor,
    thread_policy_t         policy_info,
    mach_msg_type_number_t  count)
{
        kprintf("not implemented: thread_policy_set_internal()\n");
        return 0;
}

// static void
// thread_recompute_priority(
//     thread_t        thread)
// {
//     ;
// }

#if CONFIG_EMBEDDED
// static void
// thread_throttle(
//     thread_t        thread,
//     integer_t       task_priority)
// {
//     ;
// }
#endif

void
thread_task_priority(
    thread_t        thread,
    integer_t       priority,
    integer_t       max_priority)
{
        kprintf("not implemented: thread_task_priority()\n");
}

void
thread_policy_reset(
    thread_t        thread)
{
        kprintf("not implemented: thread_policy_reset()\n");
}

kern_return_t
thread_policy_get(
    thread_t                thread,
    thread_policy_flavor_t  flavor,
    thread_policy_t         policy_info,
    mach_msg_type_number_t  *count,
    boolean_t               *get_default)
{
        kprintf("not implemented: thread_policy_get()\n");
        return 0;
}

boolean_t thread_recompute_user_promotion_locked(thread_t thread) {
	kprintf("not implemented: thread_recompute_user_promotion_locked()\n");
	return FALSE;
};

boolean_t thread_recompute_kernel_promotion_locked(thread_t thread) {
	kprintf("not implemented: thread_recompute_kernel_promotion_locked()\n");
	return FALSE;
};

thread_qos_t thread_get_requested_qos(thread_t thread, int* relpri) {
	kprintf("not implemented: thread_get_requested_qos()\n");
	return THREAD_QOS_UNSPECIFIED;
};
