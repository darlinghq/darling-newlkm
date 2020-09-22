#include "uthreads.h"

void darling_uthread_destroy(uthread_t uth) {
	if (!uth) {
		return;
	}

	// will also take care of removing the thread from the process' thread list for us
	uthread_cleanup(uth->uu_thread->task, uth, (proc_t)get_bsdthreadtask_info(uth->uu_thread));

	// deregister the uthread from the Mach thread
	uth->uu_thread->uthread = NULL;

	// finally, free the uthread
	uthread_zone_free(uth);
};
