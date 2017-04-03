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
 *	File:	vm/vm_map.c
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *	Date:	1985
 *
 *	Virtual memory mapping module.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <task_swapper.h>
#include <mach_assert.h>
#include <libkern/OSAtomic.h>

#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <mach/vm_behavior.h>
#include <mach/vm_statistics.h>
//#include <mach/memory_object.h>
#include <mach/mach_vm.h>
#include <machine/cpu_capabilities.h>
#include <mach/sdt.h>

#include <kern/assert.h>
#include <kern/counters.h>
#include <kern/kalloc.h>
#include <kern/zalloc.h>

#include <vm/cpm.h>
#include <vm/vm_init.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/vm_kern.h>
#include <ipc/ipc_port.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>
#include <kern/xpr.h>

#include <mach/vm_map_server.h>
#include <mach/mach_host_server.h>
#include <vm/vm_protos.h>
#include <vm/vm_purgeable_internal.h>

#include <vm/vm_protos.h>
#include <vm/vm_shared_region.h>
#include <vm/vm_map_store.h>

extern u_int32_t random(void);	/* from <libkern/libkern.h> */
/* Internal prototypes
 */



/*
 *	Virtual memory maps provide for the mapping, protection,
 *	and sharing of virtual memory objects.  In addition,
 *	this module provides for an efficient virtual copy of
 *	memory from one map to another.
 *
 *	Synchronization is required prior to most operations.
 *
 *	Maps consist of an ordered doubly-linked list of simple
 *	entries; a single hint is used to speed up lookups.
 *
 *	Sharing maps have been deleted from this version of Mach.
 *	All shared objects are now mapped directly into the respective
 *	maps.  This requires a change in the copy on write strategy;
 *	the asymmetric (delayed) strategy is used for shared temporary
 *	objects instead of the symmetric (shadow) strategy.  All maps
 *	are now "top level" maps (either task map, kernel map or submap
 *	of the kernel map).  
 *
 *	Since portions of maps are specified by start/end addreses,
 *	which may not align with existing map entries, all
 *	routines merely "clip" entries to these start/end values.
 *	[That is, an entry is split into two, bordering at a
 *	start or end value.]  Note that these clippings may not
 *	always be necessary (as the two resulting entries are then
 *	not changed); however, the clipping is done for convenience.
 *	No attempt is currently made to "glue back together" two
 *	abutting entries.
 *
 *	The symmetric (shadow) copy strategy implements virtual copy
 *	by copying VM object references from one map to
 *	another, and then marking both regions as copy-on-write.
 *	It is important to note that only one writeable reference
 *	to a VM object region exists in any map when this strategy
 *	is used -- this means that shadow object creation can be
 *	delayed until a write operation occurs.  The symmetric (delayed)
 *	strategy allows multiple maps to have writeable references to
 *	the same region of a vm object, and hence cannot delay creating
 *	its copy objects.  See vm_object_copy_quickly() in vm_object.c.
 *	Copying of permanent objects is completely different; see
 *	vm_object_copy_strategically() in vm_object.c.
 */

kern_return_t
vm_map_set_cache_attr(
	vm_map_t	map,
	vm_map_offset_t	va)
{
        kprintf("not implemented: vm_map_set_cache_attr()\n");
        return 0;
}

#if CONFIG_CODE_DECRYPTION
/*
 * vm_map_apple_protected:
 * This remaps the requested part of the object with an object backed by 
 * the decrypting pager.
 * crypt_info contains entry points and session data for the crypt module.
 * The crypt_info block will be copied by vm_map_apple_protected. The data structures
 * referenced in crypt_info must remain valid until crypt_info->crypt_end() is called.
 */
kern_return_t
vm_map_apple_protected(
	vm_map_t	map,
	vm_map_offset_t	start,
	vm_map_offset_t	end,
	struct pager_crypt_info *crypt_info)
{
        kprintf("not implemented: vm_map_apple_protected()\n");
        return 0;
}
#endif	/* CONFIG_CODE_DECRYPTION */



lck_grp_t		vm_map_lck_grp;
lck_grp_attr_t	vm_map_lck_grp_attr;
lck_attr_t		vm_map_lck_attr;


/*
 *	vm_map_init:
 *
 *	Initialize the vm_map module.  Must be called before
 *	any other vm_map routines.
 *
 *	Map and entry structures are allocated from zones -- we must
 *	initialize those zones.
 *
 *	There are three zones of interest:
 *
 *	vm_map_zone:		used to allocate maps.
 *	vm_map_entry_zone:	used to allocate map entries.
 *	vm_map_entry_reserved_zone:	fallback zone for kernel map entries
 *
 *	The kernel allocates map entries from a special zone that is initially
 *	"crammed" with memory.  It would be difficult (perhaps impossible) for
 *	the kernel to allocate more memory to a entry zone when it became
 *	empty since the very act of allocating memory implies the creation
 *	of a new entry.
 */


void
vm_map_steal_memory(
	void)
{
        kprintf("not implemented: vm_map_steal_memory()\n");
}

void vm_kernel_reserved_entry_init(void) 
{
        kprintf("not implemented: vm_kernel_reserved_entry_init()\n");
}


/*
 *	vm_map_entry_create:	[ internal use only ]
 *
 *	Allocates a VM map entry for insertion in the
 *	given map (or map copy).  No fields are filled.
 */
#define	vm_map_entry_create(map, map_locked)	_vm_map_entry_create(&(map)->hdr, map_locked)

#define	vm_map_copy_entry_create(copy, map_locked)					\
	_vm_map_entry_create(&(copy)->cpy_hdr, map_locked)
unsigned reserved_zalloc_count, nonreserved_zalloc_count;


/*
 *	vm_map_entry_dispose:	[ internal use only ]
 *
 *	Inverse of vm_map_entry_create.
 *
 * 	write map lock held so no need to
 *	do anything special to insure correctness
 * 	of the stores
 */
#define	vm_map_entry_dispose(map, entry)			\
	_vm_map_entry_dispose(&(map)->hdr, (entry))

#define	vm_map_copy_entry_dispose(map, entry) \
	_vm_map_entry_dispose(&(copy)->cpy_hdr, (entry))



#if MACH_ASSERT

#endif /* MACH_ASSERT */


#define vm_map_copy_entry_link(copy, after_where, entry)		\
	_vm_map_store_entry_link(&(copy)->cpy_hdr, after_where, (entry))

#define vm_map_copy_entry_unlink(copy, entry)				\
	_vm_map_store_entry_unlink(&(copy)->cpy_hdr, (entry))

#if	MACH_ASSERT && TASK_SWAPPER
/*
 *	vm_map_res_reference:
 *
 *	Adds another valid residence count to the given map.
 *
 *	Map is locked so this function can be called from
 *	vm_map_swapin.
 *
 */
void vm_map_res_reference(register vm_map_t map)
{
        kprintf("not implemented: vm_map_res_reference()\n");
}
/*
 *	vm_map_reference_swap:
 *
 *	Adds valid reference and residence counts to the given map.
 *
 *	The map may not be in memory (i.e. zero residence count).
 *
 */
void vm_map_reference_swap(register vm_map_t map)
{
        kprintf("not implemented: vm_map_reference_swap()\n");
}
/*
 *	vm_map_res_deallocate:
 *
 *	Decrement residence count on a map; possibly causing swapout.
 *
 *	The map must be in memory (i.e. non-zero residence count).
 *
 *	The map is locked, so this function is callable from vm_map_deallocate.
 *
 */
void vm_map_res_deallocate(register vm_map_t map)
{
        kprintf("not implemented: vm_map_res_deallocate()\n");
}
#endif	/* MACH_ASSERT && TASK_SWAPPER */

/*
 *	vm_map_destroy:
 *
 *	Actually destroy a map.
 */
void
vm_map_destroy(
	vm_map_t	map,
	int		flags)
{
		kprintf("not implemented: vm_map_destroy()\n");
}
#if	TASK_SWAPPER
/*
 * vm_map_swapin/vm_map_swapout
 *
 * Swap a map in and out, either referencing or releasing its resources.  
 * These functions are internal use only; however, they must be exported
 * because they may be called from macros, which are exported.
 *
 * In the case of swapout, there could be races on the residence count, 
 * so if the residence count is up, we return, assuming that a 
 * vm_map_deallocate() call in the near future will bring us back.
 *
 * Locking:
 *	-- We use the map write lock for synchronization among races.
 *	-- The map write lock, and not the simple s_lock, protects the
 *	   swap state of the map.
 *	-- If a map entry is a share map, then we hold both locks, in
 *	   hierarchical order.
 *
 * Synchronization Notes:
 *	1) If a vm_map_swapin() call happens while swapout in progress, it
 *	will block on the map lock and proceed when swapout is through.
 *	2) A vm_map_reference() call at this time is illegal, and will
 *	cause a panic.  vm_map_reference() is only allowed on resident
 *	maps, since it refuses to block.
 *	3) A vm_map_swapin() call during a swapin will block, and 
 *	proceeed when the first swapin is done, turning into a nop.
 *	This is the reason the res_count is not incremented until
 *	after the swapin is complete.
 *	4) There is a timing hole after the checks of the res_count, before
 *	the map lock is taken, during which a swapin may get the lock
 *	before a swapout about to happen.  If this happens, the swapin
 *	will detect the state and increment the reference count, causing
 *	the swapout to be a nop, thereby delaying it until a later 
 *	vm_map_deallocate.  If the swapout gets the lock first, then 
 *	the swapin will simply block until the swapout is done, and 
 *	then proceed.
 *
 * Because vm_map_swapin() is potentially an expensive operation, it
 * should be used with caution.
 *
 * Invariants:
 *	1) A map with a residence count of zero is either swapped, or
 *	   being swapped.
 *	2) A map with a non-zero residence count is either resident,
 *	   or being swapped in.
 */

int vm_map_swap_enable = 1;

void vm_map_swapin (vm_map_t map)
{
		kprintf("not implemented: vm_map_swapin()\n");
}
void vm_map_swapout(vm_map_t map)
{
		kprintf("not implemented: vm_map_swapout()\n");
}
#endif	/* TASK_SWAPPER */

/*
 *	vm_map_lookup_entry:	[ internal use only ]
 *
 *	Calls into the vm map store layer to find the map 
 *	entry containing (or immediately preceding) the 
 *	specified address in the given map; the entry is returned
 *	in the "entry" parameter.  The boolean
 *	result indicates whether the address is
 *	actually contained in the map.
 */
boolean_t
vm_map_lookup_entry(
	register vm_map_t		map,
	register vm_map_offset_t	address,
	vm_map_entry_t		*entry)		/* OUT */
{
        kprintf("not implemented: vm_map_lookup_entry()\n");
        return 0;
}
/*
 *	Routine:	vm_map_find_space
 *	Purpose:
 *		Allocate a range in the specified virtual address map,
 *		returning the entry allocated for that range.
 *		Used by kmem_alloc, etc.
 *
 *		The map must be NOT be locked. It will be returned locked
 *		on KERN_SUCCESS, unlocked on failure.
 *
 *		If an entry is allocated, the object/offset fields
 *		are initialized to zero.
 */
kern_return_t
vm_map_find_space(
	register vm_map_t	map,
	vm_map_offset_t		*address,	/* OUT */
	vm_map_size_t		size,
	vm_map_offset_t		mask,
	int			flags,
	vm_map_entry_t		*o_entry)	/* OUT */
{
        kprintf("not implemented: vm_map_find_space()\n");
        return 0;
}
int vm_map_pmap_enter_print = TRUE;
int vm_map_pmap_enter_enable = TRUE;

/*
 *	Routine:	vm_map_pmap_enter [internal only]
 *
 *	Description:
 *		Force pages from the specified object to be entered into
 *		the pmap at the specified address if they are present.
 *		As soon as a page not found in the object the scan ends.
 *
 *	Returns:
 *		Nothing.  
 *
 *	In/out conditions:
 *		The source map should not be locked on entry.
 */


boolean_t vm_map_pmap_is_empty(
	vm_map_t	map,
	vm_map_offset_t	start,
	vm_map_offset_t end);
boolean_t vm_map_pmap_is_empty(
	vm_map_t	map,
	vm_map_offset_t	start,
	vm_map_offset_t	end)
{
        kprintf("not implemented: vm_map_pmap_is_empty()\n");
        return 0;
}
#define MAX_TRIES_TO_GET_RANDOM_ADDRESS	1000
kern_return_t
vm_map_random_address_for_size(
	vm_map_t	map,
	vm_map_offset_t	*address,
	vm_map_size_t	size)
{
        kprintf("not implemented: vm_map_random_address_for_size()\n");
        return 0;
}
/*
 *	Routine:	vm_map_enter
 *
 *	Description:
 *		Allocate a range in the specified virtual address map.
 *		The resulting range will refer to memory defined by
 *		the given memory object and offset into that object.
 *
 *		Arguments are as defined in the vm_map call.
 */
int _map_enter_debug = 0;

kern_return_t
vm_map_enter_mem_object(
	vm_map_t		target_map,
	vm_map_offset_t		*address,
	vm_map_size_t		initial_size,
	vm_map_offset_t		mask,
	int			flags,
	ipc_port_t		port,
	vm_object_offset_t	offset,
	boolean_t		copy,
	vm_prot_t		cur_protection,
	vm_prot_t		max_protection,
	vm_inherit_t		inheritance)
{
        kprintf("not implemented: vm_map_enter_mem_object()\n");
        return 0;
}



kern_return_t
vm_map_enter_mem_object_control(
	vm_map_t		target_map,
	vm_map_offset_t		*address,
	vm_map_size_t		initial_size,
	vm_map_offset_t		mask,
	int			flags,
	memory_object_control_t	control,
	vm_object_offset_t	offset,
	boolean_t		copy,
	vm_prot_t		cur_protection,
	vm_prot_t		max_protection,
	vm_inherit_t		inheritance)
{
        kprintf("not implemented: vm_map_enter_mem_object_control()\n");
        return 0;
}

#if	VM_CPM

#ifdef MACH_ASSERT
extern pmap_paddr_t	avail_start, avail_end;
#endif

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
vm_map_enter_cpm(
	vm_map_t		map,
	vm_map_offset_t	*addr,
	vm_map_size_t		size,
	int			flags)
{
        kprintf("not implemented: vm_map_enter_cpm()\n");
        return 0;
}

#else	/* VM_CPM */

/*
 *	Interface is defined in all cases, but unless the kernel
 *	is built explicitly for this option, the interface does
 *	nothing.
 */

kern_return_t
vm_map_enter_cpm(
	__unused vm_map_t	map,
	__unused vm_map_offset_t	*addr,
	__unused vm_map_size_t	size,
	__unused int		flags)
{
        kprintf("not implemented: vm_map_enter_cpm()\n");
        return 0;
}
#endif /* VM_CPM */

/* Not used without nested pmaps */
#ifndef NO_NESTED_PMAP
/*
 * Clip and unnest a portion of a nested submap mapping.
 */



#endif	/* NO_NESTED_PMAP */

/*
 *	vm_map_clip_start:	[ internal use only ]
 *
 *	Asserts that the given entry begins at or after
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
void
vm_map_clip_start(
	vm_map_t	map,
	vm_map_entry_t	entry,
	vm_map_offset_t	startaddr)
{
		kprintf("not implemented: vm_map_clip_start()\n");
}

#define vm_map_copy_clip_start(copy, entry, startaddr) \
	MACRO_BEGIN \
	if ((startaddr) > (entry)->vme_start) \
		_vm_map_clip_start(&(copy)->cpy_hdr,(entry),(startaddr)); \
	MACRO_END

/*
 *	This routine is called only when it is known that
 *	the entry must be split.
 */


/*
 *	vm_map_clip_end:	[ internal use only ]
 *
 *	Asserts that the given entry ends at or before
 *	the specified address; if necessary,
 *	it splits the entry into two.
 */
void
vm_map_clip_end(
	vm_map_t	map,
	vm_map_entry_t	entry,
	vm_map_offset_t	endaddr)
{
		kprintf("not implemented: vm_map_clip_end()\n");
}

#define vm_map_copy_clip_end(copy, entry, endaddr) \
	MACRO_BEGIN \
	if ((endaddr) < (entry)->vme_end) \
		_vm_map_clip_end(&(copy)->cpy_hdr,(entry),(endaddr)); \
	MACRO_END

/*
 *	This routine is called only when it is known that
 *	the entry must be split.
 */


/*
 *	VM_MAP_RANGE_CHECK:	[ internal use only ]
 *
 *	Asserts that the starting and ending region
 *	addresses fall within the valid range of the map.
 */
#define	VM_MAP_RANGE_CHECK(map, start, end)	\
	MACRO_BEGIN				\
	if (start < vm_map_min(map))		\
		start = vm_map_min(map);	\
	if (end > vm_map_max(map))		\
		end = vm_map_max(map);		\
	if (start > end)			\
		start = end;			\
	MACRO_END

/*
 *	vm_map_range_check:	[ internal use only ]
 *	
 *	Check that the region defined by the specified start and
 *	end addresses are wholly contained within a single map
 *	entry or set of adjacent map entries of the spacified map,
 *	i.e. the specified region contains no unmapped space.
 *	If any or all of the region is unmapped, FALSE is returned.
 *	Otherwise, TRUE is returned and if the output argument 'entry'
 *	is not NULL it points to the map entry containing the start
 *	of the region.
 *
 *	The map is locked for reading on entry and is left locked.
 */

/*
 *	vm_map_submap:		[ kernel use only ]
 *
 *	Mark the given range as handled by a subordinate map.
 *
 *	This range must have been created with vm_map_find using
 *	the vm_submap_object, and no other operations may have been
 *	performed on this range prior to calling vm_map_submap.
 *
 *	Only a limited number of operations can be performed
 *	within this rage after calling vm_map_submap:
 *		vm_fault
 *	[Don't try vm_map_copyin!]
 *
 *	To remove a submapping, one must first remove the
 *	range from the superior map, and then destroy the
 *	submap (if desired).  [Better yet, don't try it.]
 */
kern_return_t
vm_map_submap(
	vm_map_t		map,
	vm_map_offset_t	start,
	vm_map_offset_t	end,
	vm_map_t		submap,
	vm_map_offset_t	offset,
#ifdef NO_NESTED_PMAP
	__unused
#endif	/* NO_NESTED_PMAP */
	boolean_t		use_pmap)
{
        kprintf("not implemented: vm_map_submap()\n");
        return 0;
}
/*
 *	vm_map_protect:
 *
 *	Sets the protection of the specified address
 *	region in the target map.  If "set_max" is
 *	specified, the maximum protection is to be set;
 *	otherwise, only the current protection is affected.
 */
kern_return_t
vm_map_protect(
	register vm_map_t	map,
	register vm_map_offset_t	start,
	register vm_map_offset_t	end,
	register vm_prot_t	new_prot,
	register boolean_t	set_max)
{
        kprintf("not implemented: vm_map_protect()\n");
        return 0;
}
/*
 *	vm_map_inherit:
 *
 *	Sets the inheritance of the specified address
 *	range in the target map.  Inheritance
 *	affects how the map will be shared with
 *	child maps at the time of vm_map_fork.
 */
kern_return_t
vm_map_inherit(
	register vm_map_t	map,
	register vm_map_offset_t	start,
	register vm_map_offset_t	end,
	register vm_inherit_t	new_inheritance)
{
        kprintf("not implemented: vm_map_inherit()\n");
        return 0;
}
/*
 * Update the accounting for the amount of wired memory in this map.  If the user has
 * exceeded the defined limits, then we fail.  Wiring on behalf of the kernel never fails.
 */


/*
 * Update the memory wiring accounting now that the given map entry is being unwired.
 */


/*
 *	vm_map_wire:
 *
 *	Sets the pageability of the specified address range in the
 *	target map as wired.  Regions specified as not pageable require
 *	locked-down physical memory and physical page maps.  The
 *	access_type variable indicates types of accesses that must not
 *	generate page faults.  This is checked against protection of
 *	memory being locked-down.
 *
 *	The map must not be locked, but a reference must remain to the
 *	map throughout the call.
 */

/*
 *	vm_map_unwire:
 *
 *	Sets the pageability of the specified address range in the target
 *	as pageable.  Regions specified must have been wired previously.
 *
 *	The map must not be locked, but a reference must remain to the map
 *	throughout the call.
 *
 *	Kernel will panic on failures.  User unwire ignores holes and
 *	unwired and intransition entries to avoid losing memory by leaving
 *	it unwired.
 */


kern_return_t
vm_map_unwire(
        register vm_map_t       map,
        register vm_map_offset_t        start,
        register vm_map_offset_t        end,
        boolean_t               user_wire)
{
        kprintf("not implemented: vm_map_unwire()\n");
        return 0;
}

kern_return_t
vm_map_wire(
        register vm_map_t       map,
        register vm_map_offset_t        start,
        register vm_map_offset_t        end,
        register vm_prot_t      access_type,
        boolean_t               user_wire)
{
        kprintf("not implemented: vm_map_wire()\n");
        return 0;
}


/*
 *	vm_map_entry_delete:	[ internal use only ]
 *
 *	Deallocate the given entry from the target map.
 */		

/*
 *	vm_map_delete:	[ internal use only ]
 *
 *	Deallocates the given address range from the target map.
 *	Removes all user wirings. Unwires one kernel wiring if
 *	VM_MAP_REMOVE_KUNWIRE is set.  Waits for kernel wirings to go
 *	away if VM_MAP_REMOVE_WAIT_FOR_KWIRE is set.  Sleeps
 *	interruptibly if VM_MAP_REMOVE_INTERRUPTIBLE is set.
 *
 *	This routine is called with map locked and leaves map locked.
 */

/*
 *	vm_map_remove:
 *
 *	Remove the given address range from the target map.
 *	This is the exported form of vm_map_delete.
 */
kern_return_t
vm_map_remove(
	register vm_map_t	map,
	register vm_map_offset_t	start,
	register vm_map_offset_t	end,
	register boolean_t	flags)
{
        kprintf("not implemented: vm_map_remove()\n");
        return 0;
}



/*
 *	Routine:	vm_map_copy_copy
 *
 *	Description:
 *			Move the information in a map copy object to
 *			a new map copy object, leaving the old one
 *			empty.
 *
 *			This is used by kernel routines that need
 *			to look at out-of-line data (in copyin form)
 *			before deciding whether to return SUCCESS.
 *			If the routine returns FAILURE, the original
 *			copy object will be deallocated; therefore,
 *			these routines must make a copy of the copy
 *			object and leave the original empty so that
 *			deallocation will not fail.
 */
vm_map_copy_t
vm_map_copy_copy(
	vm_map_copy_t	copy)
{
        kprintf("not implemented: vm_map_copy_copy()\n");
        return 0;
}

/*
 *	Routine:	vm_map_copy_overwrite
 *
 *	Description:
 *		Copy the memory described by the map copy
 *		object (copy; returned by vm_map_copyin) onto
 *		the specified destination region (dst_map, dst_addr).
 *		The destination must be writeable.
 *
 *		Unlike vm_map_copyout, this routine actually
 *		writes over previously-mapped memory.  If the
 *		previous mapping was to a permanent (user-supplied)
 *		memory object, it is preserved.
 *
 *		The attributes (protection and inheritance) of the
 *		destination region are preserved.
 *
 *		If successful, consumes the copy object.
 *		Otherwise, the caller is responsible for it.
 *
 *	Implementation notes:
 *		To overwrite aligned temporary virtual memory, it is
 *		sufficient to remove the previous mapping and insert
 *		the new copy.  This replacement is done either on
 *		the whole region (if no permanent virtual memory
 *		objects are embedded in the destination region) or
 *		in individual map entries.
 *
 *		To overwrite permanent virtual memory , it is necessary
 *		to copy each page, as the external memory management
 *		interface currently does not provide any optimizations.
 *
 *		Unaligned memory also has to be copied.  It is possible
 *		to use 'vm_trickery' to copy the aligned data.  This is
 *		not done but not hard to implement.
 *
 *		Once a page of permanent memory has been overwritten,
 *		it is impossible to interrupt this function; otherwise,
 *		the call would be neither atomic nor location-independent.
 *		The kernel-state portion of a user thread must be
 *		interruptible.
 *
 *		It may be expensive to forward all requests that might
 *		overwrite permanent memory (vm_write, vm_copy) to
 *		uninterruptible kernel threads.  This routine may be
 *		called by interruptible threads; however, success is
 *		not guaranteed -- if the request cannot be performed
 *		atomically and interruptibly, an error indication is
 *		returned.
 */


kern_return_t
vm_map_copy_overwrite(
	vm_map_t	dst_map,
	vm_map_offset_t	dst_addr,
	vm_map_copy_t	copy,
	boolean_t	interruptible)
{
        kprintf("not implemented: vm_map_copy_overwrite()\n");
        return 0;
}

/*
 *	Routine: vm_map_copy_overwrite_unaligned	[internal use only]
 *
 *	Decription:
 *	Physically copy unaligned data
 *
 *	Implementation:
 *	Unaligned parts of pages have to be physically copied.  We use
 *	a modified form of vm_fault_copy (which understands none-aligned
 *	page offsets and sizes) to do the copy.  We attempt to copy as
 *	much memory in one go as possibly, however vm_fault_copy copies
 *	within 1 memory object so we have to find the smaller of "amount left"
 *	"source object data size" and "target object data size".  With
 *	unaligned data we don't need to split regions, therefore the source
 *	(copy) object should be one map entry, the target range may be split
 *	over multiple map entries however.  In any event we are pessimistic
 *	about these assumptions.
 *
 *	Assumptions:
 *	dst_map is locked on entry and is return locked on success,
 *	unlocked on error.
 */


/*
 *	Routine: vm_map_copy_overwrite_aligned	[internal use only]
 *
 *	Description:
 *	Does all the vm_trickery possible for whole pages.
 *
 *	Implementation:
 *
 *	If there are no permanent objects in the destination,
 *	and the source and destination map entry zones match,
 *	and the destination map entry is not shared,
 *	then the map entries can be deleted and replaced
 *	with those from the copy.  The following code is the
 *	basic idea of what to do, but there are lots of annoying
 *	little details about getting protection and inheritance
 *	right.  Should add protection, inheritance, and sharing checks
 *	to the above pass and make sure that no wiring is involved.
 */

int vm_map_copy_overwrite_aligned_src_not_internal = 0;
int vm_map_copy_overwrite_aligned_src_not_symmetric = 0;
int vm_map_copy_overwrite_aligned_src_large = 0;


/*
 *	Routine: vm_map_copyin_kernel_buffer [internal use only]
 *
 *	Description:
 *		Copy in data to a kernel buffer from space in the
 *		source map. The original space may be optionally
 *		deallocated.
 *
 *		If successful, returns a new copy object.
 */

/*
 *	Routine: vm_map_copyout_kernel_buffer	[internal use only]
 *
 *	Description:
 *		Copy out data from a kernel buffer into space in the
 *		destination map. The space may be otpionally dynamically
 *		allocated.
 *
 *		If successful, consumes the copy object.
 *		Otherwise, the caller is responsible for it.
 */
static int vm_map_copyout_kernel_buffer_failures = 0;


/*
 *	Macro:		vm_map_copy_insert
 *	
 *	Description:
 *		Link a copy chain ("copy") into a map at the
 *		specified location (after "where").
 *	Side effects:
 *		The copy chain is destroyed.
 *	Warning:
 *		The arguments are evaluated multiple times.
 */
#define	vm_map_copy_insert(map, where, copy)				\
MACRO_BEGIN								\
	vm_map_store_copy_insert(map, where, copy);	  \
	zfree(vm_map_copy_zone, copy);		\
MACRO_END



/*
 *	Routine:	vm_map_copyin
 *
 *	Description:
 *		see vm_map_copyin_common.  Exported via Unsupported.exports.
 *
 */

#undef vm_map_copyin

kern_return_t
vm_map_copyin(
	vm_map_t			src_map,
	vm_map_address_t	src_addr,
	vm_map_size_t		len,
	boolean_t			src_destroy,
	vm_map_copy_t		*copy_result)	/* OUT */
{
        kprintf("not implemented: vm_map_copyin()\n");
        return 0;
}




/*
 *	vm_map_copyin_object:
 *
 *	Create a copy object from an object.
 *	Our caller donates an object reference.
 */

kern_return_t
vm_map_copyin_object(
	vm_object_t		object,
	vm_object_offset_t	offset,	/* offset of region in object */
	vm_object_size_t	size,	/* size of region in object */
	vm_map_copy_t	*copy_result)	/* OUT */
{
        kprintf("not implemented: vm_map_copyin_object()\n");
        return 0;
}


/*
 * vm_map_exec:
 *
 * 	Setup the "new_map" with the proper execution environment according
 *	to the type of executable (platform, 64bit, chroot environment).
 *	Map the comm page and shared region, etc...
 */
kern_return_t
vm_map_exec(
	vm_map_t	new_map,
	task_t		task,
	void		*fsroot,
	cpu_type_t	cpu)
{
        kprintf("not implemented: vm_map_exec()\n");
        return 0;
}
/*
 *	vm_map_lookup_locked:
 *
 *	Finds the VM object, offset, and
 *	protection for a given virtual address in the
 *	specified map, assuming a page fault of the
 *	type specified.
 *
 *	Returns the (object, offset, protection) for
 *	this address, whether it is wired down, and whether
 *	this map has the only reference to the data in question.
 *	In order to later verify this lookup, a "version"
 *	is returned.
 *
 *	The map MUST be locked by the caller and WILL be
 *	locked on exit.  In order to guarantee the
 *	existence of the returned object, it is returned
 *	locked.
 *
 *	If a lookup is requested with "write protection"
 *	specified, the map may be changed to perform virtual
 *	copying operations, although the data referenced will
 *	remain the same.
 */
kern_return_t
vm_map_lookup_locked(
	vm_map_t		*var_map,	/* IN/OUT */
	vm_map_offset_t		vaddr,
	vm_prot_t		fault_type,
	int			object_lock_type,
	vm_map_version_t	*out_version,	/* OUT */
	vm_object_t		*object,	/* OUT */
	vm_object_offset_t	*offset,	/* OUT */
	vm_prot_t		*out_prot,	/* OUT */
	boolean_t		*wired,		/* OUT */
	vm_object_fault_info_t	fault_info,	/* OUT */
	vm_map_t		*real_map)
{
        kprintf("not implemented: vm_map_lookup_locked()\n");
        return 0;
}

/*
 *	vm_map_verify:
 *
 *	Verifies that the map in question has not changed
 *	since the given version.  If successful, the map
 *	will not change until vm_map_verify_done() is called.
 */
boolean_t
vm_map_verify(
	register vm_map_t		map,
	register vm_map_version_t	*version)	/* REF */
{
        kprintf("not implemented: vm_map_verify()\n");
        return 0;
}
/*
 *	vm_map_verify_done:
 *
 *	Releases locks acquired by a vm_map_verify.
 *
 *	This is now a macro in vm/vm_map.h.  It does a
 *	vm_map_unlock_read on the map.
 */


/*
 *	TEMPORARYTEMPORARYTEMPORARYTEMPORARYTEMPORARYTEMPORARY
 *	Goes away after regular vm_region_recurse function migrates to
 *	64 bits
 *	vm_region_recurse: A form of vm_region which follows the
 *	submaps in a target map
 *
 */

kern_return_t
vm_map_region_recurse_64(
	vm_map_t		 map,
	vm_map_offset_t	*address,		/* IN/OUT */
	vm_map_size_t		*size,			/* OUT */
	natural_t	 	*nesting_depth,	/* IN/OUT */
	vm_region_submap_info_64_t	submap_info,	/* IN/OUT */
	mach_msg_type_number_t	*count)	/* IN/OUT */
{
        kprintf("not implemented: vm_map_region_recurse_64()\n");
        return 0;
}
/*
 *	vm_region:
 *
 *	User call to obtain information about a region in
 *	a task's address map. Currently, only one flavor is
 *	supported.
 *
 *	XXX The reserved and behavior fields cannot be filled
 *	    in until the vm merge from the IK is completed, and
 *	    vm_reserve is implemented.
 */

kern_return_t
vm_map_region(
	vm_map_t		 map,
	vm_map_offset_t	*address,		/* IN/OUT */
	vm_map_size_t		*size,			/* OUT */
	vm_region_flavor_t	 flavor,		/* IN */
	vm_region_info_t	 info,			/* OUT */
	mach_msg_type_number_t	*count,	/* IN/OUT */
	mach_port_t		*object_name)		/* OUT */
{
        kprintf("not implemented: vm_map_region()\n");
        return 0;
}
#define OBJ_RESIDENT_COUNT(obj, entry_size)				\
	MIN((entry_size),						\
	    ((obj)->all_reusable ?					\
	     (obj)->wired_page_count :					\
	     (obj)->resident_page_count - (obj)->reusable_page_count))

void
vm_map_region_top_walk(
        vm_map_entry_t		   entry,
	vm_region_top_info_t       top)
{
		kprintf("not implemented: vm_map_region_top_walk()\n");
}

void
vm_map_region_walk(
	vm_map_t		   	map,
	vm_map_offset_t			va,
	vm_map_entry_t			entry,
	vm_object_offset_t		offset,
	vm_object_size_t		range,
	vm_region_extended_info_t	extended,
	boolean_t			look_for_pages)
{
		kprintf("not implemented: vm_map_region_walk()\n");
}

/* object is locked on entry and locked on return */





/*
 *	Routine:	vm_map_simplify
 *
 *	Description:
 *		Attempt to simplify the map representation in
 *		the vicinity of the given starting address.
 *	Note:
 *		This routine is intended primarily to keep the
 *		kernel maps more compact -- they generally don't
 *		benefit from the "expand a map entry" technology
 *		at allocation time because the adjacent entry
 *		is often wired down.
 */
void
vm_map_simplify_entry(
	vm_map_t	map,
	vm_map_entry_t	this_entry)
{
		kprintf("not implemented: vm_map_simplify_entry()\n");
}

void
vm_map_simplify(
	vm_map_t	map,
	vm_map_offset_t	start)
{
        kprintf("not implemented: vm_map_simplify()\n");
}
/*
 *	vm_map_behavior_set:
 *
 *	Sets the paging reference behavior of the specified address
 *	range in the target map.  Paging reference behavior affects
 *	how pagein operations resulting from faults on the map will be 
 *	clustered.
 */
kern_return_t 
vm_map_behavior_set(
	vm_map_t	map,
	vm_map_offset_t	start,
	vm_map_offset_t	end,
	vm_behavior_t	new_behavior)
{
        kprintf("not implemented: vm_map_behavior_set()\n");
        return 0;
}

/*
 * Internals for madvise(MADV_WILLNEED) system call.
 *
 * The present implementation is to do a read-ahead if the mapping corresponds
 * to a mapped regular file.  If it's an anonymous mapping, then we do nothing
 * and basically ignore the "advice" (which we are always free to do).
 */










/*
 *	Routine:	vm_map_entry_insert
 *
 *	Descritpion:	This routine inserts a new vm_entry in a locked map.
 */
vm_map_entry_t
vm_map_entry_insert(
	vm_map_t		map,
	vm_map_entry_t		insp_entry,
	vm_map_offset_t		start,
	vm_map_offset_t		end,
	vm_object_t		object,
	vm_object_offset_t	offset,
	boolean_t		needs_copy,
	boolean_t		is_shared,
	boolean_t		in_transition,
	vm_prot_t		cur_protection,
	vm_prot_t		max_protection,
	vm_behavior_t		behavior,
	vm_inherit_t		inheritance,
	unsigned		wired_count,
	boolean_t		no_cache,
	boolean_t		permanent,
	unsigned int		superpage_size)
{
        kprintf("not implemented: vm_map_entry_insert()\n");
        return 0;
}
/*
 *	Routine:	vm_map_remap_extract
 *
 *	Descritpion:	This routine returns a vm_entry list from a map.
 */

/*
 *	Routine:	vm_remap
 *
 *			Map portion of a task's address space.
 *			Mapped region must not overlap more than
 *			one vm memory object. Protections and
 *			inheritance attributes remain the same
 *			as in the original task and are	out parameters.
 *			Source and Target task can be identical
 *			Other attributes are identical as for vm_map()
 */
kern_return_t
vm_map_remap(
	vm_map_t		target_map,
	vm_map_address_t	*address,
	vm_map_size_t		size,
	vm_map_offset_t		mask,
	int			flags,
	vm_map_t		src_map,
	vm_map_offset_t		memory_address,
	boolean_t		copy,
	vm_prot_t		*cur_protection,
	vm_prot_t		*max_protection,
	vm_inherit_t		inheritance)
{
        kprintf("not implemented: vm_map_remap()\n");
        return 0;
}
/*
 *	Routine:	vm_map_remap_range_allocate
 *
 *	Description:
 *		Allocate a range in the specified virtual address map.
 *		returns the address and the map entry just before the allocated
 *		range
 *
 *	Map must be locked.
 */


/*
 *	vm_map_switch:
 *
 *	Set the address map for the current thread to the specified map
 */

vm_map_t
vm_map_switch(
	vm_map_t	map)
{
        kprintf("not implemented: vm_map_switch()\n");
        return 0;
}

/*
 *	Routine:	vm_map_write_user
 *
 *	Description:
 *		Copy out data from a kernel space into space in the
 *		destination map. The space must already exist in the
 *		destination map.
 *		NOTE:  This routine should only be called by threads
 *		which can block on a page fault. i.e. kernel mode user
 *		threads.
 *
 */
kern_return_t
vm_map_write_user(
	vm_map_t		map,
	void			*src_p,
	vm_map_address_t	dst_addr,
	vm_size_t		size)
{
        kprintf("not implemented: vm_map_write_user()\n");
        return 0;
}
/*
 *	Routine:	vm_map_read_user
 *
 *	Description:
 *		Copy in data from a user space source map into the
 *		kernel map. The space must already exist in the
 *		kernel map.
 *		NOTE:  This routine should only be called by threads
 *		which can block on a page fault. i.e. kernel mode user
 *		threads.
 *
 */
kern_return_t
vm_map_read_user(
	vm_map_t		map,
	vm_map_address_t	src_addr,
	void			*dst_p,
	vm_size_t		size)
{
        kprintf("not implemented: vm_map_read_user()\n");
        return 0;
}

/*
 *	vm_map_check_protection:
 *
 *	Assert that the target map allows the specified
 *	privilege on the entire address region given.
 *	The entire region must be allocated.
 */
boolean_t
vm_map_check_protection(vm_map_t map, vm_map_offset_t start,
			vm_map_offset_t end, vm_prot_t protection)
{
        kprintf("not implemented: vm_map_check_protection()\n");
        return 0;
}
kern_return_t
vm_map_purgable_control(
	vm_map_t		map,
	vm_map_offset_t		address,
	vm_purgable_t		control,
	int			*state)
{
        kprintf("not implemented: vm_map_purgable_control()\n");
        return 0;
}
kern_return_t
vm_map_page_query_internal(
	vm_map_t	target_map,
	vm_map_offset_t	offset,
	int		*disposition,
	int		*ref_count)
{
        kprintf("not implemented: vm_map_page_query_internal()\n");
        return 0;
}
kern_return_t
vm_map_page_info(
	vm_map_t		map,
	vm_map_offset_t		offset,
	vm_page_info_flavor_t	flavor,
	vm_page_info_t		info,
	mach_msg_type_number_t	*count)
{
        kprintf("not implemented: vm_map_page_info()\n");
        return 0;
}
/*
 *	vm_map_msync
 *
 *	Synchronises the memory range specified with its backing store
 *	image by either flushing or cleaning the contents to the appropriate
 *	memory manager engaging in a memory object synchronize dialog with
 *	the manager.  The client doesn't return until the manager issues
 *	m_o_s_completed message.  MIG Magically converts user task parameter
 *	to the task's address map.
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
 *	NOTE
 *	The memory object attributes have not yet been implemented, this
 *	function will have to deal with the invalidate attribute
 *
 *	RETURNS
 *	KERN_INVALID_TASK		Bad task parameter
 *	KERN_INVALID_ARGUMENT		both sync and async were specified.
 *	KERN_SUCCESS			The usual.
 *	KERN_INVALID_ADDRESS		There was a hole in the region.
 */

kern_return_t
vm_map_msync(
	vm_map_t		map,
	vm_map_address_t	address,
	vm_map_size_t		size,
	vm_sync_t		sync_flags)
{
        kprintf("not implemented: vm_map_msync()\n");
        return 0;
}
/*
 *	Routine:	convert_port_entry_to_map
 *	Purpose:
 *		Convert from a port specifying an entry or a task
 *		to a map. Doesn't consume the port ref; produces a map ref,
 *		which may be null.  Unlike convert_port_to_map, the
 *		port may be task or a named entry backed.
 *	Conditions:
 *		Nothing locked.
 */


vm_map_t
convert_port_entry_to_map(
	ipc_port_t	port)
{
        kprintf("not implemented: convert_port_entry_to_map()\n");
        return 0;
}
/*
 *	Routine:	convert_port_entry_to_object
 *	Purpose:
 *		Convert from a port specifying a named entry to an
 *		object. Doesn't consume the port ref; produces a map ref,
 *		which may be null. 
 *	Conditions:
 *		Nothing locked.
 */


vm_object_t
convert_port_entry_to_object(
	ipc_port_t	port)
{
        kprintf("not implemented: convert_port_entry_to_object()\n");
        return 0;
}
/*
 * Export routines to other components for the things we access locally through
 * macros.
 */
#undef current_map
vm_map_t
current_map(void)
{
        kprintf("not implemented: current_map()\n");
        return 0;
}
/*
 *	vm_map_reference:
 *
 *	Most code internal to the osfmk will go through a
 *	macro defining this.  This is always here for the
 *	use of other kernel components.
 */
#undef vm_map_reference
void
vm_map_reference(
	register vm_map_t	map)
{
		kprintf("not implemented: vm_map_reference()\n");
}
/*
 *	vm_map_deallocate:
 *
 *	Removes a reference from the specified map,
 *	destroying it if no references remain.
 *	The map should not be locked.
 */
void
vm_map_deallocate(
	register vm_map_t	map)
{
		kprintf("not implemented: vm_map_deallocate()\n");
}

void
vm_map_disable_NX(vm_map_t map)
{
		kprintf("not implemented: vm_map_disable_NX()\n");
}
void
vm_map_disallow_data_exec(vm_map_t map)
{
		kprintf("not implemented: vm_map_disallow_data_exec()\n");
}
/* XXX Consider making these constants (VM_MAX_ADDRESS and MACH_VM_MAX_ADDRESS)
 * more descriptive.
 */
void
vm_map_set_32bit(vm_map_t map)
{
		kprintf("not implemented: vm_map_set_32bit()\n");
}

void
vm_map_set_64bit(vm_map_t map)
{
		kprintf("not implemented: vm_map_set_64bit()\n");
}
vm_map_offset_t
vm_compute_max_offset(unsigned is64)
{
        kprintf("not implemented: vm_compute_max_offset()\n");
        return 0;
}
boolean_t
vm_map_is_64bit(
		vm_map_t map)
{
        kprintf("not implemented: vm_map_is_64bit()\n");
        return 0;
}
boolean_t
vm_map_has_hard_pagezero(
		vm_map_t 	map,
		vm_map_offset_t	pagezero_size)
{
        kprintf("not implemented: vm_map_has_hard_pagezero()\n");
        return 0;
}
void
vm_map_set_4GB_pagezero(vm_map_t map)
{
		kprintf("not implemented: vm_map_set_4GB_pagezero()\n");
}
/*
 * Raise a VM map's maximun offset.
 */
kern_return_t
vm_map_raise_max_offset(
	vm_map_t	map,
	vm_map_offset_t	new_max_offset)
{
        kprintf("not implemented: vm_map_raise_max_offset()\n");
        return 0;
}

/*
 * Raise a VM map's minimum offset.
 * To strictly enforce "page zero" reservation.
 */
kern_return_t
vm_map_raise_min_offset(
	vm_map_t	map,
	vm_map_offset_t	new_min_offset)
{
        kprintf("not implemented: vm_map_raise_min_offset()\n");
        return 0;
}
/*
 * Set the limit on the maximum amount of user wired memory allowed for this map.
 * This is basically a copy of the MEMLOCK rlimit value maintained by the BSD side of
 * the kernel.  The limits are checked in the mach VM side, so we keep a copy so we
 * don't have to reach over to the BSD data structures.
 */

void
vm_map_set_user_wire_limit(vm_map_t 	map,
			   vm_size_t	limit)
{
		kprintf("not implemented: vm_map_set_user_wire_limit()\n");
}

void vm_map_switch_protect(vm_map_t	map, 
			   boolean_t	val) 
{
		kprintf("not implemented: vm_map_switch_protect()\n");
}
/* Add (generate) code signature for memory range */
#if CONFIG_DYNAMIC_CODE_SIGNING
kern_return_t vm_map_sign(vm_map_t map, 
		 vm_map_offset_t start, 
		 vm_map_offset_t end)
{
        kprintf("not implemented: vm_map_sign()\n");
        return 0;
}
#endif

#if CONFIG_FREEZE

kern_return_t vm_map_freeze_walk(
             	vm_map_t map,
             	unsigned int *purgeable_count,
             	unsigned int *wired_count,
             	unsigned int *clean_count,
             	unsigned int *dirty_count,
             	unsigned int  dirty_budget,
             	boolean_t *has_shared)
{
        kprintf("not implemented: vm_map_freeze_walk()\n");
        return 0;
}
kern_return_t vm_map_freeze(
             	vm_map_t map,
             	unsigned int *purgeable_count,
             	unsigned int *wired_count,
             	unsigned int *clean_count,
             	unsigned int *dirty_count,
             	unsigned int dirty_budget,
             	boolean_t *has_shared)
{
        kprintf("not implemented: vm_map_freeze()\n");
        return 0;
}

kern_return_t
vm_map_thaw(
	vm_map_t map)
{
        kprintf("not implemented: vm_map_thaw()\n");
        return 0;
}
#endif

#if !CONFIG_EMBEDDED
/*
 * vm_map_entry_should_cow_for_true_share:
 *
 * Determines if the map entry should be clipped and setup for copy-on-write
 * to avoid applying "true_share" to a large VM object when only a subset is
 * targeted.
 *
 * For now, we target only the map entries created for the Objective C
 * Garbage Collector, which initially have the following properties:
 *	- alias == VM_MEMORY_MALLOC
 * 	- wired_count == 0
 * 	- !needs_copy
 * and a VM object with:
 * 	- internal
 * 	- copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC
 * 	- !true_share
 * 	- vo_size == ANON_CHUNK_SIZE
 */
boolean_t
vm_map_entry_should_cow_for_true_share(
	vm_map_entry_t	entry)
{
        kprintf("not implemented: vm_map_entry_should_cow_for_true_share()\n");
        return 0;
}

#endif /* !CONFIG_EMBEDDED */
