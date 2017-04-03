/*
Copyright (c) 2014-2017, Wenqi Chen

Shanghai Mifu Infotech Co., Ltd
B112-113, IF Industrial Park, 508 Chunking Road, Shanghai 201103, China


All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


*/

#include "duct.h"
#include "duct_pre_xnu.h"
#include "duct_vm_user.h"

#include <vm/vm_map.h>

#define BAD_ADDR(x)     ((unsigned long)(x) >= TASK_SIZE)


kern_return_t duct_mach_vm_allocate (vm_map_t map, mach_vm_offset_t * addr, mach_vm_size_t size, int flags)
{
        assert (current_map () == map);
        vm_map_offset_t map_addr;
        vm_map_size_t    map_size;
        kern_return_t    result;
        boolean_t    anywhere;

        /* filter out any kernel-only flags */
           if (flags & ~VM_FLAGS_USER_ALLOCATE)
            return KERN_INVALID_ARGUMENT;

#if defined (__DARLING__)
#else
        if (map == VM_MAP_NULL)
            return(KERN_INVALID_ARGUMENT);
#endif

        if (size == 0) {
            *addr = 0;
            return(KERN_SUCCESS);
        }

        anywhere = ((VM_FLAGS_ANYWHERE & flags) != 0);

        if (anywhere) {
            /*
             * No specific address requested, so start candidate address
             * search at the minimum address in the map.  However, if that
             * minimum is 0, bump it up by PAGE_SIZE.  We want to limit
             * allocations of PAGEZERO to explicit requests since its
             * normal use is to catch dereferences of NULL and many
             * applications also treat pointers with a value of 0 as
             * special and suddenly having address 0 contain useable
             * memory would tend to confuse those applications.
             */
#if defined (__DARLING__)
            map_addr    = *addr;
#else
            map_addr = vm_map_min(map);
#endif
            if (map_addr == 0)
                map_addr += PAGE_SIZE;

        } else
            map_addr = vm_map_trunc_page(*addr);

        map_size = vm_map_round_page(size);
        if (map_size == 0) {
          return(KERN_INVALID_ARGUMENT);
        }

#if defined (__DARLING__)

        vm_prot_t       vm_prot         = VM_PROT_DEFAULT;

        unsigned long   map_prot        = 0;

        /* standard conversion */
        if (vm_prot & VM_PROT_READ)         map_prot   |= PROT_READ;
        if (vm_prot & VM_PROT_WRITE)        map_prot   |= PROT_WRITE;
        if (vm_prot & VM_PROT_EXECUTE)      map_prot   |= PROT_EXEC;

        int             map_flags       = MAP_PRIVATE | MAP_ANONYMOUS;

        if (! flags & VM_FLAGS_ANYWHERE)    map_flags  |= MAP_FIXED;

        down_write (&linux_current->mm->mmap_sem);

        /* WC - todo: should potentially be just a vm_brk with prot flags */
        // printk (KERN_NOTICE "mmap: requested addr: 0x%x\n", map_addr);

        map_addr    =
        do_mmap (NULL, map_addr, map_size, map_prot, map_flags, 0, 0, NULL);

        // printk ( KERN_NOTICE "mmap: returned addr: 0x%x, bad: %d\n", 
        //          map_addr, BAD_ADDR (map_addr) ? 1 : 0 );

        result      = BAD_ADDR (map_addr) ? KERN_NO_SPACE : KERN_SUCCESS;

        // printk (KERN_NOTICE "mmap: result: 0x%x\n", result);


        up_write (&linux_current->mm->mmap_sem);


#else
        result = vm_map_enter(
                map,
                &map_addr,
                map_size,
                (vm_map_offset_t)0,
                flags,
                VM_OBJECT_NULL,
                (vm_object_offset_t)0,
                FALSE,
                VM_PROT_DEFAULT,
                VM_PROT_ALL,
                VM_INHERIT_DEFAULT);
#endif

        *addr = map_addr;
        return(result);
}
