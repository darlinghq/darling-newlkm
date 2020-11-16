#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <mach/exception.h>
#include <mach/mach_types.h>
#include <kern/thread.h>
#include <sys/event.h>
#include <libkern/section_keywords.h>
#include <sys/proc.h>
#include <sys/proc_internal.h>
#include <sys/systm.h>
#include <duct/duct_post_xnu.h>

#include <darling/task_registry.h>

extern proc_t current_proc(void);

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

// <copied from="xnu://6153.61.1/bsd/kern/kern_sig.c">
static int      filt_sigattach(struct knote *kn, struct kevent_qos_s *kev);
static void     filt_sigdetach(struct knote *kn);
static int      filt_signal(struct knote *kn, long hint);
static int      filt_signaltouch(struct knote *kn, struct kevent_qos_s *kev);
static int      filt_signalprocess(struct knote *kn, struct kevent_qos_s *kev);

SECURITY_READ_ONLY_EARLY(struct filterops) sig_filtops = {
	.f_attach = filt_sigattach,
	.f_detach = filt_sigdetach,
	.f_event = filt_signal,
	.f_touch = filt_signaltouch,
	.f_process = filt_signalprocess,
};

/*
 * Attach a signal knote to the list of knotes for this process.
 *
 * Signal knotes share the knote list with proc knotes.  This
 * could be avoided by using a signal-specific knote list, but
 * probably isn't worth the trouble.
 */

static int
filt_sigattach(struct knote *kn, __unused struct kevent_qos_s *kev)
{
	proc_t p = current_proc();  /* can attach only to oneself */

	proc_klist_lock();

	kn->kn_proc = p;
	kn->kn_flags |= EV_CLEAR; /* automatically set */
	kn->kn_sdata = 0;         /* incoming data is ignored */

	KNOTE_ATTACH(&p->p_klist, kn);

	proc_klist_unlock();

	/* edge-triggered events can't have fired before we attached */
	return 0;
}

/*
 * remove the knote from the process list, if it hasn't already
 * been removed by exit processing.
 */

static void
filt_sigdetach(struct knote *kn)
{
	proc_t p = kn->kn_proc;

	proc_klist_lock();
	kn->kn_proc = NULL;
	KNOTE_DETACH(&p->p_klist, kn);
	proc_klist_unlock();
}

/*
 * Post an event to the signal filter.  Because we share the same list
 * as process knotes, we have to filter out and handle only signal events.
 *
 * We assume that we process fdfree() before we post the NOTE_EXIT for
 * a process during exit.  Therefore, since signal filters can only be
 * set up "in-process", we should have already torn down the kqueue
 * hosting the EVFILT_SIGNAL knote and should never see NOTE_EXIT.
 */
static int
filt_signal(struct knote *kn, long hint)
{
	if (hint & NOTE_SIGNAL) {
		hint &= ~NOTE_SIGNAL;

		if (kn->kn_id == (unsigned int)hint) {
			kn->kn_hook32++;
		}
	} else if (hint & NOTE_EXIT) {
		panic("filt_signal: detected NOTE_EXIT event");
	}

	return kn->kn_hook32 != 0;
}

static int
filt_signaltouch(struct knote *kn, struct kevent_qos_s *kev)
{
#pragma unused(kev)

	int res;

	proc_klist_lock();

	/*
	 * No data to save - just capture if it is already fired
	 */
	res = (kn->kn_hook32 > 0);

	proc_klist_unlock();

	return res;
}

static int
filt_signalprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	int res = 0;

	/*
	 * Snapshot the event data.
	 */

	proc_klist_lock();
	if (kn->kn_hook32) {
		knote_fill_kevent(kn, kev, kn->kn_hook32);
		kn->kn_hook32 = 0;
		res = 1;
	}
	proc_klist_unlock();
	return res;
}
// </copied>

void __pthread_testcancel(int presyscall) {
	thread_t thread = current_thread();

	if (darling_thread_canceled()) {
		unix_syscall_return(EINTR);
	}
};
