#ifndef _CLANG_TO_GCC_ATOMIC_H
#define _CLANG_TO_GCC_ATOMIC_H

#ifndef __clang

#define __c11_atomic_compare_exchange_strong(addr, exp, desired, success_memorder, failure_memorder) \
	__atomic_compare_exchange_n(addr, exp, desired, 0, success_memorder, failure_memorder)

#define __c11_atomic_fetch_add __atomic_fetch_add
#define __c11_atomic_fetch_and __atomic_fetch_and
#define __c11_atomic_fetch_or __atomic_fetch_or
#define __c11_atomic_fetch_xor __atomic_fetch_xor

#define memory_order_acq_rel __ATOMIC_ACQ_REL
#define memory_order_relaxed __ATOMIC_RELAXED

#endif

#endif

