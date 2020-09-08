#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <mach/exception.h>
#include <mach/mach_types.h>
#include <kern/thread.h>
#include <duct/duct_post_xnu.h>

extern int ux_exception(int exception, mach_exception_code_t code, mach_exception_subcode_t subcode);

kern_return_t handle_ux_exception(thread_t thread, int exception, mach_exception_code_t code, mach_exception_subcode_t subcode) {
	/* Translate exception and code to signal type */
	int ux_signal = ux_exception(exception, code, subcode);

	if (thread->in_sigprocess) {
		thread->pending_signal = ux_signal;
	} else {
		// TODO: Introduce signal
		printf("handle_ux_exception(): TODO: introduce signal\n");
	}

	return KERN_SUCCESS;
}
