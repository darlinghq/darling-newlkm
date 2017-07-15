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

#ifndef DUCT__PRE_XNU_COMPAT_FUNCS_H
#define DUCT__PRE_XNU_COMPAT_FUNCS_H

#define cpumask_t compat_cpumask_t

// for compat_proc
#define current_uthread         compat_current_uthread
#define current_proc            compat_current_proc

#define hashinit                compat_hashinit

#define thread_block            compat_thread_block
#define thread_block_parameter  compat_thread_block_parameter

#define assert_wait_deadline    compat_assert_wait_deadline
#define assert_wait             compat_assert_wait

#define thread_wakeup_prim      compat_thread_wakeup_prim

// for duct_thread
#define thread_deallocate       duct_thread_deallocate
#define thread_terminate        duct_thread_terminate


// for xnu/osfmk/kern/zalloc.h
#define zinit                   duct_zinit
#define zone_change             duct_zone_change
#define zalloc                  duct_zalloc
#define zfree                   duct_zfree


// for xnu/osfmk/kern/kalloc.h
#undef kfree

#define kalloc                  duct_kalloc
#define kalloc_noblock          duct_kalloc_noblock
#define kfree                   duct_kfree


// for xnu/osfmk/libsa/string.h
#define bcmp                    duct_bcmp
#define bcopy                   duct_bcopy
#define bzero                   duct_bzero




// for duct_waitqueue
#define waitq_init                 duct_waitq_init
#define waitq_set_init             duct_waitq_set_init

#define waitq_link_allocate        duct_waitq_link_allocate
#define waitq_link_free            duct_waitq_link_free

#define waitq_link_noalloc         duct_waitq_link_noalloc
#define waitq_unlink_nofree        duct_waitq_unlink_nofree


// xnu/osfmk/vm/vm_map.h
#define vm_map_copyin_common            duct_vm_map_copyin_common
#define vm_map_copy_discard             duct_vm_map_copy_discard
#define vm_map_copyout                  duct_vm_map_copyout

#endif
