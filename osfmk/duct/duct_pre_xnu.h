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

#ifndef DUCT_PRE_XNU_H
#define DUCT_PRE_XNU_H

/*
 * naming conventions in duct zone:
 * darling_*: duct zone's darling private implementation
 * duct_*: overriding the original implmentation
 * xnu_*: rename original xnu function to avoid symbol conflicts
 */

// for xnu/bsd/sys/event.h
#define klist               xnu_klist
#define klist_init          xmu_klist_init


#if 0
// for linux/arch/arm/include/asm/processor.h
#undef STACK_TOP

#define LINUX_STACK_TOP     ((linux_current->personality & ADDR_LIMIT_32BIT) ? \
             TASK_SIZE : TASK_SIZE_26)


#define nommu_start_thread   linux_nommu_start_thread

#define linux_nommu_start_thread(regs) regs->ARM_r10 = linux_current->mm->start_data

#define start_thread          linux_start_thread

#define linux_start_thread(regs,pc,sp)                    \
({                                  \
    unsigned long *stack = (unsigned long *)sp;         \
    memset(regs->uregs, 0, sizeof(regs->uregs));            \
    if (linux_current->personality & ADDR_LIMIT_32BIT)            \
        regs->ARM_cpsr = USR_MODE;              \
    else                                \
        regs->ARM_cpsr = USR26_MODE;                \
    if (elf_hwcap & HWCAP_THUMB && pc & 1)              \
        regs->ARM_cpsr |= PSR_T_BIT;                \
    regs->ARM_cpsr |= PSR_ENDSTATE;                 \
    regs->ARM_pc = pc & ~1;     /* pc */            \
    regs->ARM_sp = sp;      /* sp */            \
    regs->ARM_r2 = stack[2];    /* r2 (envp) */         \
    regs->ARM_r1 = stack[1];    /* r1 (argv) */         \
    regs->ARM_r0 = stack[0];    /* r0 (argc) */         \
    linux_nommu_start_thread(regs);                   \
})

// for linux/include/linux/personality.h
/*
 * Change personality of the currently running process.
 */
#define set_personality      linux_set_personality

#define linux_set_personality(pers) \
    ((linux_current->personality == (pers)) ? 0 : __set_personality(pers))

#endif



// for xnu/osfmk/kern/sched.h
#define avenrun             xnu_avenrun

// for xnu/osfmk/kern/zalloc.h
#define zone                xnu_zone

// for linux/include/linux/lockdep.h
#undef lock_acquire
#undef lock_release

#define lock_acquire        xnu_lock_acquire
#define lock_release        xnu_lock_release

// for xnu/bsd/sys/time.h
#define itimerval           xnu_itimerval
#define timezone            xnu_timezone


// for xnu/bsd/sys/uio.h
#define iovec               xnu_iovec

// for xnu/bsd/sys/_structs.h
#define sigaltstack       xnu_sigaltstack
#define stack_t           xnu_stack_t

// for xnu/bsd/sys/namei.h
#define nameidata         xnu_nameidata

// for xnu/bsd/sys/signal.h
#define sigval            xnu_sigval
#define sigevent          xnu_sigevent
#define siginfo_t         xnu_siginfo_t
#define sigaction         xnu_sigaction

#define sigev_notify_function   xnu_sigev_notify_function
#define sigev_notify_attributes xnu_sigev_notify_attributes

#define si_pid            xnu_si_pid
#define si_uid            xnu_si_uid
#define si_status         xnu_si_status
#define si_addr           xnu_si_addr
#define si_value          xnu_si_value
#define si_band           xnu_si_band

// for xnu/bsd/libkern/libkern.h
// #define max               xnu_max
// #define min               xnu_min
// #define ffs               xnu_ffs

// for xnu/bsd/sys/ipc.h
#define ipc_perm            xnu_ipc_perm

// for xnu/bsd/sys/sem.h
#define sembuf              xnu_sembuf
#define semun               xnu_semun
#define seminfo             xnu_seminfo

// for xnu/bsd/sys/sysproto.h
#define __sysctl_args       xnu___sysctl_args
#define getrusage           xnu_getrusage

// for xnu/bsd/sys/types.h
#define dev_t               xnu_dev_t
#define blkcnt_t            xnu_blkcnt_t
#define ino_t               xnu_ino_t
#define off_t               xnu_off_t
#define suseconds_t         xnu_suseconds_t

// linux/include/linux/types.h
// typedef __kernel_off_t      linux_off_t


// for xnu/bsd/sys/_structs.h
#undef fd_set
#define fd_set              xnu_fd_set


// for xnu/bsd/sys/stat.h
#define stat                xnu_stat
#define stat64              xnu_stat64

// for xnu/bsd/sys/mount.h
#define vfs_statfs          xnu_vfs_statfs
#define vfs_getattr         xnu_vfs_getattr

// for xnu/bsd/sys/user.h
#define user                xnu_user


/* rename structs */
#undef rusage
#define rusage              xnu_rusage

/* rename typedefs */
#undef fsid_t
#define fsid_t              xnu_fsid_t



// for xnu/osfmk/kern/processor.h
#define processor           xnu_processor


// for xnu/osfmk/vm/vm_user.h
#define mach_vm_allocate        duct_mach_vm_allocate

// for xnu/osfmk/vm/vm_kern.h
#define copyinmap               duct_copyinmap
#define copyoutmap              duct_copyoutmap


// for xnu/pexpert/gen/bootargs.c
#define PE_parse_boot_argn      duct_PE_parse_boot_argn

// for xnu/osfmk/kern/printf.c
#define printf                  duct_printf



// locks
typedef spinlock_t          lck_spin_t;
typedef struct mutex        lck_mtx_t;

#undef cpumask_t
#define cpumask_t xnu_cpumask_t

#endif // DUCT_PRE_XNU_H
