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
#include "duct_kern_syncsema.h"

#include <kern/sync_sema.h>


#if defined (XNU_USE_MACHTRAP_WRAPPERS_SYNCSEMA)
kern_return_t xnusys_semaphore_signal_trap (struct semaphore_signal_trap_args * args)
{
        printk (KERN_NOTICE "- args->signal_name: 0x%x\n", args->signal_name);
        return semaphore_signal_trap (args);
}
kern_return_t xnusys_semaphore_signal_all_trap (struct semaphore_signal_all_trap_args * args)
{
        printk (KERN_NOTICE "- args->signal_name: 0x%x\n", args->signal_name);
        return semaphore_signal_all_trap (args);
}

kern_return_t xnusys_semaphore_signal_thread_trap (struct semaphore_signal_thread_trap_args * args)
{
        printk (KERN_NOTICE "- args->signal_name: 0x%x, thread_name: 0x%x\n", args->signal_name, args->thread_name);

        return semaphore_signal_thread_trap (args);
}

kern_return_t xnusys_semaphore_wait_trap (struct semaphore_wait_trap_args * args)
{
        printk (KERN_NOTICE "- args->wait_name: 0x%x\n", args->wait_name);
        return semaphore_wait_trap (args);
}

kern_return_t xnusys_semaphore_wait_signal_trap (struct semaphore_wait_signal_trap_args * args)
{
        printk (KERN_NOTICE "- args->wait_name: 0x%x, signal_name: 0x%x\n", args->wait_name, args->signal_name);
        return semaphore_wait_signal_trap (args);
}

kern_return_t xnusys_semaphore_timedwait_trap (struct semaphore_timedwait_trap_args * args)
{
        printk (KERN_NOTICE "- args->wait_name: 0x%x, sec: 0x%x, nsec: 0x%x\n", args->wait_name, args->sec, args->nsec);
        return semaphore_timedwait_trap (args);
}

kern_return_t xnusys_semaphore_timedwait_signal_trap (struct semaphore_timedwait_signal_trap_args * args)
{
        printk (KERN_NOTICE "- args->wait_name: 0x%x, signal_name: 0x%x, sec: 0x%x, nsec: 0x%x\n", args->wait_name, args->signal_name, args->sec, args->nsec);
        return semaphore_timedwait_signal_trap (args);
}

#endif
