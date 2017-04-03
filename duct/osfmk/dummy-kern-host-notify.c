/*
 * Copyright (c) 2003-2009 Apple Inc. All rights reserved.
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
#include <mach/mach_host.h>

#include <kern/kern_types.h>
#include <kern/ipc_kobject.h>
#include <kern/host_notify.h>

#include <kern/queue.h>

#include "mach/host_notify_reply.h"

decl_lck_mtx_data(,host_notify_lock)

lck_mtx_ext_t           host_notify_lock_ext;
lck_grp_t               host_notify_lock_grp;
lck_attr_t              host_notify_lock_attr;
static lck_grp_attr_t   host_notify_lock_grp_attr;
static zone_t           host_notify_zone;

static queue_head_t     host_notify_queue[HOST_NOTIFY_TYPE_MAX+1];

static mach_msg_id_t    host_notify_replyid[HOST_NOTIFY_TYPE_MAX+1] =
                                { HOST_CALENDAR_CHANGED_REPLYID };

struct host_notify_entry {
    queue_chain_t       entries;
    ipc_port_t          port;
};

typedef struct host_notify_entry    *host_notify_t;

void
host_notify_init(void)
{
        kprintf("not implemented: host_notify_init()\n");
}

kern_return_t
host_request_notification(
    host_t                  host,
    host_flavor_t           notify_type,
    ipc_port_t              port)
{
        kprintf("not implemented: host_request_notification()\n");
        return 0;
}

void
host_notify_port_destroy(
    ipc_port_t          port)
{
        kprintf("not implemented: host_notify_port_destroy()\n");
}

// static void
// host_notify_all(
//     host_flavor_t       notify_type,
//     mach_msg_header_t   *msg,
//     mach_msg_size_t     msg_size)
// {
//     ;
// }

void
host_notify_calendar_change(void)
{
        kprintf("not implemented: host_notify_calendar_change()\n");
}
