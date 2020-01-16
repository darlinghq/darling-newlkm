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
 *	File:	vm/vm_user.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 * 
 *	User-exported virtual memory functions.
 */

/*
 * There are three implementations of the "XXX_allocate" functionality in
 * the kernel: mach_vm_allocate (for any task on the platform), vm_allocate
 * (for a task with the same address space size, especially the current task),
 * and vm32_vm_allocate (for the specific case of a 32-bit task). vm_allocate
 * in the kernel should only be used on the kernel_task. vm32_vm_allocate only
 * makes sense on platforms where a user task can either be 32 or 64, or the kernel
 * task can be 32 or 64. mach_vm_allocate makes sense everywhere, and is preferred
 * for new code.
 *
 * The entrypoints into the kernel are more complex. All platforms support a
 * mach_vm_allocate-style API (subsystem 4800) which operates with the largest
 * size types for the platform. On platforms that only support U32/K32,
 * subsystem 4800 is all you need. On platforms that support both U32 and U64,
 * subsystem 3800 is used disambiguate the size of parameters, and they will
 * always be 32-bit and call into the vm32_vm_allocate APIs. On non-U32/K32 platforms,
 * the MIG glue should never call into vm_allocate directly, because the calling
 * task and kernel_task are unlikely to use the same size parameters
 *
 * New VM call implementations should be added here and to mach_vm.defs
 * (subsystem 4800), and use mach_vm_* "wide" types.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <debug.h>

#include <vm_cpm.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>	/* to get vm_address_t */
//#include <mach/memory_object.h>
#include <mach/std_types.h>	/* to get pointer_t */
#include <mach/upl.h>
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <mach/vm_statistics.h>
#include <mach/mach_syscalls.h>

#include <mach/host_priv_server.h>
#include <mach/mach_vm_server.h>
#include <mach/vm_map_server.h>

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

vm_size_t        upl_offset_to_pagelist = 0;

#if	VM_CPM
#include <vm/cpm.h>
#endif	/* VM_CPM */

ipc_port_t	dynamic_pager_control_port=NULL;

/*
 *	mach_vm_allocate allocates "zero fill" memory in the specfied
 *	map.
 */
// kern_return_t
// mach_vm_allocate(
//     vm_map_t        map,
//     mach_vm_offset_t    *addr,
//     mach_vm_size_t    size,
//     int            flags)
// {
//         kprintf("not implemented: mach_vm_allocate()\n");
//         return 0;
// }
/*
 *	vm_allocate 
 *	Legacy routine that allocates "zero fill" memory in the specfied
 *	map (which is limited to the same size as the kernel).
 */
kern_return_t
vm_allocate(
	vm_map_t	map,
	vm_offset_t	*addr,
	vm_size_t	size,
	int		flags)
{
        return mach_vm_allocate(map, (mach_vm_offset_t*) addr, size, flags);
}
/*
 *	mach_vm_deallocate -
 *	deallocates the specified range of addresses in the
 *	specified address map.
 */
// kern_return_t
// mach_vm_deallocate(
// 	vm_map_t		map,
// 	mach_vm_offset_t	start,
// 	mach_vm_size_t	size)
// {
//         kprintf("not implemented: mach_vm_deallocate()\n");
//         return 0;
// }
/*
 *	vm_deallocate -
 *	deallocates the specified range of addresses in the
 *	specified address map (limited to addresses the same
 *	size as the kernel).
 */
kern_return_t
vm_deallocate(
	register vm_map_t	map,
	vm_offset_t		start,
	vm_size_t		size)
{
	return mach_vm_deallocate(map, start, size);
}
/*
 *	mach_vm_inherit -
 *	Sets the inheritance of the specified range in the
 *	specified map.
 */
kern_return_t
mach_vm_inherit(
	vm_map_t		map,
	mach_vm_offset_t	start,
	mach_vm_size_t	size,
	vm_inherit_t		new_inheritance)
{
        kprintf("not implemented: mach_vm_inherit()\n");
        return 0;
}
/*
 *	vm_inherit -
 *	Sets the inheritance of the specified range in the
 *	specified map (range limited to addresses
 */
kern_return_t
vm_inherit(
	register vm_map_t	map,
	vm_offset_t		start,
	vm_size_t		size,
	vm_inherit_t		new_inheritance)
{
        kprintf("not implemented: vm_inherit()\n");
        return 0;
}
/*
 *	mach_vm_protect -
 *	Sets the protection of the specified range in the
 *	specified map.
 */

kern_return_t
mach_vm_protect(
	vm_map_t		map,
	mach_vm_offset_t	start,
	mach_vm_size_t	size,
	boolean_t		set_maximum,
	vm_prot_t		new_protection)
{
        kprintf("no-op: mach_vm_protect()\n");
        return 0;
}
/*
 *	vm_protect -
 *	Sets the protection of the specified range in the
 *	specified map. Addressability of the range limited
 *	to the same size as the kernel.
 */

kern_return_t
vm_protect(
	vm_map_t		map,
	vm_offset_t		start,
	vm_size_t		size,
	boolean_t		set_maximum,
	vm_prot_t		new_protection)
{
	// There is currently no feasible way of doing mprotect() on a remote process.
	// Hence we silently ignore this request and permit overwriting R/O memory pages
	// in vm_write() with FOLL_FORCE.
        kprintf("no-op: vm_protect()\n");
        return 0;
}
/*
 * mach_vm_machine_attributes -
 * Handle machine-specific attributes for a mapping, such
 * as cachability, migrability, etc.
 */
kern_return_t
mach_vm_machine_attribute(
	vm_map_t			map,
	mach_vm_address_t		addr,
	mach_vm_size_t		size,
	vm_machine_attribute_t	attribute,
	vm_machine_attribute_val_t* value)		/* IN/OUT */
{
        kprintf("not implemented: mach_vm_machine_attribute()\n");
        return 0;
}
/*
 * vm_machine_attribute -
 * Handle machine-specific attributes for a mapping, such
 * as cachability, migrability, etc. Limited addressability
 * (same range limits as for the native kernel map).
 */
kern_return_t
vm_machine_attribute(
	vm_map_t	map,
	vm_address_t	addr,
	vm_size_t	size,
	vm_machine_attribute_t	attribute,
	vm_machine_attribute_val_t* value)		/* IN/OUT */
{
        kprintf("not implemented: vm_machine_attribute()\n");
        return 0;
}
/*
 * mach_vm_read -
 * Read/copy a range from one address space and return it to the caller.
 *
 * It is assumed that the address for the returned memory is selected by
 * the IPC implementation as part of receiving the reply to this call.
 * If IPC isn't used, the caller must deal with the vm_map_copy_t object
 * that gets returned.
 * 
 * JMM - because of mach_msg_type_number_t, this call is limited to a
 * single 4GB region at this time.
 *
 */
kern_return_t
mach_vm_read(
	vm_map_t		map,
	mach_vm_address_t	addr,
	mach_vm_size_t	size,
	pointer_t		*data,
	mach_msg_type_number_t	*data_size)
{
	kern_return_t	error;
	vm_map_copy_t	ipc_address;

	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);
#ifdef __DARLING__
	if (size > 30*1024*1024)
		return KERN_INVALID_ARGUMENT;
#endif

	if ((mach_msg_type_number_t) size != size)
		return KERN_INVALID_ARGUMENT;
	
	error = vm_map_copyin(map,
			(vm_map_address_t)addr,
			(vm_map_size_t)size,
			FALSE,	/* src_destroy */
			&ipc_address);

	if (KERN_SUCCESS == error) {
		*data = (pointer_t) ipc_address;
		*data_size = (mach_msg_type_number_t) size;
		assert(*data_size == size);
	}
	return(error);
}

/*
kern_return_t
mach_vm_read(
	vm_map_t		map,
	mach_vm_address_t	addr,
	mach_vm_size_t	size,
	pointer_t		*data,
	mach_msg_type_number_t	*data_size)
{
	return vm_read(map, addr, size, data, data_size);
}
*/

/*
 * vm_read -
 * Read/copy a range from one address space and return it to the caller.
 * Limited addressability (same range limits as for the native kernel map).
 * 
 * It is assumed that the address for the returned memory is selected by
 * the IPC implementation as part of receiving the reply to this call.
 * If IPC isn't used, the caller must deal with the vm_map_copy_t object
 * that gets returned.
 */
kern_return_t
vm_read(
	vm_map_t		map,
	vm_address_t		addr,
	vm_size_t		size,
	pointer_t		*data,
	mach_msg_type_number_t	*data_size)
{
	kern_return_t	error;
	vm_map_copy_t	ipc_address;

	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);
#ifdef __DARLING__
	if (size > 30*1024*1024)
		return KERN_INVALID_ARGUMENT;
#endif

	if (size > (unsigned)(mach_msg_type_number_t) -1) {
		/*
		 * The kernel could handle a 64-bit "size" value, but
		 * it could not return the size of the data in "*data_size"
		 * without overflowing.
		 * Let's reject this "size" as invalid.
		 */
		return KERN_INVALID_ARGUMENT;
	}

	error = vm_map_copyin(map,
			(vm_map_address_t)addr,
			(vm_map_size_t)size,
			FALSE,	/* src_destroy */
			&ipc_address);

	if (KERN_SUCCESS == error) {
		*data = (pointer_t) ipc_address;
		*data_size = (mach_msg_type_number_t) size;
		assert(*data_size == size);
	}
	return(error);
}
/*
kern_return_t
vm_read(
	vm_map_t		map,
	vm_address_t		addr,
	vm_size_t		size,
	pointer_t		*data,
	mach_msg_type_number_t	*data_size)
{
	struct task_struct* ltask = map->linux_task;
	void* buf;
	int rd;
	//const bool big = size > 1024*1024;
	unsigned long uaddress = 0;
	kern_return_t rv = KERN_SUCCESS;

	if (size > 20*1024*1024)
		return KERN_INVALID_ARGUMENT;

	buf = vmalloc_user(size);
	printk(KERN_DEBUG " - allocated %d bytes at %p\n", size, buf);

	*data = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
	rd = access_process_vm(ltask, addr, buf, size, 0);
#else
	// Older kernels don't export access_process_vm(),
	// so we need to fall back to our own copy in access_process_vm.h
	rd = __access_process_vm(ltask, addr, buf, size, 0);
#endif

	if (rd <= 0)
	{
		rv = KERN_INVALID_ADDRESS;
		*data_size = 0;
		goto err;
	}

	*data_size = rd;

	if (rd > 0)
	{
		uaddress = vm_mmap(NULL, 0, rd, PROT_READ | PROT_WRITE,
			MAP_ANONYMOUS | MAP_PRIVATE, 0);
		printk(KERN_ERR "- mmapped user memory at %p\n", (void *)uaddress);

		if (IS_ERR((void*) uaddress))
		{
			printk(KERN_ERR "- vm_read(): failed to do vm_mmap with err %d\n", (int)uaddress);
			rv = KERN_FAILURE;
			goto err;
		}
		if (copy_to_user((void __user*) uaddress, buf, rd))
		{
			vm_munmap(uaddress, rd);

			printk(KERN_ERR "- vm_read(): failed to copy_to_user() into a vm_mapped() area\n");
			rv = KERN_FAILURE;
			goto err;
		}
	}
	*data = uaddress;

err:
	//if (big)
	//{
		printk(KERN_DEBUG "- using vfree\n");
		vfree(buf);
	//}
	//else
	//{
	//	printk(KERN_DEBUG "- using kfree\n");
	//	kfree(buf, size);
	//}

	return rv;
}
*/

/* 
 * mach_vm_read_list -
 * Read/copy a list of address ranges from specified map.
 *
 * MIG does not know how to deal with a returned array of
 * vm_map_copy_t structures, so we have to do the copyout
 * manually here.
 */
kern_return_t
mach_vm_read_list(
	vm_map_t			map,
	mach_vm_read_entry_t		data_list,
	natural_t			count)
{
        kprintf("not implemented: mach_vm_read_list()\n");
        return 0;
}
/* 
 * vm_read_list -
 * Read/copy a list of address ranges from specified map.
 *
 * MIG does not know how to deal with a returned array of
 * vm_map_copy_t structures, so we have to do the copyout
 * manually here.
 *
 * The source and destination ranges are limited to those
 * that can be described with a vm_address_t (i.e. same
 * size map as the kernel).
 *
 * JMM - If the result of the copyout is an address range
 * that cannot be described with a vm_address_t (i.e. the
 * caller had a larger address space but used this call
 * anyway), it will result in a truncated address being
 * returned (and a likely confused caller).
 */

kern_return_t
vm_read_list(
	vm_map_t		map,
	vm_read_entry_t	data_list,
	natural_t		count)
{
        kprintf("not implemented: vm_read_list()\n");
        return 0;
}
/*
 * mach_vm_read_overwrite -
 * Overwrite a range of the current map with data from the specified
 * map/address range.
 * 
 * In making an assumption that the current thread is local, it is
 * no longer cluster-safe without a fully supportive local proxy
 * thread/task (but we don't support cluster's anymore so this is moot).
 */

kern_return_t
mach_vm_read_overwrite(
	vm_map_t		map,
	mach_vm_address_t	address,
	mach_vm_size_t	size,
	mach_vm_address_t	data,
	mach_vm_size_t	*data_size)
{
	kern_return_t	error;
	vm_map_copy_t	copy;

	if (map == VM_MAP_NULL)
		return(KERN_INVALID_ARGUMENT);

	error = vm_map_copyin(map, (vm_map_address_t)address,
				(vm_map_size_t)size, FALSE, &copy);

	if (KERN_SUCCESS == error) {
		error = vm_map_copy_overwrite(current_thread()->map,
 					(vm_map_address_t)data, 
					copy, FALSE);
		if (KERN_SUCCESS == error) {
			*data_size = size;
			return error;
		}
		vm_map_copy_discard(copy);
	}
	return(error);
}
/*
 * vm_read_overwrite -
 * Overwrite a range of the current map with data from the specified
 * map/address range.
 * 
 * This routine adds the additional limitation that the source and
 * destination ranges must be describable with vm_address_t values
 * (i.e. the same size address spaces as the kernel, or at least the
 * the ranges are in that first portion of the respective address
 * spaces).
 */

kern_return_t
vm_read_overwrite(
	vm_map_t	map,
	vm_address_t	address,
	vm_size_t	size,
	vm_address_t	data,
	vm_size_t	*data_size)
{
        kprintf("not implemented: vm_read_overwrite()\n");
        return 0;
}

/*
 * mach_vm_write -
 * Overwrite the specified address range with the data provided
 * (from the current map).
 */
kern_return_t
mach_vm_write(
	vm_map_t			map,
	mach_vm_address_t		address,
	pointer_t			data,
	__unused mach_msg_type_number_t	size)
{
	if (map == VM_MAP_NULL)
		return KERN_INVALID_ARGUMENT;

	return vm_map_copy_overwrite(map, (vm_map_address_t)address,
		(vm_map_copy_t) data, FALSE /* interruptible XXX */);
}
/*
 * vm_write -
 * Overwrite the specified address range with the data provided
 * (from the current map).
 *
 * The addressability of the range of addresses to overwrite is
 * limited bu the use of a vm_address_t (same size as kernel map).
 * Either the target map is also small, or the range is in the
 * low addresses within it.
 */
kern_return_t
vm_write(
	vm_map_t			map,
	vm_address_t			address,
	pointer_t			data,
	__unused mach_msg_type_number_t	size)
{
	if (map == VM_MAP_NULL)
		return KERN_INVALID_ARGUMENT;

	return vm_map_copy_overwrite(map, (vm_map_address_t)address,
		(vm_map_copy_t) data, FALSE /* interruptible XXX */);
}
#if 0
/*
 * mach_vm_copy -
 * Overwrite one range of the specified map with the contents of
 * another range within that same map (i.e. both address ranges
 * are "over there").
 */
kern_return_t
mach_vm_copy(
	vm_map_t		map,
	mach_vm_address_t	source_address,
	mach_vm_size_t	size,
	mach_vm_address_t	dest_address)
{
        kprintf("not implemented: mach_vm_copy()\n");
        return 0;
}
kern_return_t
vm_copy(
	vm_map_t	map,
	vm_address_t	source_address,
	vm_size_t	size,
	vm_address_t	dest_address)
{
        kprintf("not implemented: vm_copy()\n");
        return 0;
}
#endif
/*
 * mach_vm_map -
 * Map some range of an object into an address space.
 *
 * The object can be one of several types of objects:
 *	NULL - anonymous memory
 *	a named entry - a range within another address space
 *	                or a range within a memory object
 *	a whole memory object
 *
 */
kern_return_t
mach_vm_map(
	vm_map_t		target_map,
	mach_vm_offset_t	*address,
	mach_vm_size_t	initial_size,
	mach_vm_offset_t	mask,
	int			flags,
	ipc_port_t		port,
	vm_object_offset_t	offset,
	boolean_t		copy,
	vm_prot_t		cur_protection,
	vm_prot_t		max_protection,
	vm_inherit_t		inheritance)
{
        kprintf("not implemented: mach_vm_map()\n");
        return 0;
}

/* legacy interface */
kern_return_t
vm_map_64(
	vm_map_t		target_map,
	vm_offset_t		*address,
	vm_size_t		size,
	vm_offset_t		mask,
	int			flags,
	ipc_port_t		port,
	vm_object_offset_t	offset,
	boolean_t		copy,
	vm_prot_t		cur_protection,
	vm_prot_t		max_protection,
	vm_inherit_t		inheritance)
{
        kprintf("not implemented: vm_map_64()\n");
        return 0;
}
/* temporary, until world build */
kern_return_t
vm_map(
	vm_map_t		target_map,
	vm_offset_t		*address,
	vm_size_t		size,
	vm_offset_t		mask,
	int			flags,
	ipc_port_t		port,
	vm_offset_t		offset,
	boolean_t		copy,
	vm_prot_t		cur_protection,
	vm_prot_t		max_protection,
	vm_inherit_t		inheritance)
{
        kprintf("not implemented: vm_map()\n");
        return 0;
}
/*
 * mach_vm_remap -
 * Remap a range of memory from one task into another,
 * to another address range within the same task, or
 * over top of itself (with altered permissions and/or
 * as an in-place copy of itself).
 */

kern_return_t
mach_vm_remap(
	vm_map_t		target_map,
	mach_vm_offset_t	*address,
	mach_vm_size_t	size,
	mach_vm_offset_t	mask,
	int			flags,
	vm_map_t		src_map,
	mach_vm_offset_t	memory_address,
	boolean_t		copy,
	vm_prot_t		*cur_protection,
	vm_prot_t		*max_protection,
	vm_inherit_t		inheritance)
{
        kprintf("not implemented: mach_vm_remap()\n");
        return 0;
}
/*
 * vm_remap -
 * Remap a range of memory from one task into another,
 * to another address range within the same task, or
 * over top of itself (with altered permissions and/or
 * as an in-place copy of itself).
 *
 * The addressability of the source and target address
 * range is limited by the size of vm_address_t (in the
 * kernel context).
 */
kern_return_t
vm_remap(
	vm_map_t		target_map,
	vm_offset_t		*address,
	vm_size_t		size,
	vm_offset_t		mask,
	int			flags,
	vm_map_t		src_map,
	vm_offset_t		memory_address,
	boolean_t		copy,
	vm_prot_t		*cur_protection,
	vm_prot_t		*max_protection,
	vm_inherit_t		inheritance)
{
        kprintf("not implemented: vm_remap()\n");
        return 0;
}
/*
 * NOTE: these routine (and this file) will no longer require mach_host_server.h
 * when mach_vm_wire and vm_wire are changed to use ledgers.
 */
#include <mach/mach_host_server.h>
/*
 *	mach_vm_wire
 *	Specify that the range of the virtual address space
 *	of the target task must not cause page faults for
 *	the indicated accesses.
 *
 *	[ To unwire the pages, specify VM_PROT_NONE. ]
 */
kern_return_t
mach_vm_wire(
	host_priv_t		host_priv,
	vm_map_t		map,
	mach_vm_offset_t	start,
	mach_vm_size_t	size,
	vm_prot_t		access)
{
        kprintf("not implemented: mach_vm_wire()\n");
        return 0;
}
/*
 *	vm_wire -
 *	Specify that the range of the virtual address space
 *	of the target task must not cause page faults for
 *	the indicated accesses.
 *
 *	[ To unwire the pages, specify VM_PROT_NONE. ]
 */
kern_return_t
vm_wire(
	host_priv_t		host_priv,
	register vm_map_t	map,
	vm_offset_t		start,
	vm_size_t		size,
	vm_prot_t		access)
{
        kprintf("not implemented: vm_wire()\n");
        return 0;
}
/*
 *	vm_msync
 *
 *	Synchronises the memory range specified with its backing store
 *	image by either flushing or cleaning the contents to the appropriate
 *	memory manager.
 *
 *	interpretation of sync_flags
 *	VM_SYNC_INVALIDATE	- discard pages, only return precious
 *				  pages to manager.
 *
 *	VM_SYNC_INVALIDATE & (VM_SYNC_SYNCHRONOUS | VM_SYNC_ASYNCHRONOUS)
 *				- discard pages, write dirty or precious
 *				  pages back to memory manager.
 *
 *	VM_SYNC_SYNCHRONOUS | VM_SYNC_ASYNCHRONOUS
 *				- write dirty or precious pages back to
 *				  the memory manager.
 *
 *	VM_SYNC_CONTIGUOUS	- does everything normally, but if there
 *				  is a hole in the region, and we would
 *				  have returned KERN_SUCCESS, return
 *				  KERN_INVALID_ADDRESS instead.
 *
 *	RETURNS
 *	KERN_INVALID_TASK		Bad task parameter
 *	KERN_INVALID_ARGUMENT		both sync and async were specified.
 *	KERN_SUCCESS			The usual.
 *	KERN_INVALID_ADDRESS		There was a hole in the region.
 */

kern_return_t
mach_vm_msync(
	vm_map_t		map,
	mach_vm_address_t	address,
	mach_vm_size_t	size,
	vm_sync_t		sync_flags)
{
        kprintf("not implemented: mach_vm_msync()\n");
        return 0;
}
/*
 *	vm_msync
 *
 *	Synchronises the memory range specified with its backing store
 *	image by either flushing or cleaning the contents to the appropriate
 *	memory manager.
 *
 *	interpretation of sync_flags
 *	VM_SYNC_INVALIDATE	- discard pages, only return precious
 *				  pages to manager.
 *
 *	VM_SYNC_INVALIDATE & (VM_SYNC_SYNCHRONOUS | VM_SYNC_ASYNCHRONOUS)
 *				- discard pages, write dirty or precious
 *				  pages back to memory manager.
 *
 *	VM_SYNC_SYNCHRONOUS | VM_SYNC_ASYNCHRONOUS
 *				- write dirty or precious pages back to
 *				  the memory manager.
 *
 *	VM_SYNC_CONTIGUOUS	- does everything normally, but if there
 *				  is a hole in the region, and we would
 *				  have returned KERN_SUCCESS, return
 *				  KERN_INVALID_ADDRESS instead.
 *
 *	The addressability of the range is limited to that which can
 *	be described by a vm_address_t.
 *
 *	RETURNS
 *	KERN_INVALID_TASK		Bad task parameter
 *	KERN_INVALID_ARGUMENT		both sync and async were specified.
 *	KERN_SUCCESS			The usual.
 *	KERN_INVALID_ADDRESS		There was a hole in the region.
 */

kern_return_t
vm_msync(
	vm_map_t	map,
	vm_address_t	address,
	vm_size_t	size,
	vm_sync_t	sync_flags)
{
        kprintf("not implemented: vm_msync()\n");
        return 0;
}

int
vm_toggle_entry_reuse(int toggle, int *old_value)
{
        kprintf("not implemented: vm_toggle_entry_reuse()\n");
        return 0;
}

/*
 *	mach_vm_behavior_set 
 *
 *	Sets the paging behavior attribute for the  specified range
 *	in the specified map.
 *
 *	This routine will fail with KERN_INVALID_ADDRESS if any address
 *	in [start,start+size) is not a valid allocated memory region.
 */
kern_return_t 
mach_vm_behavior_set(
	vm_map_t		map,
	mach_vm_offset_t	start,
	mach_vm_size_t	size,
	vm_behavior_t		new_behavior)
{
        kprintf("not implemented: mach_vm_behavior_set()\n");
        return 0;
}

/*
 *	vm_behavior_set 
 *
 *	Sets the paging behavior attribute for the  specified range
 *	in the specified map.
 *
 *	This routine will fail with KERN_INVALID_ADDRESS if any address
 *	in [start,start+size) is not a valid allocated memory region.
 *
 *	This routine is potentially limited in addressibility by the
 *	use of vm_offset_t (if the map provided is larger than the
 *	kernel's).
 */
kern_return_t 
vm_behavior_set(
	vm_map_t		map,
	vm_offset_t		start,
	vm_size_t		size,
	vm_behavior_t		new_behavior)
{
        kprintf("not implemented: vm_behavior_set()\n");
        return 0;
}

/*
 *	mach_vm_region:
 *
 *	User call to obtain information about a region in
 *	a task's address map. Currently, only one flavor is
 *	supported.
 *
 *	XXX The reserved and behavior fields cannot be filled
 *	    in until the vm merge from the IK is completed, and
 *	    vm_reserve is implemented.
 *
 *	XXX Dependency: syscall_vm_region() also supports only one flavor.
 */

#if 0
kern_return_t
mach_vm_region(
	vm_map_t		 map,
	mach_vm_offset_t	*address,		/* IN/OUT */
	mach_vm_size_t	*size,			/* OUT */
	vm_region_flavor_t	 flavor,		/* IN */
	vm_region_info_t	 info,			/* OUT */
	mach_msg_type_number_t	*count,			/* IN/OUT */
	mach_port_t		*object_name)		/* OUT */
{
        kprintf("not implemented: mach_vm_region()\n");
        return 0;
}
#endif

/*
 *	vm_region_64 and vm_region:
 *
 *	User call to obtain information about a region in
 *	a task's address map. Currently, only one flavor is
 *	supported.
 *
 *	XXX The reserved and behavior fields cannot be filled
 *	    in until the vm merge from the IK is completed, and
 *	    vm_reserve is implemented.
 *
 *	XXX Dependency: syscall_vm_region() also supports only one flavor.
 */

kern_return_t
vm_region_64(
	vm_map_t		 map,
	vm_offset_t	        *address,		/* IN/OUT */
	vm_size_t		*size,			/* OUT */
	vm_region_flavor_t	 flavor,		/* IN */
	vm_region_info_t	 info,			/* OUT */
	mach_msg_type_number_t	*count,			/* IN/OUT */
	mach_port_t		*object_name)		/* OUT */
{
        kprintf("not implemented: vm_region_64()\n");
        return 0;
}

kern_return_t
vm_region(
	vm_map_t			map,
	vm_address_t	      		*address,	/* IN/OUT */
	vm_size_t			*size,		/* OUT */
	vm_region_flavor_t	 	flavor,	/* IN */
	vm_region_info_t	 	info,		/* OUT */
	mach_msg_type_number_t	*count,	/* IN/OUT */
	mach_port_t			*object_name)	/* OUT */
{
        kprintf("not implemented: vm_region()\n");
        return 0;
}
/*
 *	vm_region_recurse: A form of vm_region which follows the
 *	submaps in a target map
 *
 */
#if 0
kern_return_t
mach_vm_region_recurse(
	vm_map_t			map,
	mach_vm_address_t		*address,
	mach_vm_size_t		*size,
	uint32_t			*depth,
	vm_region_recurse_info_t	info,
	mach_msg_type_number_t 	*infoCnt)
{
        kprintf("not implemented: mach_vm_region_recurse()\n");
        return 0;
}
#endif
/*
 *	vm_region_recurse: A form of vm_region which follows the
 *	submaps in a target map
 *
 */
kern_return_t
vm_region_recurse_64(
	vm_map_t			map,
	vm_address_t			*address,
	vm_size_t			*size,
	uint32_t			*depth,
	vm_region_recurse_info_64_t	info,
	mach_msg_type_number_t 	*infoCnt)
{
        kprintf("not implemented: vm_region_recurse_64()\n");
        return 0;
}
kern_return_t
vm_region_recurse(
	vm_map_t			map,
	vm_offset_t	       	*address,	/* IN/OUT */
	vm_size_t			*size,		/* OUT */
	natural_t	 		*depth,	/* IN/OUT */
	vm_region_recurse_info_t	info32,	/* IN/OUT */
	mach_msg_type_number_t	*infoCnt)	/* IN/OUT */
{
        kprintf("not implemented: vm_region_recurse()\n");
        return 0;
}
kern_return_t
mach_vm_purgable_control(
	vm_map_t		map,
	mach_vm_offset_t	address,
	vm_purgable_t		control,
	int			*state)
{
        kprintf("not implemented: mach_vm_purgable_control()\n");
        return 0;
}
kern_return_t
vm_purgable_control(
	vm_map_t		map,
	vm_offset_t		address,
	vm_purgable_t		control,
	int			*state)
{
        kprintf("not implemented: vm_purgable_control()\n");
        return 0;
}

/*
 *	Ordinarily, the right to allocate CPM is restricted
 *	to privileged applications (those that can gain access
 *	to the host priv port).  Set this variable to zero if
 *	you want to let any application allocate CPM.
 */
unsigned int	vm_allocate_cpm_privileged = 0;

/*
 *	Allocate memory in the specified map, with the caveat that
 *	the memory is physically contiguous.  This call may fail
 *	if the system can't find sufficient contiguous memory.
 *	This call may cause or lead to heart-stopping amounts of
 *	paging activity.
 *
 *	Memory obtained from this call should be freed in the
 *	normal way, viz., via vm_deallocate.
 */
kern_return_t
vm_allocate_cpm(
	host_priv_t		host_priv,
	vm_map_t		map,
	vm_address_t		*addr,
	vm_size_t		size,
	int			flags)
{
        kprintf("not implemented: vm_allocate_cpm()\n");
        return 0;
}

kern_return_t
mach_vm_page_query(
	vm_map_t		map,
	mach_vm_offset_t	offset,
	int			*disposition,
	int			*ref_count)
{
        kprintf("not implemented: mach_vm_page_query()\n");
        return 0;
}
kern_return_t
vm_map_page_query(
	vm_map_t		map,
	vm_offset_t		offset,
	int			*disposition,
	int			*ref_count)
{
        kprintf("not implemented: vm_map_page_query()\n");
        return 0;
}
kern_return_t
mach_vm_page_info(
	vm_map_t		map,
	mach_vm_address_t	address,
	vm_page_info_flavor_t	flavor,
	vm_page_info_t		info,
	mach_msg_type_number_t	*count)
{
        kprintf("not implemented: mach_vm_page_info()\n");
        return 0;
}
/* map a (whole) upl into an address space */
kern_return_t
vm_upl_map(
	vm_map_t		map, 
	upl_t			upl, 
	vm_address_t		*dst_addr)
{
        kprintf("not implemented: vm_upl_map()\n");
        return 0;
}
kern_return_t
vm_upl_unmap(
	vm_map_t		map,
	upl_t 			upl)
{
        kprintf("not implemented: vm_upl_unmap()\n");
        return 0;
}
/* Retrieve a upl for an object underlying an address range in a map */

kern_return_t vm_map_get_upl(
                vm_map_t        target_map,
                vm_map_offset_t     map_offset,
                upl_size_t      *size,
                upl_t           *upl,
                upl_page_info_array_t   page_info,
                unsigned int        *page_infoCnt,
                upl_control_flags_t *flags,
                int         force_data_sync)
{
        kprintf("not implemented: vm_map_get_upl()\n");
        return 0;
}
/*
 * mach_make_memory_entry_64
 *
 * Think of it as a two-stage vm_remap() operation.  First
 * you get a handle.  Second, you get map that handle in
 * somewhere else. Rather than doing it all at once (and
 * without needing access to the other whole map).
 */

kern_return_t
mach_make_memory_entry_64(
	vm_map_t		target_map,
	memory_object_size_t	*size,
	memory_object_offset_t offset,
	vm_prot_t		permission,
	ipc_port_t		*object_handle,
	ipc_port_t		parent_handle)
{
        kprintf("not implemented: mach_make_memory_entry_64()\n");
        return 0;
}
kern_return_t
_mach_make_memory_entry(
	vm_map_t		target_map,
	memory_object_size_t	*size,
	memory_object_offset_t	offset,
	vm_prot_t		permission,
	ipc_port_t		*object_handle,
	ipc_port_t		parent_entry)
{
        kprintf("not implemented: _mach_make_memory_entry()\n");
        return 0;
}
kern_return_t
mach_make_memory_entry(
	vm_map_t		target_map,
	vm_size_t		*size,
	vm_offset_t		offset,
	vm_prot_t		permission,
	ipc_port_t		*object_handle,
	ipc_port_t		parent_entry)
{
        kprintf("not implemented: mach_make_memory_entry()\n");
        return 0;
}
/*
 *	task_wire
 *
 *	Set or clear the map's wiring_required flag.  This flag, if set,
 *	will cause all future virtual memory allocation to allocate
 *	user wired memory.  Unwiring pages wired down as a result of
 *	this routine is done with the vm_wire interface.
 */
kern_return_t
task_wire(
	vm_map_t	map,
	boolean_t	must_wire)
{
        kprintf("not implemented: task_wire()\n");
        return 0;
}
/* __private_extern__ */ extern kern_return_t
mach_memory_entry_allocate(
	vm_named_entry_t	*user_entry_p,
	ipc_port_t		*user_handle_p)
{
        kprintf("not implemented: mach_memory_entry_allocate()\n");
        return 0;
}
/*
 *	mach_memory_object_memory_entry_64
 *
 *	Create a named entry backed by the provided pager.
 *
 *	JMM - we need to hold a reference on the pager -
 *	and release it when the named entry is destroyed.
 */
kern_return_t
mach_memory_object_memory_entry_64(
	host_t			host,
	boolean_t		internal,
	vm_object_offset_t	size,
	vm_prot_t		permission,
 	memory_object_t		pager,
	ipc_port_t		*entry_handle)
{
        kprintf("not implemented: mach_memory_object_memory_entry_64()\n");
        return 0;
}
kern_return_t
mach_memory_object_memory_entry(
	host_t		host,
	boolean_t	internal,
	vm_size_t	size,
	vm_prot_t	permission,
 	memory_object_t	pager,
	ipc_port_t	*entry_handle)
{
        kprintf("not implemented: mach_memory_object_memory_entry()\n");
        return 0;
}

kern_return_t
mach_memory_entry_purgable_control(
	ipc_port_t	entry_port,
	vm_purgable_t	control,
	int		*state)
{
        kprintf("not implemented: mach_memory_entry_purgable_control()\n");
        return 0;
}
/*
 * mach_memory_entry_port_release:
 *
 * Release a send right on a named entry port.  This is the correct
 * way to destroy a named entry.  When the last right on the port is
 * released, ipc_kobject_destroy() will call mach_destroy_memory_entry().
 */
void
mach_memory_entry_port_release(
	ipc_port_t	port)
{
		kprintf("not implemented: mach_memory_entry_port_release()\n");
}

/*
 * mach_destroy_memory_entry:
 *
 * Drops a reference on a memory entry and destroys the memory entry if
 * there are no more references on it.
 * NOTE: This routine should not be called to destroy a memory entry from the
 * kernel, as it will not release the Mach port associated with the memory
 * entry.  The proper way to destroy a memory entry in the kernel is to
 * call mach_memort_entry_port_release() to release the kernel's send-right on
 * the memory entry's port.  When the last send right is released, the memory
 * entry will be destroyed via ipc_kobject_destroy().
 */
void
mach_destroy_memory_entry(
	ipc_port_t	port)
{
		kprintf("not implemented: mach_destroy_memory_entry()\n");
}

/* Allow manipulation of individual page state.  This is actually part of */
/* the UPL regimen but takes place on the memory entry rather than on a UPL */

kern_return_t
mach_memory_entry_page_op(
	ipc_port_t		entry_port,
	vm_object_offset_t	offset,
	int			ops,
	ppnum_t			*phys_entry,
	int			*flags)
{
        kprintf("not implemented: mach_memory_entry_page_op()\n");
        return 0;
}
/*
 * mach_memory_entry_range_op offers performance enhancement over 
 * mach_memory_entry_page_op for page_op functions which do not require page 
 * level state to be returned from the call.  Page_op was created to provide 
 * a low-cost alternative to page manipulation via UPLs when only a single 
 * page was involved.  The range_op call establishes the ability in the _op 
 * family of functions to work on multiple pages where the lack of page level
 * state handling allows the caller to avoid the overhead of the upl structures.
 */

kern_return_t
mach_memory_entry_range_op(
	ipc_port_t		entry_port,
	vm_object_offset_t	offset_beg,
	vm_object_offset_t	offset_end,
	int                     ops,
	int                     *range)
{
        kprintf("not implemented: mach_memory_entry_range_op()\n");
        return 0;
}

kern_return_t
set_dp_control_port(
	host_priv_t	host_priv,
	ipc_port_t	control_port)	
{
        kprintf("not implemented: set_dp_control_port()\n");
        return 0;
}
kern_return_t
get_dp_control_port(
	host_priv_t	host_priv,
	ipc_port_t	*control_port)	
{
        kprintf("not implemented: get_dp_control_port()\n");
        return 0;
}
/* ******* Temporary Internal calls to UPL for BSD ***** */

extern int kernel_upl_map(
	vm_map_t        map,
	upl_t           upl,
	vm_offset_t     *dst_addr);

extern int kernel_upl_unmap(
	vm_map_t        map,
	upl_t           upl);

extern int kernel_upl_commit(
	upl_t                   upl,
	upl_page_info_t         *pl,
	mach_msg_type_number_t	 count);

extern int kernel_upl_commit_range(
	upl_t                   upl,
	upl_offset_t             offset,
	upl_size_t		size,
	int			flags,
	upl_page_info_array_t	pl,
	mach_msg_type_number_t	count);

extern int kernel_upl_abort(
	upl_t                   upl,
	int                     abort_type);

extern int kernel_upl_abort_range(
	upl_t                   upl,
	upl_offset_t             offset,
	upl_size_t               size,
	int                     abort_flags);


kern_return_t
kernel_upl_map(
	vm_map_t	map,
	upl_t		upl,
	vm_offset_t	*dst_addr)
{
        kprintf("not implemented: kernel_upl_map()\n");
        return 0;
}

kern_return_t
kernel_upl_unmap(
	vm_map_t	map,
	upl_t		upl)
{
        kprintf("not implemented: kernel_upl_unmap()\n");
        return 0;
}
kern_return_t
kernel_upl_commit(
	upl_t                   upl,
	upl_page_info_t        *pl,
	mach_msg_type_number_t  count)
{
        kprintf("not implemented: kernel_upl_commit()\n");
        return 0;
}

kern_return_t
kernel_upl_commit_range(
	upl_t 			upl,
	upl_offset_t		offset,
	upl_size_t		size,
	int			flags,
	upl_page_info_array_t   pl,
	mach_msg_type_number_t  count)
{
        kprintf("not implemented: kernel_upl_commit_range()\n");
        return 0;
}
kern_return_t
kernel_upl_abort_range(
	upl_t			upl,
	upl_offset_t		offset,
	upl_size_t		size,
	int			abort_flags)
{
        kprintf("not implemented: kernel_upl_abort_range()\n");
        return 0;
}
kern_return_t
kernel_upl_abort(
	upl_t			upl,
	int			abort_type)
{
        kprintf("not implemented: kernel_upl_abort()\n");
        return 0;
}
/*
 * Now a kernel-private interface (for BootCache
 * use only).  Need a cleaner way to create an
 * empty vm_map() and return a handle to it.
 */

kern_return_t
vm_region_object_create(
	__unused vm_map_t	target_map,
	vm_size_t		size,
	ipc_port_t		*object_handle)
{
        kprintf("not implemented: vm_region_object_create()\n");
        return 0;
}


kern_return_t kernel_object_iopl_request(	/* forward */
	vm_named_entry_t	named_entry,
	memory_object_offset_t	offset,
	upl_size_t		*upl_size,
	upl_t			*upl_ptr,
	upl_page_info_array_t	user_page_list,
	unsigned int		*page_list_count,
	int			*flags);

kern_return_t
kernel_object_iopl_request(
	vm_named_entry_t	named_entry,
	memory_object_offset_t	offset,
	upl_size_t		*upl_size,
	upl_t			*upl_ptr,
	upl_page_info_array_t	user_page_list,
	unsigned int		*page_list_count,
	int			*flags)
{
        kprintf("not implemented: kernel_object_iopl_request()\n");
        return 0;
}
