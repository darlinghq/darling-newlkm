#ifndef _DARLING_LKM_SYSCALL_ARGS_H_
#define _DARLING_LKM_SYSCALL_ARGS_H_

#include <darling/api.h> // for most of the arg structures...

// ...and now let's define the rest (the ones that the outside world doesn't need to use)

struct bsdthread_register_args {
	uint64_t threadstart;
	uint64_t wqthread;
	uint32_t flags;
	uint64_t stack_addr_hint;
	uint64_t targetconc_ptr;
	uint32_t dispatchqueue_offset;
	uint32_t tsd_offset;
};

struct bsdthread_create_args {
	uint64_t func;
	uint64_t func_arg;
	uint64_t stack;
	uint64_t pthread;
	uint32_t flags;
};

struct psynch_rw_longrdlock_args {
	uint64_t rwlock;
	uint32_t lgenval;
	uint32_t ugenval;
	uint32_t rw_wc;
	int flags;
};

struct psynch_rw_unlock2_args {
	uint64_t rwlock;
	uint32_t lgenval;
	uint32_t ugenval;
	uint32_t rw_wc;
	int flags;
};

struct psynch_rw_yieldwrlock_args {
	uint64_t rwlock;
	uint32_t lgenval;
	uint32_t ugenval;
	uint32_t rw_wc;
	int flags;
};

struct psynch_rw_upgrade_args {
	uint64_t rwlock;
	uint32_t lgenval;
	uint32_t ugenval;
	uint32_t rw_wc;
	int flags;
};

struct psynch_rw_downgrade_args {
	uint64_t rwlock;
	uint32_t lgenval;
	uint32_t ugenval;
	uint32_t rw_wc;
	int flags;
};

// void?
struct thread_selfid_args;

#endif // _DARLING_LKM_SYSCALL_ARGS_H_
