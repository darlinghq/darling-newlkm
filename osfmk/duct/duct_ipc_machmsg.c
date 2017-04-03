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
#include "duct_ipc_machmsg.h"

#if defined (XNU_USE_MACHTRAP_WRAPPERS_MACHMSG)
mach_msg_return_t xnusys_mach_msg_trap (struct mach_msg_overwrite_trap_args * args)
{
        printk ( KERN_NOTICE "- args->msg: 0x%x, option: 0x%x, send_size: %d, rcv_size: %d, rcv_name: 0x%x, timeout: 0x%x, rcv_msg: 0x%x \n",
                 args->msg, args->option, args->send_size, args->rcv_size, args->rcv_name, args->timeout, args->rcv_msg );

        return mach_msg_trap (args);
}

mach_msg_return_t xnusys_mach_msg_overwrite_trap (struct mach_msg_overwrite_trap_args * args)
{
        printk ( KERN_NOTICE "- args->msg: 0x%x, option: 0x%x, send_size: %d, rcv_size: %d, rcv_name: 0x%x, timeout: 0x%x, rcv_msg: 0x%x \n",
                 args->msg, args->option, args->send_size, args->rcv_size, args->rcv_name, args->timeout, args->rcv_msg );

        return mach_msg_overwrite_trap (args);
}
#endif
