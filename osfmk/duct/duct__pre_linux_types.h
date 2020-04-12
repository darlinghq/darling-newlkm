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

#include <linux/version.h>

#ifndef DUCT__PRE_LINUX_TYPES_H
#define DUCT__PRE_LINUX_TYPES_H
// include/linux/socket.h
#define ucred                       linux_ucred
#define linger                      linux_linger
#define sockaddr                    linux_sockaddr
#define msghdr                      linux_msghdr
#define cmsghdr                     linux_cmsghdr
#define __kernel_sockaddr_storage   linux___kernel_sockaddr_storage

// include/linux/kernel.h
#define panic                       linux_panic

#define sa_family_t                 linux_sa_family_t

// asm/include/statfs.h
#define statfs                      linux_statfs
#define statfs64                    linux_statfs64

// include/linux/time.h
#define timeval                     linux_timeval
#define timezone                    linux_timezone
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,20,0)
#define timespec64                  linux_timespec
#else
#define timespec                    linux_timespec
#endif


// include/linux/semaphore.h
#define semaphore                   linux_semaphore


// include/linux/resource.h
#define rusage                      linux_rusage
#define rlimit                      linux_rlimit

// linux/include/linux/syscalls.h
#define iovec                       linux_iovec
#define fd_set                      linux_fd_set

// asm/include/signal.h
#define sigaction                   linux_sigaction
#define k_sigaction                 linux_k_sigaction
#define sigset_t                    linux_sigset_t
#define sigprocmask                 linux_sigprocmask

// include/linux/wait.h
#define wait_queue_t                linux_wait_queue_t
// LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
#define wait_queue_entry_t          linux_wait_queue_t
#define __wait_queue                wait_queue_entry
#define __wait_queue_head           wait_queue_head

//#define cpumask_t                   linux_cpumask_t

// include/linux/alarmtimer.h
#define alarm                       linux_alarm

// include/linux/mm.h
#define page_size                   linux_page_size
#define page_shift                  linux_page_shift

// include/linux/bitmap.h
#define bitmap_free                 linux_bitmap_free
#define bitmap_alloc                linux_bitmap_alloc

#define uuid_t                      linux_uuid_t
#define sigset_t                    linux_sigset_t

#endif // DUCT__PRE_LINUX_TYPES_H
