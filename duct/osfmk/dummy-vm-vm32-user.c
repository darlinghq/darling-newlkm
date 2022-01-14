/*
 * Copyright (c) 2008 Apple Inc. All rights reserved.
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
 *  File:   vm/vm32_user.c
 *  Author: Avadis Tevanian, Jr., Michael Wayne Young
 *
 *  User-exported virtual memory functions.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <debug.h>

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>    /* to get vm_address_t */
#include <mach/memory_object.h>
#include <mach/std_types.h> /* to get pointer_t */
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <mach/vm_statistics.h>
#include <mach/mach_syscalls.h>

#include <mach/host_priv_server.h>
#include <mach/mach_vm_server.h>
#include <mach/vm32_map_server.h>

#include <kern/host.h>
#include <kern/kalloc.h>
#include <kern/task.h>
#include <kern/misc_protos.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/memory_object.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>

#if VM32_SUPPORT

/*
 * See vm_user.c for the real implementation of all of these functions.
 * We call through to the mach_ "wide" versions of the routines, and trust
 * that the VM system verifies the arguments and only returns address that
 * are appropriate for the task's address space size.
 *
 * New VM call implementations should not be added here, because they would
 * be available only to 32-bit userspace clients. Add them to vm_user.c
 * and the corresponding prototype to mach_vm.defs (subsystem 4800).
 */

kern_return_t
vm32_allocate(
    vm_map_t    map,
    vm32_offset_t   *addr,
    vm32_size_t size,
    int     flags)
{
        kprintf("not implemented: vm32_allocate()\n");
        return 0;
}

kern_return_t
vm32_deallocate(
    vm_map_t    map,
    vm32_offset_t       start,
    vm32_size_t     size)
{
        kprintf("not implemented: vm32_deallocate()\n");
        return 0;
}

kern_return_t
vm32_inherit(
    vm_map_t    map,
    vm32_offset_t       start,
    vm32_size_t     size,
    vm_inherit_t        new_inheritance)
{
        kprintf("not implemented: vm32_inherit()\n");
        return 0;
}

kern_return_t
vm32_protect(
    vm_map_t        map,
    vm32_offset_t       start,
    vm32_size_t     size,
    boolean_t       set_maximum,
    vm_prot_t       new_protection)
{
        kprintf("not implemented: vm32_protect()\n");
        return 0;
}

kern_return_t
vm32_machine_attribute(
    vm_map_t    map,
    vm32_address_t  addr,
    vm32_size_t size,
    vm_machine_attribute_t  attribute,
    vm_machine_attribute_val_t* value)      /* IN/OUT */
{
        kprintf("not implemented: vm32_machine_attribute()\n");
        return 0;
}

kern_return_t
vm32_read(
    vm_map_t        map,
    vm32_address_t      addr,
    vm32_size_t     size,
    pointer_t       *data,
    mach_msg_type_number_t  *data_size)
{
        kprintf("not implemented: vm32_read()\n");
        return 0;
}

kern_return_t
vm32_read_list(
    vm_map_t        map,
    vm32_read_entry_t   data_list,
    natural_t       count)
{
        kprintf("not implemented: vm32_read_list()\n");
        return 0;
}

kern_return_t
vm32_read_overwrite(
    vm_map_t    map,
    vm32_address_t  address,
    vm32_size_t size,
    vm32_address_t  data,
    vm32_size_t *data_size)
{
        kprintf("not implemented: vm32_read_overwrite()\n");
        return 0;
}

kern_return_t
vm32_write(
    vm_map_t            map,
    vm32_address_t          address,
    pointer_t           data,
    mach_msg_type_number_t  size)
{
        kprintf("not implemented: vm32_write()\n");
        return 0;
}

kern_return_t
vm32_copy(
    vm_map_t    map,
    vm32_address_t  source_address,
    vm32_size_t size,
    vm32_address_t  dest_address)
{
        kprintf("not implemented: vm32_copy()\n");
        return 0;
}

kern_return_t
vm32_map_64(
    vm_map_t        target_map,
    vm32_offset_t       *address,
    vm32_size_t     size,
    vm32_offset_t       mask,
    int         flags,
    ipc_port_t      port,
    vm_object_offset_t  offset,
    boolean_t       copy,
    vm_prot_t       cur_protection,
    vm_prot_t       max_protection,
    vm_inherit_t        inheritance)
{
        kprintf("not implemented: vm32_map_64()\n");
        return 0;
}

kern_return_t
vm32_map(
    vm_map_t        target_map,
    vm32_offset_t       *address,
    vm32_size_t     size,
    vm32_offset_t       mask,
    int         flags,
    ipc_port_t      port,
    vm32_offset_t       offset,
    boolean_t       copy,
    vm_prot_t       cur_protection,
    vm_prot_t       max_protection,
    vm_inherit_t        inheritance)
{
        kprintf("not implemented: vm32_map()\n");
        return 0;
}

kern_return_t
vm32_remap(
    vm_map_t        target_map,
    vm32_offset_t       *address,
    vm32_size_t     size,
    vm32_offset_t       mask,
    boolean_t       anywhere,
    vm_map_t        src_map,
    vm32_offset_t       memory_address,
    boolean_t       copy,
    vm_prot_t       *cur_protection,
    vm_prot_t       *max_protection,
    vm_inherit_t        inheritance)
{
        kprintf("not implemented: vm32_remap()\n");
        return 0;
}

kern_return_t
vm32_msync(
    vm_map_t    map,
    vm32_address_t  address,
    vm32_size_t size,
    vm_sync_t   sync_flags)
{
        kprintf("not implemented: vm32_msync()\n");
        return 0;
}

kern_return_t
vm32_behavior_set(
    vm_map_t        map,
    vm32_offset_t       start,
    vm32_size_t     size,
    vm_behavior_t       new_behavior)
{
        kprintf("not implemented: vm32_behavior_set()\n");
        return 0;
}

kern_return_t
vm32_region_64(
    vm_map_t         map,
    vm32_offset_t           *address,       /* IN/OUT */
    vm32_size_t     *size,          /* OUT */
    vm_region_flavor_t   flavor,        /* IN */
    vm_region_info_t     info,          /* OUT */
    mach_msg_type_number_t  *count,         /* IN/OUT */
    mach_port_t     *object_name)       /* OUT */
{
        kprintf("not implemented: vm32_region_64()\n");
        return 0;
}

kern_return_t
vm32_region(
    vm_map_t            map,
    vm32_address_t              *address,   /* IN/OUT */
    vm32_size_t         *size,      /* OUT */
    vm_region_flavor_t      flavor, /* IN */
    vm_region_info_t        info,       /* OUT */
    mach_msg_type_number_t  *count, /* IN/OUT */
    mach_port_t         *object_name)   /* OUT */
{
        kprintf("not implemented: vm32_region()\n");
        return 0;
}

kern_return_t
vm32_region_recurse_64(
    vm_map_t            map,
    vm32_address_t          *address,
    vm32_size_t         *size,
    uint32_t            *depth,
    vm_region_recurse_info_64_t info,
    mach_msg_type_number_t  *infoCnt)
{
        kprintf("not implemented: vm32_region_recurse_64()\n");
        return 0;
}

kern_return_t
vm32_region_recurse(
    vm_map_t            map,
    vm32_offset_t           *address,   /* IN/OUT */
    vm32_size_t         *size,      /* OUT */
    natural_t           *depth, /* IN/OUT */
    vm_region_recurse_info_t    info32, /* IN/OUT */
    mach_msg_type_number_t  *infoCnt)   /* IN/OUT */
{
        kprintf("not implemented: vm32_region_recurse()\n");
        return 0;
}

kern_return_t
vm32_purgable_control(
    vm_map_t        map,
    vm32_offset_t       address,
    vm_purgable_t       control,
    int         *state)
{
        kprintf("not implemented: vm32_purgable_control()\n");
        return 0;
}

kern_return_t
vm32_map_page_query(
    vm_map_t        map,
    vm32_offset_t       offset,
    int         *disposition,
    int         *ref_count)
{
        kprintf("not implemented: vm32_map_page_query()\n");
        return 0;
}

kern_return_t
vm32_make_memory_entry_64(
    vm_map_t        target_map,
    memory_object_size_t    *size,
    memory_object_offset_t offset,
    vm_prot_t       permission,
    ipc_port_t      *object_handle,
    ipc_port_t      parent_handle)
{
        kprintf("not implemented: vm32_make_memory_entry_64()\n");
        return 0;
}

kern_return_t
vm32_make_memory_entry(
    vm_map_t        target_map,
    vm32_size_t     *size,
    vm32_offset_t       offset,
    vm_prot_t       permission,
    ipc_port_t      *object_handle,
    ipc_port_t      parent_entry)
{
        kprintf("not implemented: vm32_make_memory_entry()\n");
        return 0;
}

kern_return_t
vm32__task_wire(
    vm_map_t    map,
    boolean_t   must_wire)
{
        kprintf("not implemented: vm32__task_wire()\n");
        return 0;
}

#endif /* VM32_SUPPORT */
