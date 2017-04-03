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

kern_return_t duct_filt_machport_process (struct compat_knote * kn)
{
        printk (KERN_NOTICE "- duct_ipc_mqueue_receive_on_thread () called\n");
        mach_port_name_t        name        = (mach_port_name_t)kn->kevent.ident;
        ipc_pset_t              pset        = IPS_NULL;
        wait_result_t           wresult;
        thread_t                self        = current_thread ();

        mach_msg_option_t       option      = 0;
        mach_msg_size_t         size        = 0;

        /*
         * called from user context. Have to validate the
         * name.  If it changed, we have an EOF situation.
         */
        kern_return_t       kr  =
        ipc_object_translate (current_space (), name, MACH_PORT_RIGHT_PORT_SET, (ipc_object_t *) &pset);

        if (kr != KERN_SUCCESS || /* pset != kn->kn_ptr.p_pset || */ !ips_active (pset)) {
                printk (KERN_NOTICE "- ipc_object_translate return not KERN_SUCCESS\n");

                kn->kevent.data     = 0;
                kn->kevent.flags   |= (EV_EOF | EV_ONESHOT);

                if (pset != IPS_NULL) {
                        ips_unlock (pset);
                }
                return kr;
        }

        /* just use the reference from here on out */
        ips_reference (pset);
        ips_unlock (pset);

        /*
         * Only honor supported receive options. If no options are
         * provided, just force a MACH_RCV_TOO_LARGE to detect the
         * name of the port and sizeof the waiting message.
         */
        option  = kn->saved_fflags & (MACH_RCV_MSG|MACH_RCV_LARGE|MACH_RCV_TRAILER_MASK);
        if (option & MACH_RCV_MSG) {
        //     self->ith_msg_addr = (mach_vm_address_t) kn->kn_ext[0];
        //     size = (mach_msg_size_t)kn->kn_ext[1];
        } 
        else {
            option              = MACH_RCV_LARGE;
            self->ith_msg_addr  = 0;
            size                = 0;
        }

        // option |= MACH_RCV_LARGE;

        /*
         * Set up to receive a message or the notification of a
         * too large message.  But never allow this call to wait.
         * If the user provided aditional options, like trailer
         * options, pass those through here.  But we don't support
         * scatter lists through this interface.
         */
        self->ith_object                = (ipc_object_t) pset;
        self->ith_msize                 = size;
        self->ith_option                = option;
        self->ith_scatter_list_size     = 0;
        self->ith_receiver_name         = MACH_PORT_NULL;
        self->ith_continuation          = NULL;
        option |= MACH_RCV_TIMEOUT; // never wait
        assert ((self->ith_state = MACH_RCV_IN_PROGRESS) == MACH_RCV_IN_PROGRESS);

        wresult = ipc_mqueue_receive_on_thread (
                &pset->ips_messages,
                option,
                size, /* max_size */
                0, /* immediate timeout */
                THREAD_INTERRUPTIBLE,
                self);

        printk (KERN_NOTICE "- ipc_mqueue_receive_on_thread retval: %d\n", wresult);
        assert (wresult == THREAD_NOT_WAITING);
        assert (self->ith_state != MACH_RCV_IN_PROGRESS);

        // CPH, copy out ith_receiver_name
        kn->kevent.data   = self->ith_receiver_name;

        /*
         * If we timed out, just release the reference on the
         * portset and return zero.
         */
        // if (self->ith_state == MACH_RCV_TIMED_OUT) {
        //     ips_release(pset);
        //     return 0;
        // }

        /*
         * If we weren't attempting to receive a message
         * directly, we need to return the port name in
         * the kevent structure.
         */
        // if ((option & MACH_RCV_MSG) != MACH_RCV_MSG) {
        //     assert(self->ith_state == MACH_RCV_TOO_LARGE);
        //     assert(self->ith_kmsg == IKM_NULL);
        //     kn->kn_data = self->ith_receiver_name;
        //     ips_release(pset);
        //     return 1;
        // }

        /*
         * Attempt to receive the message directly, returning
         * the results in the fflags field.
         */
        // assert(option & MACH_RCV_MSG);
        //     kn->kn_data = MACH_PORT_NULL;
        // kn->kn_ext[1] = self->ith_msize;
        // kn->kn_fflags = mach_msg_receive_results();
        /* kmsg and pset reference consumed */

        return KERN_SUCCESS;
}

wait_queue_head_t * duct_wait_queue_link_fdctx (xnu_wait_queue_t waitq, void * fdctx)
{
        waitq->fdctx    = fdctx;

        return &waitq->linux_waitqh;
}
