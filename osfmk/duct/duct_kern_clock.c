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
#include "duct_kern_clock.h"

#include <mach/mach_time.h>
#include <kern/clock.h>
#include <kern/wait_queue.h>

#include "duct_post_xnu.h"


/* WC - todo wrapper */
void duct_clock_init (void)
{
        clock_oldinit ();
}


// kern/clock.c
kern_return_t duct_mach_timebase_info_trap (struct mach_timebase_info_trap_args * args)
{
        mach_timebase_info_data_t       info;

        // arm/rtclock.c
        info.numer      = 1;
        info.denom      = 1;

        // WC - XNU didn't check return value
        copyout ((void *) &info, args->info, sizeof (mach_timebase_info_data_t));

        return KERN_SUCCESS;
}

void clock_absolutetime_interval_to_deadline (uint64_t abstime, uint64_t * result)
{
        *result     = mach_absolute_time () + abstime;
}

void clock_get_uptime (uint64_t * result)
{
        *result     = mach_absolute_time ();
}

void clock_get_calendar_microtime (clock_sec_t *secs, clock_usec_t *microsecs)
{
        kprintf ("not implemented: clock_get_calendar_microtime()\n");
}

#if defined (XNU_USE_MACHTRAP_WRAPPERS_TIMEKEEPING)
kern_return_t xnusys_mach_timebase_info_trap (struct mach_timebase_info_trap_args * args)
{
        printk (KERN_NOTICE "- args->info: 0x%llx\n", args->info);

        // printk (KERN_NOTICE "xnusys_mach_timebase_info_trap: is invalid\n");
        kern_return_t   retval  = duct_mach_timebase_info_trap (args);
        // printk (KERN_NOTICE "- retval: %d", retval);
        return retval;
}

kern_return_t xnusys_mach_wait_until_trap (struct mach_wait_until_trap_args * args)
{
        printk (KERN_NOTICE "- args->deadline: 0x%x\n", (unsigned int) args->deadline);

        printk (KERN_NOTICE "xnusys_mach_wait_until_trap: not implemented\n");
        // kern_return_t   retval  = mach_mach_wait_until_trap (args);
        // printk (KERN_NOTICE "- retval: %d", retval);
        // return retval;
        return 0;
}
#endif
