#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <sys/proc_internal.h>
#include <sys/user.h>
#include <duct/duct_post_xnu.h>

void darling_uthread_destroy(uthread_t uth) {
	if (!uth) {
		return;
	}

	// will also take care of removing the thread from the process' thread list for us
	uthread_cleanup(uth->uu_thread->task, uth, uth->uu_proc);

	// deregister the uthread from the Mach thread
	uth->uu_thread->uthread = NULL;

	// finally, free the uthread
	uthread_zone_free(uth);
};
