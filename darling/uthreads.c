#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <sys/proc_internal.h>
#include <sys/user.h>
#include <duct/duct_post_xnu.h>

#include "uthreads.h"

void darling_uthread_destroy(struct uthread* uth) {
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

_Bool darling_uthread_is_canceling(struct uthread* uth) {
	return (uth->uu_flag & (UT_CANCELDISABLE | UT_CANCEL | UT_CANCELED)) == UT_CANCEL;
};

_Bool darling_uthread_is_cancelable(struct uthread* uth) {
	return (uth->uu_flag & UT_CANCELDISABLE) == 0;
};

_Bool darling_uthread_mark_canceling(struct uthread* uth) {
	if (darling_uthread_is_cancelable(uth)) {
		uth->uu_flag |= UT_CANCEL;
		return true;
	}
	return false;
};

void darling_uthread_change_cancelable(struct uthread* uth, _Bool cancelable) {
	if (cancelable) {
		uth->uu_flag &= ~UT_CANCELDISABLE;
	} else {
		uth->uu_flag |= UT_CANCELDISABLE;
	}
};
