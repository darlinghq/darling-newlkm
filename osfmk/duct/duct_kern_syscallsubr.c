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
#include "duct_kern_syscallsubr.h"

#include <mach/thread_switch.h>
#include <kern/thread.h>

#define XNU_PROBE_IN_MACHTRAP_WRAPPERS_SYSCALLSUBR

extern void resched_curr (void);

boolean_t swtch_pri (struct swtch_pri_args * args)
{
        // resched_curr ();
        // resched_task (linux_current);

        // int     lretval         = _cond_resched ();
        // return lretval ? TRUE : FALSE;

        sys_sched_yield ();     // always success

        // sleep for 1ms
        struct linux_semaphore  lsem;
        sema_init (&lsem, 0);

        uint64_t        nsecs   = 1000000;
        unsigned long   jiffies = nsecs_to_jiffies (nsecs);

        int     downret     = down_timeout (&lsem, jiffies);

        return TRUE;
}

#if defined (XNU_USE_MACHTRAP_WRAPPERS_SYSCALLSUBR)
boolean_t xnusys_swtch_pri (struct swtch_pri_args * args)
{
        printk (KERN_NOTICE "- args->pri: 0x%x\n", args->pri);
        return swtch_pri (args);
}

boolean_t xnusys_swtch (struct swtch_args * args)
{
        // dummy args
        return swtch_pri (0);
}

kern_return_t xnusys_thread_switch (struct thread_switch_args * args)
{
        printk (KERN_NOTICE "- args->thread_name: 0x%x, option: 0x%x, option_time: 0x%x\n", args->thread_name, args->option, args->option_time);

        thread_t                thread, self    = (thread_t) linux_current->mach_thread;
        mach_port_name_t        thread_name     = args->thread_name;
        int                     option          = args->option;
        mach_msg_timeout_t      option_time     = args->option_time;
        int                     lretval;

        /*
         *  Process option.
         */
        switch (option) {
                case SWITCH_OPTION_NONE:
                case SWITCH_OPTION_DEPRESS:
                case SWITCH_OPTION_WAIT:
                        break;

                default:
                        return -LINUX_EINVAL;
        }

        /*
         * Translate the port name if supplied.
         */
        if (thread_name != MACH_PORT_NULL) {
                ipc_port_t          port;
                kern_return_t       kret =
                ipc_port_translate_send (current_space (), thread_name, &port);
                if (kret == KERN_SUCCESS) {
                        ip_reference (port);
                        ip_unlock (port);

                        thread = (thread_t) convert_port_to_thread (port);
                        ip_release (port);
                        if (thread == self) {
                                (void) thread_deallocate_internal(thread);
                                thread = THREAD_NULL;
                        }
                }
                else {
                        thread = THREAD_NULL;
                }
        }
        else {
                thread = THREAD_NULL;
        }

        if (thread != THREAD_NULL) {
                lretval     = yield_to (thread->linux_task, 1);
        }
        sys_sched_yield ();
        // sleep for 1s
        struct linux_semaphore  lsem;
        sema_init (&lsem, 0);

        uint64_t        nsecs   = 1000000;
        unsigned long   jiffies = nsecs_to_jiffies (nsecs);

        int     downret     = down_timeout (&lsem, jiffies);

        return TRUE;
}

#endif
