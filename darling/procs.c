#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/zalloc.h>
#include <kern/locks.h>
#include <kern/queue.h>
#include <sys/proc_internal.h>
#include <sys/queue.h>
#include <libkern/OSAtomic.h>
#include <duct/duct_post_xnu.h>

#include "pthread_internal.h"

extern void pth_proc_hashinit(proc_t p);

/**
 * This file is for creating a thin layer of BSD `proc`s to support certain Mach functions that use them
 *
 * note: these functions are entierly Darling-specific; the XNU code actually does all proc-related creation and destruction
 * directly in `proc` management functions like `proc_exit` (indirectly, through `reap_child_locked`) and `forkproc` (directly)
 *
 * we don't really need to manage `proc`s in Darling (at least not right now), so we just need to allocate and free them
 */

#warning We might not be allocating enough space for tasks, threads, and processes
// note: i think we're actually using CONFIG_*_MAX wrong
// Apple has some checks in `osfmk/kern/startup.c` that increase the limits for these, depending on the current system's capabilities
// We don't currently perform these checks, and as a result, we have a hard limit of 512 tasks/processes and 1024 threads

#define PROC_CHUNK 64
#define PROC_MAX CONFIG_TASK_MAX

static int proc_max = PROC_MAX;
struct zone* proc_zone = NULL;

/**
 * @brief BSD-specific PID counter.
 *
 * Darling's `proc` PIDs are taken from their corresponding Linux processes. However, this counter is BSD-specific, and doesn't need to be
 * associated with the correct Linux task. It's used by BSD code to (somewhat-)uniquely identify a given process, mainly for security purposes.
 * It does not get reused as quickly as PIDs do.
 * 
 * IMO, a better name for this would be PSID, for "process security ID", because that's really what it's used for.
 */
int pidversion_counter = 0;

void darling_procs_init(void) {
	// initialize the zone for `proc`s
	proc_zone = zinit(sizeof(struct proc), proc_max * sizeof(struct proc), PROC_CHUNK * sizeof(struct proc), "procs");
};

void darling_procs_exit(void) {
	// nothing for now
};

proc_t darling_proc_create(task_t task) {
	// try to get the parent BSD process
	proc_t parent = get_bsdtask_info(task);

	// try to allocate a process
	proc_t proc = zalloc(proc_zone);

	if (proc == NULL) {
		// can't `goto error_out` because proc is not valid
		return NULL;
	}

	// zero it out
	bzero(proc, sizeof(struct proc));

	// increment the BSD-specific PID counter
	proc->p_idversion = OSIncrementAtomic(&pidversion_counter);

	// copy the stuff we need from the parent
	if (parent) {
		memcpy(&proc->p_startcopy, &parent->p_startcopy, (size_t)((char*)&proc->p_endcopy - (char*)&proc->p_startcopy));
	}

	// initialize the process mutex
	lck_mtx_init(&proc->p_mlock, LCK_GRP_NULL, LCK_ATTR_NULL);

	// intiailize the thread list
	TAILQ_INIT_AFTER_BZERO(&proc->p_uthlist);

	// initialize pthread stuff for this new process
	pth_proc_hashinit(proc);

	// tell the task about the process...
	task->bsd_info = proc;
	// ...and the process about the task
	proc->task = task;

	// TODO: not sure if we need to do anything else here right now

	return proc;

error_out:
	// if we ever need to do some cleanup on failure, this would be the place to do it;
	// you can safely assume that `proc` is allocated and accessible here

	zfree(proc_zone, proc);
	return NULL;
};

void darling_proc_destroy(proc_t proc) {
	// make sure there are no threads still active
	proc_lock(proc);
	if (!TAILQ_EMPTY(&proc->p_uthlist)) {
		panic("proc_destroy: BSD uthreads still attached to the process");
	}
	proc_unlock(proc);

	pth_proc_hashdelete(proc);

	// unregister the process from its task
	if (proc->task) {
		((task_t)proc->task)->bsd_info = NULL;
	}

	// destroy the process mutex
	lck_mtx_destroy(&proc->p_mlock, LCK_GRP_NULL);

	// finally, free the process
	zfree(proc_zone, proc);
};
