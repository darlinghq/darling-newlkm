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

/*
 * naming conventions in duct zone:
 * darling_*: duct zone's darling private implementation
 * duct_*: overriding the original implmentation
 * xnu_*: rename original xnu function to avoid symbol conflicts
 */
#ifndef DUCT_H
#define DUCT_H
#include "duct__pre_linux_types.h"

/* WC - todo shouldn't be here */
#define fsid_t              linux_fsid_t


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/mman.h>
#include <linux/errno.h>
#include <linux/personality.h>
#include <linux/init.h>
#include <linux/highuid.h>
#include <linux/compiler.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/security.h>
#include <linux/utsname.h>
#include <linux/magic.h>
#include <linux/binfmts.h>
#include <linux/ctype.h>
#include <linux/file.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/stddef.h>
#include <linux/signal.h>
#include <linux/socket.h>
#include <linux/stat.h>
#include <linux/statfs.h>
#include <linux/err.h>
#include <linux/gfp.h>
#include <linux/limits.h>
#include <linux/eventpoll.h>
#include <linux/inotify.h>
#include <linux/poll.h>
#include <linux/resource.h>
#include <linux/wait.h>
#include <linux/cpumask.h>
#include <darling/down_interruptible.h>

#include <linux/mutex.h>
#include <linux/rbtree.h>
#include <linux/anon_inodes.h>
#include <linux/timerfd.h>
#include <linux/signalfd.h>
#include <linux/eventfd.h>

#include <linux/cpumask.h>
#include <linux/version.h>

#include "duct__pre_xnu_defs.h"
#include "duct__pre_xnu_errsig.h"
#include "duct__pre_xnu_sockets.h"
#include "duct__pre_xnu_types.h"
#include "duct__pre_xnu_funcs.h"

// #include <asm/string.h>

// for xnu/osfmk/mach/arm/vm_param.h
#undef PAGE_MASK
#undef PAGE_SIZE
#undef PAGE_SHIFT

// for xnu/osfmk/arm/thread.h
#undef FP_SIZE

// for xnu/osfmk/mach/clock_types.h
#undef NSEC_PER_USEC
#undef USEC_PER_SEC
#undef NSEC_PER_SEC
#undef NSEC_PER_MSEC

// for xnu/bsd/queue.h
#undef LIST_HEAD

// for xnu/bsd/sys/cdefs.h
#undef __CONCAT

// for xnu/bsd/sys/_endian.h
#undef ntohs
#undef htons
#undef ntohl
#undef htonl

// for xnu/libkern/libkern/tree.h
#undef RB_BLACK
#undef RB_RED
#undef RB_ROOT


// for xnu/bsd/sys/filio.h
#undef FIOSETOWN
#undef FIOGETOWN
#undef SIOCATMARK
#undef SIOCSPGRP
#undef SIOCGPGRP
#undef SIOCSIFADDR
#undef SIOCSIFDSTADDR
#undef SIOCSIFFLAGS
#undef SIOCGIFFLAGS
#undef SIOCSIFBRDADDR
#undef SIOCSIFNETMASK
#undef SIOCGIFMETRIC
#undef SIOCSIFMETRIC
#undef SIOCDIFADDR
#undef SIOCGIFADDR
#undef SIOCGIFDSTADDR
#undef SIOCGIFBRDADDR
#undef SIOCGIFCONF
#undef SIOCGIFNETMASK
#undef SIOCADDMULTI
#undef SIOCDELMULTI
#undef SIOCGIFMTU
#undef SIOCSIFMTU
#undef SIOCSIFVLAN
#undef SIOCGIFVLAN




// for xnu/bsd/sys/resource.h
#undef PRIO_MIN
#undef RUSAGE_CHILDREN



// // for xnu/osfmk/kern/call_entry.h
// #undef current

// for linux/arch/arm/include/asm/string.h
#undef memset

// for linux/include/linux/lockdep.h
#undef lock_acquire
#undef lock_release

// for xnu/bsd/sys/time.h
#undef itimerval
#undef timezone

#undef ILL_ILLOPC
#undef ILL_ILLTRP
#undef ILL_PRVOPC
#undef ILL_ILLOPN
#undef ILL_ILLADR
#undef ILL_PRVREG
#undef ILL_COPROC
#undef ILL_BADSTK

#undef FPE_FLTDIV
#undef FPE_FLTOVF
#undef FPE_FLTUND
#undef FPE_FLTRES
#undef FPE_FLTINV
#undef FPE_FLTSUB
#undef FPE_INTDIV
#undef FPE_INTOVF

#undef SEGV_MAPERR
#undef SEGV_ACCERR

#undef BUS_ADRALN
#undef BUS_ADRERR
#undef BUS_OBJERR

#undef TRAP_BRKPT
#undef TRAP_TRACE

#undef CLD_EXITED
#undef CLD_KILLED
#undef CLD_DUMPED
#undef CLD_TRAPPED
#undef CLD_STOPPED
#undef CLD_CONTINUED

#undef POLL_IN
#undef POLL_OUT
#undef POLL_MSG
#undef POLL_ERR
#undef POLL_PRI
#undef POLL_HUP

#undef SI_USER
#undef SI_QUEUE
#undef SI_TIMER
#undef SI_ASYNCIO
#undef SI_MESGQ

#undef SS_ONSTACK
#undef SS_DISABLE

// for linux/include/linux/resource.h
#undef getrusage



// for linux/include/linux/mm.h
#undef VM_FAULT_RETRY

// for xnu/bsd/libkern/libkern.h
#undef max
#undef min
#undef ffs

// for xnu/bsd/sys/_structs.h
#undef stack_t

// for xnu/bsd/sys/param.h
#undef NOGROUP
#undef MAXHOSTNAMELEN
#undef roundup

// for xnu/bsd/arm/param.h
#undef ALIGN

// for xnu/bsd/sys/uio.h
#undef iovec

// for xnu/bsd/sys/types.h
#undef dev_t
#undef blkcnt_t
#undef ino_t
#undef off_t
#undef suseconds_t
#undef NFDBITS

// for xnu/bsd/sys/wait.h
#undef P_ALL
#undef P_PID
#undef P_PGID

#undef WCONTINUED
#undef WNOWAIT
#undef WSTOPPED

// for xnu/bsd/sys/wait.h
#undef CTL_MAXNAME

// for linux/include/linux/ipc.h
#undef ipc_perm
#undef IPC_CREAT
#undef IPC_EXCL
#undef IPC_NOWAIT
#undef IPC_PRIVATE

// for linux/include/linux/sem.h
#undef GETNCNT
#undef GETPID
#undef GETVAL
#undef GETALL
#undef GETZCNT
#undef SETVAL
#undef SETALL
#undef SEM_UNDO
#undef SEMAEM
#undef SEMUSZ

#undef sembuf
#undef semun
#undef seminfo

// for linux/include/linux/sysctl.h
#undef __sysctl_args

// for xnu/bsd/sys/_structs.h
#undef fd_set

// // for xnu/osfmk/kern/kalloc.h
// #undef kfree

// for linux/include/asm-generic/ioctl.h
#undef IOC_OUT
#undef IOC_IN
#undef IOC_INOUT
#undef _IOC
#undef _IO
#undef _IOR
#undef _IOW
#undef _IOWR

// for linux/include/linux/limits.h
#define LINUX_PATH_MAX      4096

#undef ARG_MAX
#undef LINK_MAX
#undef MAX_CANON
#undef MAX_INPUT
#undef NGROUPS_MAX
#undef PATH_MAX
#undef PIPE_BUF


//for linux/arch/arm/include/asm/user.h
#undef NBPG


// for linux/arch/arm/include/asm/stat.h
#undef stat
#undef stat64

// for linux/arch/arm/include/asm/processor.h
// #undef STACK_TOP
// #undef nommu_start_thread
// #undef start_thread

// for linux/include/linux/mount.h
#undef MNT_NOEXEC
#undef MNT_NOSUID
#undef MNT_NODEV
#undef MNT_NOATIME

// for linux/include/linux/dcache.h
#undef nameidata

// for linux/include/linux/personality.h
// #undef set_personality

// for linux/include/linux/fs.h
#undef MNT_FORCE
#undef vfs_statfs
#undef vfs_getattr

// for linux/include/linux/kernel.h
#undef user

// for linux/arch/arm/mach-msm/include/mach/memory.h
#undef __CONCAT

// for linux/include/linux/compiler-gcc4.h
#undef __used

// for linux/include/linux/compiler-gcc.h
#undef __pure

// for linux/include/linux/klist.h
#undef klist
#undef klist_init

// from linux/arm/include/asm/proc-fns.h
// #undef processor

#endif // DUCT_H
