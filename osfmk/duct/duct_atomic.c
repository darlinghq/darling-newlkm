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
#include "duct_atomic.h"

// arm/atomic.s

#if 0
uint32_t hw_compare_and_store (uint32_t oldval, uint32_t newval, volatile uint32_t * dest)
{
        return (atomic_cmpxchg ((atomic_t *) dest, oldval, newval) == oldval) ? 1 : 0;
}
#endif

uint32_t hw_atomic_add (volatile uint32_t * dest, uint32_t mask)
{
        return atomic_add_return (mask, (atomic_t *) dest);
}

uint32_t hw_atomic_sub (volatile uint32_t * dest, uint32_t mask)
{
        return atomic_sub_return (mask, (atomic_t *) dest);
}

#if 0
// ref: include/linux/atomic.h
void hw_atomic_and_noret (volatile uint32_t * dest, uint32_t mask)
{
        int     old;
        int     new;

        do {
                old     = atomic_read ((atomic_t *) dest);
                new     = old & mask;
        } while (atomic_cmpxchg ((atomic_t *) dest, old, new) != old);
}


void hw_atomic_or_noret (volatile uint32_t * dest, uint32_t mask)
{
        int     old;
        int     new;

        do {
                old     = atomic_read ((atomic_t *) dest);
                new     = old | mask;
        } while (atomic_cmpxchg ((atomic_t *) dest, old, new) != old);
}

uint32_t hw_atomic_or (volatile uint32_t * dest, uint32_t mask)
{
        int     old;
        int     new;

        do {
                old     = atomic_read ((atomic_t *) dest);
                new     = old | mask;
        } while (atomic_cmpxchg ((atomic_t *) dest, old, new) != old);
        return new;
}
#endif
