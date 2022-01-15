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
#include "duct_kern_sysctl.h"

#include <kern/sync_sema.h>

#if DEBUG
int debug_kprint_syscall = 0
    // | DEBUG_KPRINT_SYSCALL_UNIX_MASK
    // | DEBUG_KPRINT_SYSCALL_MACH_MASK
    // | DEBUG_KPRINT_SYSCALL_MDEP_MASK
    // | DEBUG_KPRINT_SYSCALL_IPC_MASK
;

int debug_kprint_current_process(const char **namep)
{
#if defined (__DARLING__)
#else
        struct proc *p = current_proc();

        if (p == NULL) {
            return 0;
        }

        if (debug_kprint_syscall_process[0]) {
            /* user asked to scope tracing to a particular process name */
            if(0 == strncmp(debug_kprint_syscall_process,
                            p->p_comm, sizeof(debug_kprint_syscall_process))) {
                /* no value in telling the user that we traced what they asked */
                if(namep) *namep = NULL;

                return 1;
            } else {
                return 0;
            }
        }

        /* trace all processes. Tell user what we traced */
        if (namep) {
            *namep = p->p_comm;
        }
#endif

        return 1;
}
#endif
