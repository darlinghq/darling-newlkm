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
#include "duct_ipc_kernelrpc.h"

// #include "duct_post_xnu.h"

#if defined (XNU_USE_MACHTRAP_WRAPPERS_KERNELRPC)
kern_return_t xnusys__kernelrpc_mach_vm_allocate_trap (struct _kernelrpc_mach_vm_allocate_trap_args * args)
{
        printk (KERN_NOTICE "- args->target: 0x%x, addr: 0x%x, size: 0x%x, flags: 0x%x\n", args->target, args->addr, args->size, args->flags);

        mach_vm_offset_t        addr;
        if (! copy_from_user (&addr, (const void __user *) args->addr, sizeof(addr))) {
                printk (KERN_NOTICE "- copyin addr: 0x%x\n", addr);
        }

        return _kernelrpc_mach_vm_allocate_trap (args);
}

kern_return_t xnusys__kernelrpc_mach_vm_deallocate_trap (struct _kernelrpc_mach_vm_deallocate_args * args)
{
        printk (KERN_NOTICE "- args->target: 0x%x, address: 0x%x, size: %d", args->target, args->address, args->size);
        return _kernelrpc_mach_vm_deallocate_trap (args);
}

kern_return_t xnusys__kernelrpc_mach_vm_protect_trap (struct _kernelrpc_mach_vm_protect_args * args)
{
        printk ( KERN_NOTICE "- args->target: 0x%x, address: 0x%x, size: %d, set_maximum: %d, new_protection: 0x%x\n", 
                 args->target, args->address, args->size, args->set_maximum, args->new_protection );
        return _kernelrpc_mach_vm_protect_trap (args);
}





kern_return_t xnusys__kernelrpc_mach_port_allocate_trap (struct _kernelrpc_mach_port_allocate_args * args)
{
        printk (KERN_NOTICE "- args->target: 0x%x, right: 0x%x, name: 0x%x\n", args->target, args->right, args->name);
        return _kernelrpc_mach_port_allocate_trap (args);
}

kern_return_t xnusys__kernelrpc_mach_port_destroy_trap (struct _kernelrpc_mach_port_destroy_args * args)
{
        printk (KERN_NOTICE "- args->target: 0x%x, name: 0x%x\n", args->target, args->name);
        return _kernelrpc_mach_port_destroy_trap (args);
}

kern_return_t xnusys__kernelrpc_mach_port_deallocate_trap (struct _kernelrpc_mach_port_deallocate_args * args)
{
        printk (KERN_NOTICE "- args->target: 0x%x, name: 0x%x\n", args->target, args->name);
        return _kernelrpc_mach_port_deallocate_trap (args);
}


kern_return_t xnusys__kernelrpc_mach_port_mod_refs_trap (struct _kernelrpc_mach_port_mod_refs_args * args)
{
        printk (KERN_NOTICE "- args->target: 0x%x, name: 0x%x, right: 0x%x, delta: 0x%x\n", args->target, args->name, args->right, args->delta);
        return _kernelrpc_mach_port_mod_refs_trap (args);
}

kern_return_t xnusys__kernelrpc_mach_port_move_member_trap (struct _kernelrpc_mach_port_move_member_args * args)
{
        printk (KERN_NOTICE "- args->target: 0x%x, member: 0x%x, after: 0x%x\n", args->target, args->member, args->after);
        return _kernelrpc_mach_port_move_member_trap (args);
}



kern_return_t xnusys__kernelrpc_mach_port_insert_right_trap (struct _kernelrpc_mach_port_insert_right_args * args)
{
        printk (KERN_NOTICE "- args->target: 0x%x, name: 0x%x, poly: 0x%x, polyPoly: 0x%x\n", args->target, args->name, args->poly, args->polyPoly);
        return _kernelrpc_mach_port_insert_right_trap (args);
}

kern_return_t xnusys__kernelrpc_mach_port_insert_member_trap (struct _kernelrpc_mach_port_insert_member_args * args)
{
        printk (KERN_NOTICE "- args->target: 0x%x, name: 0x%x, pset: 0x%x\n", args->target, args->name, args->pset);
        return _kernelrpc_mach_port_insert_member_trap (args);
}

kern_return_t xnusys__kernelrpc_mach_port_extract_member_trap (struct _kernelrpc_mach_port_extract_member_args * args)
{
        printk (KERN_NOTICE "- args->target: 0x%x, name: 0x%x, pset: 0x%x\n", args->target, args->name, args->pset);
        return _kernelrpc_mach_port_extract_member_trap (args);
}
#endif
