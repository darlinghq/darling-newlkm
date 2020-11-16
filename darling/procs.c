#include "procs.h"

#include <linux/thread_info.h>

#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/zalloc.h>
#include <kern/locks.h>
#include <kern/queue.h>
#include <vm/vm_map.h>
#include <sys/proc_internal.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/filedesc.h>
#include <sys/event.h>
#include <sys/eventvar.h>
#include <libkern/OSAtomic.h>
#include <duct/duct_post_xnu.h>

#include "kqueue.h"

extern void pth_proc_hashinit(proc_t p);

static struct filedesc* create_process_fd(proc_t proc);
static void destroy_process_fd(proc_t proc);

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
	proc_t parent;
	proc_t proc;
	struct filedesc* fdp = NULL;

	// try to get the parent BSD process
	parent = get_bsdtask_info(task);

	// try to allocate a process
	proc = zalloc(proc_zone);

	if (proc == NULL) {
		// can't `goto error_out` because proc is not valid
		return NULL;
	}

	// zero it out
	bzero(proc, sizeof(struct proc));

	// grab our PID
	// XNU code keeps a separate BSD counter, but our `task_pid` returns our namespaced Linux PID, which is what everyone in Darling cares about
	proc->p_pid = task_pid(task);

	// increment the BSD-specific PID counter
	proc->p_idversion = OSIncrementAtomic(&pidversion_counter);

	// copy the stuff we need from the parent
	if (parent) {
		memcpy(&proc->p_startcopy, &parent->p_startcopy, (size_t)((char*)&proc->p_endcopy - (char*)&proc->p_startcopy));
	}

	// set whether the process is 32-bit or 64-bit
	if (test_ti_thread_flag(task_thread_info(task->map->linux_task), TIF_ADDR32)) {
		// 32-bit
		proc->p_flag &= ~(unsigned int)P_LP64;
	} else {
		// 64-bit
		proc->p_flag |= P_LP64;
	}

	// initialize process mutexes
	lck_mtx_init(&proc->p_mlock, LCK_GRP_NULL, LCK_ATTR_NULL);
	lck_mtx_init(&proc->p_fdmlock, LCK_GRP_NULL, LCK_ATTR_NULL);

	// create a process file descriptor table
	// we don't actually need to allocate space to store descriptors, though, since we just need this for kqueue to dump its stuff
	if ((fdp = create_process_fd(proc)) == NULL) {
		goto error_out;
	}

	// intiailize the thread list
	TAILQ_INIT(&proc->p_uthlist);

	// initialize pthread stuff for this new process
	pth_proc_hashinit(proc);

	// initialize the kqueue fork listener ID to an invalid value
	proc->kqueue_fork_listener_id = -1;

	// tell the task about the process...
	task->bsd_info = proc;
	// ...and the process about the task
	proc->task = task;

	return proc;

error_out:
	// if we ever need to do some cleanup on failure, this would be the place to do it;
	// you can safely assume that `proc` is allocated and accessible here

	if (fdp != NULL) {
		FREE_ZONE(fdp, sizeof(*fdp), M_FILEDESC);
	}

	zfree(proc_zone, proc);
	return NULL;
};

void darling_proc_destroy(proc_t proc) {
	struct filedesc* fdp;

	proc_lock(proc);

	// make sure there are no threads still active
	if (!TAILQ_EMPTY(&proc->p_uthlist)) {
		duct_panic("proc_destroy: BSD uthreads still attached to the process");
	}

	// destroy the process file descriptor
	destroy_process_fd(proc);

	pth_proc_hashdelete(proc);

	// unregister the process from its task
	if (proc->task) {
		((task_t)proc->task)->bsd_info = NULL;
	}

	proc_unlock(proc);

	// destroy process mutexes
	lck_mtx_destroy(&proc->p_mlock, LCK_GRP_NULL);
	lck_mtx_destroy(&proc->p_fdmlock, LCK_GRP_NULL);

	// finally, free the process
	zfree(proc_zone, proc);
};

static struct filedesc* create_process_fd(proc_t proc) {
	struct filedesc* fdp = NULL;

	fdp = MALLOC_ZONE(fdp, struct filedesc *, sizeof(*fdp), M_FILEDESC, M_WAITOK);
	if (fdp == NULL) {
		goto out;
	}

	// initialize kqueue management stuff
	fdp->fd_knlist = NULL;
	fdp->fd_knhash = NULL;
	fdp->fd_kqhash = NULL;
	fdp->fd_wqkqueue = NULL;
	fdp->fd_kqhashmask = 0;
	fdp->fd_knlistsize = 0;
	fdp->fd_knhashmask = 0;
	lck_mtx_init(&fdp->fd_kqhashlock, LCK_GRP_NULL, LCK_ATTR_NULL);
	lck_mtx_init(&fdp->fd_knhashlock, LCK_GRP_NULL, LCK_ATTR_NULL);
	LIST_INIT(&fdp->kqueue_list);

	// attach it onto the process
	proc->p_fd = fdp;

out:
	return fdp;
};

static void destroy_process_fd(proc_t proc) {
	struct filedesc* fdp = proc->p_fd;
	struct kqworkq* deferred_kqwq = NULL;

	proc_fdlock(proc);

	knotes_dealloc(proc);
	kqworkloops_dealloc(proc);

	dkqueue_clear(proc);

	if (fdp->fd_wqkqueue) {
		deferred_kqwq = fdp->fd_wqkqueue;
		fdp->fd_wqkqueue = NULL;
	}

	proc_fdunlock(proc);

	if (deferred_kqwq) {
		kqworkq_dealloc(deferred_kqwq);
	}

	if (fdp->fd_kqhash) {
		FREE(fdp->fd_kqhash, M_KQUEUE);
	}

	lck_mtx_destroy(&fdp->fd_kqhashlock, LCK_GRP_NULL);
	lck_mtx_destroy(&fdp->fd_knhashlock, LCK_GRP_NULL);

	// free the file descriptor
	FREE_ZONE(fdp, sizeof(*fdp), M_FILEDESC);
};
