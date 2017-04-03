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
#include "duct_kern_thread_act.h"
#include "duct_kern_task.h"
#include "duct_kern_zalloc.h"

#include <mach/mach_types.h>
#include <kern/mach_param.h>
#include <kern/thread.h>

#include "duct_post_xnu.h"

extern wait_queue_head_t global_wait_queue_head;

kern_return_t duct_thread_terminate (thread_t thread)
{

        struct task_struct        * linux_task      = thread->linux_task;

        printk ( KERN_NOTICE "- duct_thread_terminate, thread: 0x%x, linux_task: 0x%x, pid: %d, state: %ld\n",
                 (unsigned int) thread, (unsigned int) linux_task, linux_task->pid, linux_task->state);

        kern_return_t   result;

        if (thread == THREAD_NULL) {
                return KERN_INVALID_ARGUMENT;
        }

        // if (    thread->task == kernel_task        &&
        //         thread != current_thread()            )
        //     return (KERN_FAILURE);

        result = KERN_SUCCESS;

        if (linux_task == linux_current) {
                // printk (KERN_NOTICE "- do_exit ()\n");
                do_exit (0);

                // WC - should not return never return
        }

        // other threads
        // CPH: refer to zap_other_threads () on how to kill other threads
        // sigaddset (&t->pending.signal, SIGKILL); signal_wake_up ();
        printk (KERN_NOTICE "- BUG: duct_thread_terminate can't terminate other thread now\n");
        return result;
}
