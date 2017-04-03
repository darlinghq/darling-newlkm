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

#ifndef DUCT_IPC_KERNELRPC_H
#define DUCT_IPC_KERNELRPC_H

#include <mach/mach_types.h>
#include <mach/mach_traps.h>

#define XNU_USE_MACHTRAP_WRAPPERS_KERNELRPC

#if defined (XNU_USE_MACHTRAP_WRAPPERS_KERNELRPC)
extern kern_return_t xnusys__kernelrpc_mach_vm_allocate_trap (struct _kernelrpc_mach_vm_allocate_trap_args * args);
extern kern_return_t xnusys__kernelrpc_mach_vm_deallocate_trap (struct _kernelrpc_mach_vm_deallocate_args * args);
extern kern_return_t xnusys__kernelrpc_mach_vm_protect_trap (struct _kernelrpc_mach_vm_protect_args * args);

extern kern_return_t xnusys__kernelrpc_mach_port_allocate_trap (struct _kernelrpc_mach_port_allocate_args * args);
extern kern_return_t xnusys__kernelrpc_mach_port_destroy_trap (struct _kernelrpc_mach_port_destroy_args * args);
extern kern_return_t xnusys__kernelrpc_mach_port_deallocate_trap (struct _kernelrpc_mach_port_deallocate_args * args);
extern kern_return_t xnusys__kernelrpc_mach_port_mod_refs_trap (struct _kernelrpc_mach_port_mod_refs_args * args);
extern kern_return_t xnusys__kernelrpc_mach_port_move_member_trap (struct _kernelrpc_mach_port_move_member_args * args);

extern kern_return_t xnusys__kernelrpc_mach_port_insert_right_trap (struct _kernelrpc_mach_port_insert_right_args * args);
extern kern_return_t xnusys__kernelrpc_mach_port_insert_member_trap (struct _kernelrpc_mach_port_insert_member_args * args);
extern kern_return_t xnusys__kernelrpc_mach_port_extract_member_trap (struct _kernelrpc_mach_port_extract_member_args * args);

#else
#define xnusys__kernelrpc_mach_vm_allocate_trap             _kernelrpc_mach_vm_allocate_trap
#define xnusys__kernelrpc_mach_vm_deallocate_trap           _kernelrpc_mach_vm_deallocate_trap
#define xnusys__kernelrpc_mach_vm_protect_trap              _kernelrpc_mach_vm_protect_trap

#define xnusys__kernelrpc_mach_port_allocate_trap           _kernelrpc_mach_port_allocate_trap
#define xnusys__kernelrpc_mach_port_destroy_trap            _kernelrpc_mach_port_destroy_trap
#define xnusys__kernelrpc_mach_port_deallocate_trap         _kernelrpc_mach_port_deallocate_trap
#define xnusys__kernelrpc_mach_port_mod_refs_trap           _kernelrpc_mach_port_mod_refs_trap
#define xnusys__kernelrpc_mach_port_move_member_trap        _kernelrpc_mach_port_move_member_trap

#define xnusys__kernelrpc_mach_port_insert_right_trap       _kernelrpc_mach_port_insert_right_trap
#define xnusys__kernelrpc_mach_port_insert_member_trap      _kernelrpc_mach_port_insert_member_trap
#define xnusys__kernelrpc_mach_port_extract_member_trap     _kernelrpc_mach_port_extract_member_trap

#endif

#endif // DUCT_IPC_KERNELRPC_H
