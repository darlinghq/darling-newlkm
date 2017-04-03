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

#ifndef DUCT_KERN_WAITQUEUE_H
#define DUCT_KERN_WAITQUEUE_H

#include <mach/mach_types.h>
#include <mach/mach_traps.h>

#include <kern/wait_queue.h>

extern int      duct_event64_ready;
#define DUCT_EVENT64_READY      CAST_EVENT64_T (&duct_event64_ready)

extern kern_return_t duct_wait_queue_init (wait_queue_t wq, int policy);
extern void duct_wait_queue_bootstrap (void);

extern kern_return_t duct_wait_queue_init (wait_queue_t wq, int policy);
extern kern_return_t duct_wait_queue_set_init (wait_queue_set_t wqset, int policy);

extern wait_queue_link_t duct_wait_queue_link_allocate (void);
extern kern_return_t duct_wait_queue_link_free (wait_queue_link_t wql);

extern kern_return_t duct_wait_queue_link_noalloc (wait_queue_t wq, wait_queue_set_t wq_set, wait_queue_link_t wql);
extern kern_return_t duct_wait_queue_unlink_nofree (wait_queue_t wq, wait_queue_set_t wq_set, wait_queue_link_t * wqlp);

extern xnu_wait_queue_t duct__wait_queue_walkup (xnu_wait_queue_t waitq, event64_t event);
extern int duct_autoremove_wake_function (linux_wait_queue_t * lwait, unsigned mode, int sync, void * key);

#endif // DUCT_KERN_WAITQUEUE_H
