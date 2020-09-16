#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/sched_prim.h>
#include <duct/duct_post_xnu.h>

// <copied from="xnu://6153.61.1/osfmk/kern/sched_prim.c">

void
thread_set_pending_block_hint(thread_t thread, block_hint_t block_hint)
{
	thread->pending_block_hint = block_hint;
}

/*
 * Wakeup a specified thread if and only if it's waiting for this event
 */
kern_return_t
thread_wakeup_thread(
	event_t         event,
	thread_t        thread)
{
	if (__improbable(event == NO_EVENT)) {
		panic("%s() called with NO_EVENT", __func__);
	}

	if (__improbable(thread == THREAD_NULL)) {
		panic("%s() called with THREAD_NULL", __func__);
	}

	struct waitq *wq = global_eventq(event);

	return waitq_wakeup64_thread(wq, CAST_EVENT64_T(event), thread, THREAD_AWAKENED);
}

struct waitq *
assert_wait_queue(
	event_t                         event)
{
	return global_eventq(event);
}

// </copied>
