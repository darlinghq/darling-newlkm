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
#include "darling_xnu_init.h"
#include "duct_kern_thread.h"
#include "duct_kern_task.h"
#include "duct_vm_map.h"

#include <kern/thread.h>
#include <kern/task.h>
#include <ipc/ipc_init.h>


static void machine_startup (void);
extern void duct_kernel_early_bootstrap(void);
extern void duct_kernel_bootstrap(void);

// [i386|arm]_init.h
void darling_xnu_init ()
{
        kprintf ("darling_xnu_init () called");
        // kprintf (KERN_NOTICE "now print using xnu's kprintf ()\n");


        kprintf ("darling.xnu.init.1 ()\n");

        duct_kernel_early_bootstrap ();
        duct_thread_bootstrap ();
        machine_startup ();

        //kprintf ("darling.xnu.init.2 ()\n");
        //machine_startup();
}

void darling_xnu_deinit (void)
{
        thread_call_deinitialize();

        // TODO: add many more!
}

// WC: should be in duct_model_dep.c
static void machine_startup ()
{
        duct_kernel_bootstrap ();
}

#ifdef CLONE_STOPPED
#error CLONE_STOPPED should not be defined
#endif

#define CLONE_STOPPED   0x02000000

#if 0
// WC: ref to fork_create_child ()
void * darling_copy_mach_thread (unsigned long clone_flags, struct task_struct * p, struct pt_regs * regs)
{
        // printk (KERN_NOTICE "darling_copy_mach_thread () called");

        thread_t        thread  = NULL;
        thread_t        curr_thread  = (thread_t) linux_current->mach_thread;

        if (curr_thread == NULL) {
                printk (KERN_NOTICE "error trying to fork from curr_thread == 0");
                return NULL;
        }

        task_t          curr_task    = curr_thread->task;
        task_t          task    = NULL;

        kern_return_t   result  = KERN_SUCCESS;

        /* from kern_fork.c:fork_create_child () */
        /* CLONE_THREAD flag */
        if (clone_flags & CLONE_THREAD) {
                task        = curr_task;
        }
        else {
                duct_task_create_internal (curr_task, 0 /* inherit_memory */, 0 /* is64bit */, &task, NULL);

                if (result != KERN_SUCCESS) {
                        printk (KERN_NOTICE "duct_task_create_internal failed.  Code: %d\n", result);
                        goto ret;
                }

               /* WC: Notes
                * - map creation was originally in task_create_internal,
                * we moved it here as we have access to linux_task struct here, and we do not
                * want to modify the interface of task_create_internal ()
                * - p->mm could be either new or referenced current->mm */

                /* WC - todo check below */
                task->map       = duct_vm_map_create (p->mm);

                // WC - todo audit_tokens, others see kern_prot.c
                // audit_token.val[0] = my_cred->cr_audit.as_aia_p->ai_auid;
                // audit_token.val[1] = my_pcred->cr_uid;
                // audit_token.val[2] = my_pcred->cr_gid;
                // audit_token.val[3] = my_pcred->cr_ruid;
                // audit_token.val[4] = my_pcred->cr_rgid;
                // audit_token.val[5] = p->p_pid;
                // audit_token.val[6] = my_cred->cr_audit.as_aia_p->ai_asid;
                // audit_token.val[7] = p->p_idversion;

                /* WC - this is wrong as it returns tgid of p from the current process's namespace, not
                 * from its own namespace. However, if we do not use namespaces, it is just fine */
                task->audit_token.val[5]    = task_tgid_vnr (p);

                // printk ( KERN_NOTICE "- current->tgid: %d, p->tgid: %d (vnr: %d), p->pid: %d\n",
                //          linux_current->tgid,
                //          p->tgid, task_tgid_vnr (p),
                //          p->pid );
                // task->audit_token.val[5]    = task_tgid_nr (p); // WC - _nr, not _vnr for now
        }


        /* CLONE_VM flag */

        // printk (KERN_NOTICE "task: 0x%x", (unsigned int) task);
        //

        //     /* Propagate CPU limit timer from parent */
        //     if (timerisset(&child_proc->p_rlim_cpu))
        //         task_vtimer_set(child_task, TASK_VTIMER_RLIM);
        //
        //     /* Set/clear 64 bit vm_map flag */
        //     if (is64bit)
        //         vm_map_set_64bit(get_task_map(child_task));
        //     else
        //         vm_map_set_32bit(get_task_map(child_task));
        //
        // #if CONFIG_MACF
        //     /* Update task for MAC framework */
        //     /* valid to use p_ucred as child is still not running ... */
        //     mac_task_label_update_cred(child_proc->p_ucred, child_task);
        // #endif
        //
        //     /*
        //      * Set child process BSD visible scheduler priority if nice value
        //      * inherited from parent
        //      */
        //     if (child_proc->p_nice != 0)
        //         resetpriority(child_proc);

        result  = duct_thread_create (task, &thread);
        if (result != KERN_SUCCESS) {
                printk (KERN_NOTICE "execve: thread_create failed. Code: %d\n", result);
                // task_deallocate (child_task);
                // child_task = NULL;
        }
        // linux_init_wait_on_task (& thread->lwait, p);


        /* from pthread_synch.c:bsdthread_create () */
        if (linux_current->personality & 0xff) {
                if (clone_flags & (CLONE_DETACHED | CLONE_STOPPED)) { /* pthread */
                        struct pt_regs    * childregs   = task_pt_regs (p);
                        childregs->ARM_r0   = regs->ARM_r0;

                        void              * sright;
                        mach_port_name_t    th_thport;

                        thread_reference (thread);
                        sright      = (void *) convert_thread_to_port (thread);
                        th_thport   = ipc_port_copyout_send (sright, task->itk_space);

                        childregs->ARM_r1   = th_thport;

                        printk ( KERN_NOTICE "setting childregs ARM_r0: 0x%lx, ARM_r1: 0x%lx, ARM_r3: 0x%lx\n",
                                 childregs->ARM_r0, childregs->ARM_r1, childregs->ARM_r3);
                }
        }

        // thread_set_tag(child_thread, THREAD_TAG_MAINTHREAD);
        //
        // thread_yield_internal(1);

        thread->linux_task          = p;

        p->mach_thread              = thread;

ret:
        return thread;
}
#endif

void darling_exit_mach_thread (void * mach_thread)
{
        // WC - todo check below
        duct_thread_deallocate ((thread_t) mach_thread);
}
