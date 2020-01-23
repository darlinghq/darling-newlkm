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
 *  File:   vm/vm_kern.c
 *  Author: Avadis Tevanian, Jr., Michael Wayne Young
 *  Date:   1985
 *
 *  Kernel memory management.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <mach/kern_return.h>
#include <mach/vm_param.h>
#include <kern/assert.h>
#include <kern/lock.h>
#include <kern/thread.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <kern/misc_protos.h>
#include <vm/cpm.h>

#include <string.h>

#include <libkern/OSDebug.h>
#include <sys/kdebug.h>

/*
 *  Variables exported by this module.
 */

vm_map_t    kernel_map;
vm_map_t    kernel_pageable_map;

vm_offset_t vm_kernel_slid_base = 0;
vm_offset_t vm_kernel_slid_top = 0;

extern boolean_t vm_kernel_ready;

/*
 * Forward declarations for internal functions.
 */
extern kern_return_t kmem_alloc_pages(
    register vm_object_t        object,
    register vm_object_offset_t offset,
    register vm_object_size_t   size);

extern void kmem_remap_pages(
    register vm_object_t        object,
    register vm_object_offset_t offset,
    register vm_offset_t        start,
    register vm_offset_t        end,
    vm_prot_t           protection);

kern_return_t kmem_alloc_contig(
                vm_map_t    map,
                vm_offset_t *addrp,
                vm_size_t   size,
                vm_offset_t     mask,
                ppnum_t     max_pnum,
                ppnum_t     pnum_mask,
                int         flags,
                vm_tag_t        tag)
{
        kprintf("not implemented: kmem_alloc_contig()\n");
        return 0;
}

/*
 * Master entry point for allocating kernel memory.
 * NOTE: this routine is _never_ interrupt safe.
 *
 * map      : map to allocate into
 * addrp    : pointer to start address of new memory
 * size     : size of memory requested
 * flags    : options
 *        KMA_HERE      *addrp is base address, else "anywhere"
 *        KMA_NOPAGEWAIT    don't wait for pages if unavailable
 *        KMA_KOBJECT       use kernel_object
 *        KMA_LOMEM     support for 32 bit devices in a 64 bit world
 *                  if set and a lomemory pool is available
 *                  grab pages from it... this also implies
 *                  KMA_NOPAGEWAIT
 */

/*
kern_return_t    kernel_memory_allocate(
                vm_map_t    map,
                vm_offset_t *addrp,
                vm_size_t   size,
                vm_offset_t mask,
                int     flags,
                vm_tag_t        tag)
{
        kprintf("not implemented: kernel_memory_allocate()\n");
        return 0;
}
*/

/*
 *  kmem_alloc:
 *
 *  Allocate wired-down memory in the kernel's address map
 *  or a submap.  The memory is not zero-filled.
 */

/*
kern_return_t
kmem_alloc(
    vm_map_t    map,
    vm_offset_t *addrp,
    vm_size_t   size)
{
        kprintf("not implemented: kmem_alloc()\n");
        return 0;
}
*/

/*
 *  kmem_realloc:
 *
 *  Reallocate wired-down memory in the kernel's address map
 *  or a submap.  Newly allocated pages are not zeroed.
 *  This can only be used on regions allocated with kmem_alloc.
 *
 *  If successful, the pages in the old region are mapped twice.
 *  The old region is unchanged.  Use kmem_free to get rid of it.
 */
kern_return_t    kmem_realloc(
                vm_map_t    map,
                vm_offset_t oldaddr,
                vm_size_t   oldsize,
                vm_offset_t *newaddrp,
                vm_size_t   newsize,
                vm_tag_t        tag)
{
        kprintf("not implemented: kmem_realloc()\n");
        return 0;
}

/*
 *  kmem_alloc_kobject:
 *
 *  Allocate wired-down memory in the kernel's address map
 *  or a submap.  The memory is not zero-filled.
 *
 *  The memory is allocated in the kernel_object.
 *  It may not be copied with vm_map_copy, and
 *  it may not be reallocated with kmem_realloc.
 */

kern_return_t    kmem_alloc_kobject(
                vm_map_t    map,
                vm_offset_t *addrp,
                vm_size_t   size,
                vm_tag_t        tag)
{
        kprintf("not implemented: kmem_alloc_kobject()\n");
        return 0;
}

/*
 *  kmem_alloc_aligned:
 *
 *  Like kmem_alloc_kobject, except that the memory is aligned.
 *  The size should be a power-of-2.
 */

kern_return_t    kmem_alloc_aligned(
                vm_map_t    map,
                vm_offset_t *addrp,
                vm_size_t   size,
                vm_tag_t        tag)
{
        kprintf("not implemented: kmem_alloc_aligned()\n");
        return 0;
}

/*
 *  kmem_alloc_pageable:
 *
 *  Allocate pageable memory in the kernel's address map.
 */

kern_return_t    kmem_alloc_pageable(
                vm_map_t    map,
                vm_offset_t *addrp,
                vm_size_t   size,
                vm_tag_t        tag)
{
        kprintf("not implemented: kmem_alloc_pageable()\n");
        return 0;
}

/*
 *  kmem_free:
 *
 *  Release a region of kernel virtual memory allocated
 *  with kmem_alloc, kmem_alloc_kobject, or kmem_alloc_pageable,
 *  and return the physical pages associated with that region.
 */

// void
// kmem_free(
//     vm_map_t    map,
//     vm_offset_t addr,
//     vm_size_t   size)
// {
//         kprintf("not implemented: kmem_free()\n");
// }

/*
 *  Allocate new pages in an object.
 */

kern_return_t
kmem_alloc_pages(
    register vm_object_t        object,
    register vm_object_offset_t offset,
    register vm_object_size_t   size)
{
        kprintf("not implemented: kmem_alloc_pages()\n");
        return 0;
}

/*
 *  Remap wired pages in an object into a new region.
 *  The object is assumed to be mapped into the kernel map or
 *  a submap.
 */
void
kmem_remap_pages(
    register vm_object_t        object,
    register vm_object_offset_t offset,
    register vm_offset_t        start,
    register vm_offset_t        end,
    vm_prot_t           protection)
{
        kprintf("not implemented: kmem_remap_pages()\n");
}

/*
 *  kmem_suballoc:
 *
 *  Allocates a map to manage a subrange
 *  of the kernel virtual address space.
 *
 *  Arguments are as follows:
 *
 *  parent      Map to take range from
 *  addr        Address of start of range (IN/OUT)
 *  size        Size of range to find
 *  pageable    Can region be paged
 *  anywhere    Can region be located anywhere in map
 *  new_map     Pointer to new submap
 */
kern_return_t
kmem_suballoc(
    vm_map_t    parent,
    vm_offset_t *addr,
    vm_size_t   size,
    boolean_t   pageable,
    int     flags,
    vm_map_t    *new_map)
{
        kprintf("not implemented: kmem_suballoc()\n");
        return 0;
}

/*
 *  kmem_init:
 *
 *  Initialize the kernel's virtual memory map, taking
 *  into account all memory allocated up to this time.
 */
void
kmem_init(
    vm_offset_t start,
    vm_offset_t end)
{
        kprintf("not implemented: kmem_init()\n");
}



kern_return_t
vm_conflict_check(
    vm_map_t        map,
    vm_map_offset_t off,
    vm_map_size_t       len,
    memory_object_t pager,
    vm_object_offset_t  file_off)
{
        kprintf("not implemented: vm_conflict_check()\n");
        return 0;
}
