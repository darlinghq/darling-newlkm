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
#include "duct_ipc_pset.h"

#include <mach/port.h>
#include <mach/kern_return.h>
#include <mach/message.h>

#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_right.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_port.h>

#include <kern/kern_types.h>
#include <kern/spl.h>

// bsd
#include <sys/errno.h>
// #include <sys/event.h>

#if 0
kern_return_t duct_filt_machport_attach (struct compat_knote * kn, xnu_wait_queue_t * waitqp)
{
        printk (KERN_NOTICE "- duct_filt_machport_attach () called\n");
        mach_port_name_t        name    = (mach_port_name_t) kn->kevent.ident;
        // wait_queue_link_t       wql     = wait_queue_link_allocate();
        ipc_pset_t              pset    = IPS_NULL;
        // int                     result  = -LINUX_ENOSYS;

        // printk (KERN_NOTICE "duct_filt_machport_attach called on name: 0x%x\n", name);

        kern_return_t       kr  =
        ipc_object_translate (current_space (), name, MACH_PORT_RIGHT_PORT_SET, (ipc_object_t *) &pset);

        if (kr != KERN_SUCCESS)   return kr;
        /* We've got a lock on pset */

        *waitqp     = &(pset->ips_messages.imq_set_queue.wqs_wait_queue);

        ips_reference (pset);
        ips_unlock (pset);
        return kr;
}
#endif

