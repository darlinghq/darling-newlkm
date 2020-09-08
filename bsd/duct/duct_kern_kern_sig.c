#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <mach/exception.h>
#include <mach/mach_types.h>
#include <kern/thread.h>
#include <duct/duct_post_xnu.h>

// NOTE(@facekapow): i copied this function over from `bsd/uxkern/ux_exception.c` from the old LKM code,
//                   but i don't see it actually used by any code that we use now, so we might be able to just delete it

void threadsignal(thread_t sig_actthread, int signum, mach_exception_code_t code, boolean_t set_exitreason)
{
	if (sig_actthread->in_sigprocess)
	{
		if (signum == XNU_SIGSTOP)
		{
			// TODO: deliver LINUX_SIGSTOP directly
		}
		else
			sig_actthread->pending_signal = signum;
	}
	else
	{
		printf(
			"Someone introduced a new signal by sending a message to the exception port.\n"
			"This is not supported under Darling.\n"
		);
	}
}
