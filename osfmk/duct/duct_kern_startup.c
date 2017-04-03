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
#include "duct_kern_startup.h"
#include "duct_kern_task.h"
#include "duct_kern_thread.h"
#include "duct_kern_clock.h"
#include "duct_vm_init.h"
// #include "duct_machine_routines.h"

#include <mach/mach_types.h>
#include <ipc/ipc_init.h>
#include <kern/locks.h>
#include <kern/thread.h>
#include <kern/clock.h>
#include <libkern/version.h>

extern void compat_init (void);

static void duct_kernel_bootstrap_thread (void);

// WC: should be in duct_startup.c
void __init duct_kernel_early_bootstrap (void)
{
        lck_mod_init ();

        // WC: do not start timer
        // timer_call_initialize();
}

// WC: should be in ovrt_startup.c
void __init duct_kernel_bootstrap (void)
{
        thread_t	thread;

        // TODO: the original implementation uses printf instead; also version is not yet available
        // kprintf ("%s\n", version); /* log kernel version */

    // #define kernel_bootstrap_kprintf(x...) /**/
    #define kernel_bootstrap_kprintf(x...) printk (KERN_NOTICE x)

        // scale_setup();

        kernel_bootstrap_kprintf ("calling vm_mem_bootstrap\n");
        duct_vm_mem_bootstrap ();

        // WC: not necessary
        // kernel_bootstrap_kprintf("calling vm_mem_init\n");
        // vm_mem_init();

        // machine_info.memory_size = (uint32_t)mem_size;
        // machine_info.max_mem = max_mem;
        // machine_info.major_version = version_major;
        // machine_info.minor_version = version_minor;

        // WC: intentionally left out
        // kernel_bootstrap_kprintf("calling sched_init\n");
        // sched_init();

        kernel_bootstrap_kprintf ("calling duct_wait_queue_bootstrap\n");
        duct_wait_queue_bootstrap ();

        kernel_bootstrap_kprintf ("calling ipc_bootstrap\n");
        ipc_bootstrap ();

    // #if CONFIG_MACF
    //     mac_policy_init();
    // #endif

        kernel_bootstrap_kprintf ("calling ipc_init\n");
        ipc_init ();

        /*
         * As soon as the virtual memory system is up, we record
         * that this CPU is using the kernel pmap.
         */
        // kernel_bootstrap_kprintf("calling PMAP_ACTIVATE_KERNEL\n");
        // PMAP_ACTIVATE_KERNEL(master_cpu);
        //
        // kernel_bootstrap_kprintf("calling mapping_free_prime\n");
        // mapping_free_prime();                        /* Load up with temporary mapping blocks */
        //
        // kernel_bootstrap_kprintf("calling machine_init\n");
        // machine_init();
        //
        kernel_bootstrap_kprintf("calling clock_init\n");
        duct_clock_init();

        // ledger_init();

        /*
         *	Initialize the IPC, task, and thread subsystems.
         */
        kernel_bootstrap_kprintf ("calling task_init\n");
        duct_task_init ();

        kernel_bootstrap_kprintf ("calling thread_init\n");
        duct_thread_init();

        thread          = current_thread ();
        thread->task    = kernel_task;

        kprintf ("current_thread(0x%x)\n", (unsigned int) thread);
        kprintf ("kernel_task(0x%x)\n", (unsigned int) kernel_task);



        // /*
        //  *    Create a kernel thread to execute the kernel bootstrap.
        //  */
        kernel_bootstrap_kprintf ("calling kernel_thread_create\n");
        // result = kernel_thread_create((thread_continue_t)kernel_bootstrap_thread, NULL, MAXPRI_KERNEL, &thread);
        duct_kernel_bootstrap_thread ();

        // if (result != KERN_SUCCESS) panic("kernel_bootstrap: result = %08X\n", result);
        //
        // thread->state = TH_RUN;
        // thread_deallocate(thread);
        //
        // kernel_bootstrap_kprintf ("calling load_context - done\n");
        // load_context(thread);

}


static void duct_kernel_bootstrap_thread (void)
{
    // #define kernel_bootstrap_thread_kprintf(x...) /**/
    #define kernel_bootstrap_thread_kprintf(x...) printk (KERN_NOTICE x)

        // kernel_bootstrap_thread_kprintf("calling idle_thread_create\n");
        //
        //  * Create the idle processor thread.
        //  */
        // idle_thread_create(processor);
        //
        // /*
        //  * N.B. Do not stick anything else
        //  * before this point.
        //  *
        //  * Start up the scheduler services.
        //  */
        // kernel_bootstrap_thread_kprintf("calling sched_startup\n");
        // sched_startup();
        //
        // /*
        //  * Thread lifecycle maintenance (teardown, stack allocation)
        //  */
        // kernel_bootstrap_thread_kprintf("calling thread_daemon_init\n");
        // thread_daemon_init();
        //
        // /*
        //  * Thread callout service.
        //  */
        // kernel_bootstrap_thread_kprintf("calling thread_call_initialize\n");
        // thread_call_initialize();
        //
        // /*
        //  * Remain on current processor as
        //  * additional processors come online.
        //  */
        // kernel_bootstrap_thread_kprintf("calling thread_bind\n");
        // thread_bind(processor);
        //
        // /*
        //  * Kick off memory mapping adjustments.
        //  */
        // kernel_bootstrap_thread_kprintf("calling mapping_adjust\n");
        // mapping_adjust();
        //
        /*
         *    Create the clock service.
         */
        kernel_bootstrap_thread_kprintf("calling clock_service_create\n");
        clock_service_create ();
        //
        // /*
        //  *    Create the device service.
        //  */
        // device_service_create();
        //
        // kth_started = 1;
        //
        // #if (defined(__i386__) || defined(__x86_64__)) && NCOPY_WINDOWS > 0
        // /*
        //  * Create and initialize the physical copy window for processor 0
        //  * This is required before starting kicking off  IOKit.
        //  */
        // cpu_physwindow_init(0);
        // #endif
        //
        // vm_kernel_reserved_entry_init();
        //
        // #if MACH_KDP
        // kernel_bootstrap_kprintf("calling kdp_init\n");
        // kdp_init();
        // #endif
        //
        // #if CONFIG_COUNTERS
        // pmc_bootstrap();
        // #endif
        //
        // #if (defined(__i386__) || defined(__x86_64__))
        // if (turn_on_log_leaks && !new_nkdbufs)
        //     new_nkdbufs = 200000;
        // start_kern_tracing(new_nkdbufs);
        // if (turn_on_log_leaks)
        //     log_leaks = 1;
        // #endif
        //
        // #ifdef    IOKIT
        // PE_init_iokit();
        // #endif
        //
        // (void) spllo();        /* Allow interruptions */
        //
        // #if (defined(__i386__) || defined(__x86_64__)) && NCOPY_WINDOWS > 0
        // /*
        //  * Create and initialize the copy window for processor 0
        //  * This also allocates window space for all other processors.
        //  * However, this is dependent on the number of processors - so this call
        //  * must be after IOKit has been started because IOKit performs processor
        //  * discovery.
        //  */
        // cpu_userwindow_init(0);
        // #endif
        //
        // #if (!defined(__i386__) && !defined(__x86_64__))
        // if (turn_on_log_leaks && !new_nkdbufs)
        //     new_nkdbufs = 200000;
        // start_kern_tracing(new_nkdbufs);
        // if (turn_on_log_leaks)
        //     log_leaks = 1;
        // #endif
        //
        // /*
        //  *    Initialize the shared region module.
        //  */
        // vm_shared_region_init();
        // vm_commpage_init();
        // vm_commpage_text_init();
        //
        // #if CONFIG_MACF
        // mac_policy_initmach();
        // #endif
        //
        // /*
        //  * Initialize the global used for permuting kernel
        //  * addresses that may be exported to userland as tokens
        //  * using VM_KERNEL_ADDRPERM(). Force the random number
        //  * to be odd to avoid mapping a non-zero
        //  * word-aligned address to zero via addition.
        //  */
        // vm_kernel_addrperm = (vm_offset_t)early_random() | 1;
        //
        // /*
        //  *    Start the user bootstrap.
        //  */
        // #ifdef    MACH_BSD
        // bsd_init();
        // #endif
        compat_init ();

        // /*
        //  * Get rid of segments used to bootstrap kext loading. This removes
        //  * the KLD, PRELINK symtab, LINKEDIT, and symtab segments/load commands.
        //  */
        // #if 0
        // OSKextRemoveKextBootstrap();
        // #endif
        //
        // serial_keyboard_init();        /* Start serial keyboard if wanted */
        //
        // vm_page_init_local_q();
        //
        // thread_bind(PROCESSOR_NULL);
        //
        // /*
        //  *    Become the pageout daemon.
        //  */
        // vm_pageout();
        // /*NOTREACHED

}
