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

#ifndef DUCT__PRE_XNU_TYPES_H
#define DUCT__PRE_XNU_TYPES_H

// for arch/arm/include/asm/signal.h
#undef sigaction
#undef sigaltstack

// for linux/include/asm-generic/siginfo.h
#undef sigval
#undef sigevent
#undef siginfo_t

// bsd/sys/sockets.h
#undef ucred
#undef linger
#undef sockaddr
#undef msghdr
#undef cmsghdr
#undef sockaddr_storage

#define ucred                   xnu_ucred
#define linger                  xnu_linger
#define sockaddr                xnu_sockaddr
#define msghdr                  xnu_msghdr
#define cmsghdr                 xnu_cmsghdr
#define sockaddr_storage        xnu_sockaddr_storage

#undef sa_family_t
#define sa_family_t             xnu_sa_family_t

// bsd/sys/mount.h
#undef statfs
#undef statfs64

#define statfs                  xnu_statfs
#define statfs64                xnu_statfs64

// bsd/sys/_structs.h
#undef timeval
#undef timezone
#undef timespec

#define timeval                 xnu_timeval
#define timezone                xnu_timezone
#define timespec                xnu_timespec

#undef _STRUCT_TIMEVAL
#undef _STRUCT_TIMESPEC
#undef _STRUCT_TIMEZONE


// osfmk/kern/sync_sema.h
#undef semaphore
#define semaphore               xnu_semaphore

// osfmk/kern/kern_types.h
//#undef waitq_t
//#define waitq_t            xnu_wait_queue_t


// bsd/sys/_structs.h
#undef rusage
#undef rlimit
#define rusage                  xnu_rusage
#define rlimit                  xnu_rlimit

#undef iovec
#define iovec                   xnu_iovec


// bsd/sys/select.h, bsd/sys/signal.h, bsd/sys/ucontext.h
#undef sigset_t
#define sigset_t                xnu_sigset_t
#undef sigprocmask
#define sigprocmask             xnu_sigprocmask


// compats
#define uthread                 compat_uthread
#define uthread_t               compat_uthread_t

struct compat_proc;
#define proc                    compat_proc


#undef fsid_t
#define fsid_t xnu_fsid_

#endif // DUCT__PRE_XNU_TYPES_H
