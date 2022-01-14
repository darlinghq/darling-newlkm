#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/sched_urgency.h>
#include <duct/duct_post_xnu.h>

#include <darling/traps.h> // for `kprintf`

thread_urgency_t thread_get_urgency(thread_t thread, uint64_t* arg1, uint64_t* arg2) {
	kprintf("not implemented: thread_get_urgency()\n");
	if (arg1)
		*arg1 = 0;
	if (arg2)
		*arg2 = 0;
	return THREAD_URGENCY_NONE;
};
