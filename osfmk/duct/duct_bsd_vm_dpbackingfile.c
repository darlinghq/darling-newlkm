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
#include "duct_bsd_vm_dpbackingfile.h"

#if defined (XNU_USE_MACHTRAP_WRAPPERS_VMDPBACKINGFILE)
kern_return_t xnusys_macx_swapon (struct macx_swapon_args * args)
{
        printk (KERN_NOTICE "- args->filename: 0x%x, flags: 0x%x, size: %d, priority: 0x%x \n", (unsigned int) args->filename, args->flags, args->size, args->priority);

        printk (KERN_NOTICE "xnusys_macx_swapon: is invalid\n");

        // kern_return_t   retval  = macx_swapon (args);
        // printk (KERN_NOTICE "- retval: 0x%x", retval);
        // return retval;

        return 0;
}

kern_return_t xnusys_macx_swapoff (struct macx_swapoff_args * args)
{
        printk (KERN_NOTICE "- args->filename: 0x%x, flags: 0x%x \n", (unsigned int) args->filename, args->flags);

        printk (KERN_NOTICE "xnusys_macx_swapoff: is invalid\n");

        // kern_return_t   retval  = macx_swapoff (args);
        // printk (KERN_NOTICE "- retval: 0x%x", retval);
        // return retval;

        return 0;
}

kern_return_t xnusys_macx_triggers (struct macx_triggers_args * args)
{
        printk ( KERN_NOTICE "args->hi_water: 0x%x, low_water: 0x%x, flags: 0x%x, alert_port: 0x%x \n",
                 args->hi_water, args->low_water, args->flags, (unsigned int) args->alert_port );

        printk (KERN_NOTICE "xnusys_macx_triggers: is invalid\n");

        // kern_return_t   retval  = macx_triggers (args);
        // printk (KERN_NOTICE "- retval: 0x%x", retval);
        // return retval;

        return 0;
}

kern_return_t xnusys_macx_backing_store_suspend (struct macx_backing_store_suspend_args * args)
{
        printk (KERN_NOTICE "- args->suspend: 0x%x \n", args->suspend);

        printk (KERN_NOTICE "xnusys_macx_backing_store_suspend: is invalid\n");

        // kern_return_t   retval  = macx_backing_store_suspend (args);
        // printk (KERN_NOTICE "- retval: 0x%x", retval);
        // return retval;

        return 0;
}
kern_return_t xnusys_macx_backing_store_recovery (struct macx_backing_store_recovery_args * args)
{
        printk (KERN_NOTICE "- args->pid: 0x%x \n", args->pid);

        printk (KERN_NOTICE "xnusys_macx_backing_store_recovery: is invalid\n");

        // kern_return_t   retval  = macx_backing_store_recovery (args);
        // printk (KERN_NOTICE "- retval: 0x%x", retval);
        // return retval;

        return 0;
}
#endif
