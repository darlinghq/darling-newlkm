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
#include "duct_machine_rtclock.h"

#include <mach/mach_time.h>
#include <kern/clock.h>
#include <kern/waitq.h>

#include "duct_post_xnu.h"

// #define XNU_PROBE_IN_MACHTRAP_WRAPPERS_TIMEKEEPING

int rtclock_init (void)
{
        // pe_arm_init_timebase(NULL);
        // clock_timebase_init();
        return 1;
}

void clock_get_system_nanotime (clock_sec_t * secs, clock_nsec_t * nanosecs)
{
        // uint64_t    now = rtc_nanotime_read();
        // _absolutetime_to_nanotime(now, secs, nanosecs);

        // ktime_t     kt      = ktime_get_boottime ();
        struct linux_timespec       ltimespec;        
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,20,0)
        ltimespec = ktime_to_timespec64(ktime_get_boottime());
#else
        get_monotonic_boottime (&ltimespec);
#endif

        *secs       = (clock_sec_t) ltimespec.tv_sec;
        *nanosecs   = (clock_nsec_t) ltimespec.tv_nsec;
}

void clock_get_system_microtime (clock_sec_t * secs, clock_usec_t * microsecs)
{
        // uint64_t  now = rtc_nanotime_read();
        // _absolutetime_to_microtime(now, secs, microsecs);
        
        // ktime_t     kt      = ktime_get_boottime ();
        struct linux_timespec       ltimespec;        
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,20,0)
        ltimespec = ktime_to_timespec64(ktime_get_boottime());
#else
        get_monotonic_boottime (&ltimespec);
#endif

        *secs       = (clock_sec_t) ltimespec.tv_sec;
        *microsecs  = (clock_nsec_t) ltimespec.tv_nsec / NSEC_PER_USEC;
}

void absolutetime_to_nanoseconds (uint64_t abstime, uint64_t * result)
{
        *result     = abstime;
}

void nanoseconds_to_absolutetime (uint64_t nanoseconds, uint64_t * result)
{
        *result     = nanoseconds;
}



uint64_t mach_absolute_time (void)
{
        // WC - todo: implementation has issue
        return ktime_to_ns (ktime_get ());
}

#if 0
uint32_t xnusys_mach_absolute_time (struct pt_regs * regs)
{
        uint64_t    abstime     = mach_absolute_time ();

        // regs->ARM_r0        = abstime & 0xFFFFFFFF;
        regs->ARM_r1        = abstime >> 32;
        // * ((uint64_t *) &regs->ARM_r0   = mach_absolute_time ();        

    #if defined (XNU_PROBE_IN_MACHTRAP_WRAPPERS_TIMEKEEPING)
        printk (KERN_NOTICE "abstime: 0x%llx\n", abstime);
    #endif
                         
        return (abstime & 0xffffffff);
}
#endif

void clock_interval_to_absolutetime_interval (uint32_t interval, uint32_t scale_factor, uint64_t * result)
{
        *result     = (uint64_t) interval * scale_factor;
}
