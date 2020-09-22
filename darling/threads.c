#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/thread.h>
#include <duct/duct_post_xnu.h>

#include "uthreads.h"

void darling_thread_destroy(thread_t thread) {
	if (thread->uthread) {
		darling_uthread_destroy((uthread_t)thread->uthread);
	}
};
