#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/sched_prim.h>
#include <sys/kdebug_kernel.h>
#include <duct/duct_post_xnu.h>

void sched_thread_promote_reason(thread_t thread, uint32_t reason, __kdebug_only uintptr_t trace_obj) {
	kprintf("not implemented: sched_thread_promote_reason\n");
};

void sched_thread_unpromote_reason(thread_t thread, uint32_t reason, __kdebug_only uintptr_t trace_obj) {
	kprintf("not implemented: sched_thread_unpromote_reason\n");
};
