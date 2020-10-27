/**
 * General BSD syscall support
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>
#include <kern/sched_prim.h>
#include <duct/duct_post_xnu.h>

#include <darling/debug_print.h>

void unix_syscall_return(int error) {
	debug_msg("unix_syscall_return called with error=%d", error);

	// XNU uses `thread_exception_return`, but for our purposes (jumping back to our trap) we can just use `thread_syscall_return` (implemented in `continuation.c`)
	thread_syscall_return(error);
};
