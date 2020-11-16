#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/thread.h> // workaround an include dependency issue
#include <sys/systm.h>
#include <duct/duct_post_xnu.h>

// <copied from="xnu://6153.61.1/bsd/kern/kern_time.c">

void
microuptime(
	struct timeval  *tvp)
{
	clock_sec_t             tv_sec;
	clock_usec_t    tv_usec;

	clock_get_system_microtime(&tv_sec, &tv_usec);

	tvp->tv_sec = tv_sec;
	tvp->tv_usec = tv_usec;
}

uint64_t
tvtoabstime(
	struct timeval  *tvp)
{
	uint64_t        result, usresult;

	clock_interval_to_absolutetime_interval(
		tvp->tv_sec, NSEC_PER_SEC, &result);
	clock_interval_to_absolutetime_interval(
		tvp->tv_usec, NSEC_PER_USEC, &usresult);

	return result + usresult;
}

uint64_t
tstoabstime(struct timespec *ts)
{
	uint64_t abstime_s, abstime_ns;
	clock_interval_to_absolutetime_interval(ts->tv_sec, NSEC_PER_SEC, &abstime_s);
	clock_interval_to_absolutetime_interval(ts->tv_nsec, 1, &abstime_ns);
	return abstime_s + abstime_ns;
}

int
timespec_is_valid(const struct timespec *ts)
{
	/* The INT32_MAX limit ensures the timespec is safe for clock_*() functions
	 * which accept 32-bit ints. */
	if (ts->tv_sec < 0 || ts->tv_sec > INT32_MAX ||
	    ts->tv_nsec < 0 || (unsigned long long)ts->tv_nsec > NSEC_PER_SEC) {
		return 0;
	}
	return 1;
}

// </copied>
