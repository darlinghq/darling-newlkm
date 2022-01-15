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

#ifndef DUCT_VM_MAP_H
#define DUCT_VM_MAP_H

#include <mach/mach_types.h>
#include <mach/vm_types.h>

extern void duct_vm_map_init (void);

/* WC - different interface */
extern vm_map_t duct_vm_map_create (struct task_struct * linux_task);

extern kern_return_t duct_vm_map_copyin_common ( vm_map_t src_map, vm_map_address_t src_addr, vm_map_size_t len,
                                                 boolean_t src_destroy, boolean_t src_volatile,
                                                 vm_map_copy_t * copy_result, boolean_t use_maxprot );
extern kern_return_t vm_map_copyout (vm_map_t dst_map, vm_map_address_t* dst_addr, vm_map_copy_t copy);

extern void duct_vm_map_copy_discard (vm_map_copy_t copy);

extern kern_return_t duct_vm_map_copyout (vm_map_t dst_map, vm_map_address_t * dst_addr,vm_map_copy_t copy);

extern int darling_is_task_64bit(void);

#endif // DUCT_VM_MAP_H
