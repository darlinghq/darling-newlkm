/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
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

#include <sys/errno.h>

#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <mach/memory_object_control.h>
#include <mach/memory_object_types.h>
#include <mach/port.h>
#include <mach/policy.h>
#include <mach/upl.h>
#include <kern/kern_types.h>
#include <kern/ipc_kobject.h>
#include <kern/host.h>
#include <kern/thread.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>
#include <device/device_port.h>
#include <vm/memory_object.h>
#include <vm/vm_pageout.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>


/* Device VM COMPONENT INTERFACES */


/*
 * Device PAGER
 */


/* until component support available */



/* until component support available */
const struct memory_object_pager_ops device_pager_ops = {
    device_pager_reference,
    device_pager_deallocate,
    device_pager_init,
    device_pager_terminate,
    device_pager_data_request,
    device_pager_data_return,
    device_pager_data_initialize,
    device_pager_data_unlock,
    device_pager_synchronize,
    device_pager_map,
    device_pager_last_unmap,
    NULL, /* data_reclaim */
    "device pager"
};

typedef uintptr_t device_port_t;

/*
 * The start of "struct device_pager" MUST match a "struct memory_object".
 */
typedef struct device_pager {
    struct ipc_object_header    pager_header;   /* fake ip_kotype() */
    memory_object_pager_ops_t pager_ops; /* == &device_pager_ops    */
    unsigned int    ref_count;  /* reference count      */
    memory_object_control_t control_handle; /* mem object's cntrl handle */
    device_port_t   device_handle;  /* device_handle */
    vm_size_t   size;
    int     flags;
} *device_pager_t;

#define pager_ikot pager_header.io_bits


device_pager_t
device_pager_lookup(        /* forward */
    memory_object_t);

device_pager_t
device_object_create(void); /* forward */

zone_t  device_pager_zone;


#define DEVICE_PAGER_NULL   ((device_pager_t) 0)


#define MAX_DNODE       10000

/*
 *
 */
void
device_pager_bootstrap(void)
{
        kprintf("not implemented: device_pager_bootstrap()");
}

/*
 *
 */
memory_object_t
device_pager_setup(
    __unused memory_object_t device,
    uintptr_t       device_handle,
    vm_size_t   size,
    int     flags)
{
        kprintf("not implemented: device_pager_setup()");
        return 0;
}

/*
 *
 */
kern_return_t
device_pager_populate_object(
    memory_object_t     device,
    memory_object_offset_t  offset,
    ppnum_t         page_num,
    vm_size_t       size)
{
        kprintf("not implemented: device_pager_populate_object()");
        return 0;
}

/*
 *
 */
device_pager_t
device_pager_lookup(
    memory_object_t name)
{
        kprintf("not implemented: device_pager_lookup()");
        return 0;
}

/*
 *
 */
kern_return_t
device_pager_init(
    memory_object_t mem_obj,
    memory_object_control_t control,
    __unused memory_object_cluster_size_t pg_size)
{
        kprintf("not implemented: device_pager_init()");
        return 0;
}

/*
 *
 */
/*ARGSUSED6*/
kern_return_t
device_pager_data_return(
    memory_object_t         mem_obj,
    memory_object_offset_t      offset,
    memory_object_cluster_size_t            data_cnt,
    __unused memory_object_offset_t *resid_offset,
    __unused int            *io_error,
    __unused boolean_t      dirty,
    __unused boolean_t      kernel_copy,
    __unused int            upl_flags)
{
        kprintf("not implemented: device_pager_data_return()");
        return 0;
}

/*
 *
 */
kern_return_t
device_pager_data_request(
    memory_object_t     mem_obj,
    memory_object_offset_t  offset,
    memory_object_cluster_size_t        length,
    __unused vm_prot_t  protection_required,
        __unused memory_object_fault_info_t     fault_info)
{
        kprintf("not implemented: device_pager_data_request()");
        return 0;
}

/*
 *
 */
void
device_pager_reference(
    memory_object_t     mem_obj)
{
        kprintf("not implemented: device_pager_reference()");
}

/*
 *
 */
void
device_pager_deallocate(
    memory_object_t     mem_obj)
{
        kprintf("not implemented: device_pager_deallocate()");
}

kern_return_t
device_pager_data_initialize(
        __unused memory_object_t        mem_obj,
        __unused memory_object_offset_t offset,
        __unused memory_object_cluster_size_t       data_cnt)
{
        kprintf("not implemented: device_pager_data_initialize()");
        return 0;
}

kern_return_t
device_pager_data_unlock(
    __unused memory_object_t        mem_obj,
    __unused memory_object_offset_t offset,
    __unused memory_object_size_t       size,
    __unused vm_prot_t      desired_access)
{
        kprintf("not implemented: device_pager_data_unlock()");
        return 0;
}

kern_return_t
device_pager_terminate(
    __unused memory_object_t    mem_obj)
{
        kprintf("not implemented: device_pager_terminate()");
        return 0;
}



/*
 *
 */
kern_return_t
device_pager_synchronize(
    memory_object_t     mem_obj,
    memory_object_offset_t  offset,
    memory_object_size_t        length,
    __unused vm_sync_t      sync_flags)
{
        kprintf("not implemented: device_pager_synchronize()");
        return 0;
}

/*
 *
 */
kern_return_t
device_pager_map(
    __unused memory_object_t    mem_obj,
    __unused vm_prot_t      prot)
{
        kprintf("not implemented: device_pager_map()");
        return 0;
}

kern_return_t
device_pager_last_unmap(
    __unused memory_object_t    mem_obj)
{
        kprintf("not implemented: device_pager_last_unmap()");
        return 0;
}



/*
 *
 */
device_pager_t
device_object_create(void)
{
        kprintf("not implemented: device_object_create()");
        return 0;
}
