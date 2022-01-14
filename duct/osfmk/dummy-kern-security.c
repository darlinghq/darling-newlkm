/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
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
/*-
 * Copyright (c) 2005-2007 SPARTA, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <mach/message.h>
#include <kern/kern_types.h>
#include <kern/ipc_kobject.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_right.h>
#include <ipc/ipc_labelh.h>
#include <kern/task.h>
#include <security/mac_mach_internal.h>
#include <mach/security.h>

#if CONFIG_MACF_MACH
kern_return_t
mach_get_task_label(
    ipc_space_t  space,
    mach_port_name_t *outlabel)
{
        kprintf("not implemented: mach_get_task_label()\n");
        return 0;
}
#else
kern_return_t
mach_get_task_label(
    ipc_space_t  space __unused,
    mach_port_name_t *outlabel __unused)
{
        kprintf("not implemented: mach_get_task_label()\n");
        return 0;
}
#endif

#if CONFIG_MACF_MACH
kern_return_t
mach_get_task_label_text(
    ipc_space_t space,
    labelstr_t  policies,
    labelstr_t  outl)
{
        kprintf("not implemented: mach_get_task_label_text()\n");
        return 0;
}
#else
kern_return_t
mach_get_task_label_text(
    ipc_space_t space __unused,
    labelstr_t  policies __unused,
    labelstr_t  outl __unused)
{
        kprintf("not implemented: mach_get_task_label_text()\n");
        return 0;
}
#endif

#if CONFIG_MACF_MACH
int
mac_task_check_service(
    task_t       self,
    task_t       obj,
    const char * perm)
{
        kprintf("not implemented: mac_task_check_service()\n");
        return 0;
}
#else
int
mac_task_check_service(
    task_t       self __unused,
    task_t       obj __unused,
    const char * perm __unused)
{
        kprintf("not implemented: mac_task_check_service()\n");
        return 0;
}
#endif

#if CONFIG_MACF_MACH
kern_return_t
mac_check_service(
    __unused ipc_space_t space,
    labelstr_t  subj,
    labelstr_t  obj,
    labelstr_t  serv,
    labelstr_t  perm)
{
        kprintf("not implemented: mac_check_service()\n");
        return 0;
}
#else
kern_return_t
mac_check_service(
    __unused ipc_space_t space,
    __unused labelstr_t  subj,
    __unused labelstr_t  obj,
    __unused labelstr_t  serv,
    __unused labelstr_t  perm)
{
        kprintf("not implemented: mac_check_service()\n");
        return 0;
}
#endif

#if CONFIG_MACF_MACH
kern_return_t
mac_port_check_service_obj(
    ipc_space_t      space,
    labelstr_t       subj,
    mach_port_name_t obj,
    labelstr_t       serv,
    labelstr_t       perm)
{
        kprintf("not implemented: mac_port_check_service_obj()\n");
        return 0;
}
#else
kern_return_t
mac_port_check_service_obj(
    __unused ipc_space_t      space,
    __unused labelstr_t       subj,
    __unused mach_port_name_t obj,
    __unused labelstr_t       serv,
    __unused labelstr_t       perm)
{
        kprintf("not implemented: mac_port_check_service_obj()\n");
        return 0;
}
#endif

#if CONFIG_MACF_MACH
kern_return_t
mac_port_check_access(
    ipc_space_t      space,
    mach_port_name_t sub,
    mach_port_name_t obj,
    labelstr_t       serv,
    labelstr_t       perm)
{
        kprintf("not implemented: mac_port_check_access()\n");
        return 0;
}
#else
kern_return_t
mac_port_check_access(
    __unused ipc_space_t      space,
    __unused mach_port_name_t sub,
    __unused mach_port_name_t obj,
    __unused labelstr_t       serv,
    __unused labelstr_t       perm)
{
        kprintf("not implemented: mac_port_check_access()\n");
        return 0;
}
#endif

#if CONFIG_MACF_MACH
kern_return_t
mac_request_label(
    ipc_space_t      space,
    mach_port_name_t sub,
    mach_port_name_t obj,
    labelstr_t       serv,
    mach_port_name_t *outlabel)
{
        kprintf("not implemented: mac_request_label()\n");
        return 0;
}
#else /* !MAC_MACH */

kern_return_t
mac_request_label(
    __unused ipc_space_t      space,
    __unused mach_port_name_t sub,
    __unused mach_port_name_t obj,
    __unused labelstr_t       serv,
    __unused mach_port_name_t *outlabel)
{
        kprintf("not implemented: mac_request_label()\n");
        return 0;
}

#endif /* MAC_MACH */
