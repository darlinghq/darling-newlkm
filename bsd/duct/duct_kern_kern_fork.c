#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/thread.h>
#include <kern/waitq.h>
#include <kern/mach_param.h>
#include <sys/proc_internal.h>
#include <sys/user.h>
#include <sys/reason.h>
#include <sys/doc_tombstone.h>
#include <duct/duct_post_xnu.h>

// <copied from="xnu://6153.61.1/bsd/kern/kern_fork.c" modified="true">

struct zone *uthread_zone = NULL;

static void
uthread_zone_init(void)
{
	assert(uthread_zone == NULL);

#ifndef __DARLING__
	rethrottle_lock_grp_attr = lck_grp_attr_alloc_init();
	rethrottle_lock_grp = lck_grp_alloc_init("rethrottle", rethrottle_lock_grp_attr);
	rethrottle_lock_attr = lck_attr_alloc_init();
#endif

	uthread_zone = zinit(sizeof(struct uthread),
	    thread_max * sizeof(struct uthread),
	    THREAD_CHUNK * sizeof(struct uthread),
	    "uthreads");
}

void *
uthread_alloc(task_t task, thread_t thread, int noinherit)
{
	proc_t p;
	uthread_t uth;
	uthread_t uth_parent;
	void *ut;

	if (uthread_zone == NULL) {
		uthread_zone_init();
	}

	ut = (void *)zalloc(uthread_zone);
	bzero(ut, sizeof(struct uthread));

	p = (proc_t) get_bsdtask_info(task);
	uth = (uthread_t)ut;
	uth->uu_thread = thread;

#ifdef __DARLING__
	lck_spin_init(&uth->uu_rethrottle_lock, LCK_GRP_NULL, LCK_ATTR_NULL);
#else
	lck_spin_init(&uth->uu_rethrottle_lock, rethrottle_lock_grp,
	    rethrottle_lock_attr);
#endif

#ifdef __DARLING__
	// we don't really need credentials in Darling, at least not yet
	uth->uu_ucred = NOCRED;
#else
	/*
	 * Thread inherits credential from the creating thread, if both
	 * are in the same task.
	 *
	 * If the creating thread has no credential or is from another
	 * task we can leave the new thread credential NULL.  If it needs
	 * one later, it will be lazily assigned from the task's process.
	 */
	uth_parent = (uthread_t)get_bsdthread_info(current_thread());
	if ((noinherit == 0) && task == current_task() &&
	    uth_parent != NULL &&
	    IS_VALID_CRED(uth_parent->uu_ucred)) {
		/*
		 * XXX The new thread is, in theory, being created in context
		 * XXX of parent thread, so a direct reference to the parent
		 * XXX is OK.
		 */
		kauth_cred_ref(uth_parent->uu_ucred);
		uth->uu_ucred = uth_parent->uu_ucred;
		/* the credential we just inherited is an assumed credential */
		if (uth_parent->uu_flag & UT_SETUID) {
			uth->uu_flag |= UT_SETUID;
		}
	} else {
		/* sometimes workqueue threads are created out task context */
		if ((task != kernel_task) && (p != PROC_NULL)) {
			uth->uu_ucred = kauth_cred_proc_ref(p);
		} else {
			uth->uu_ucred = NOCRED;
		}
	}
#endif


	if ((task != kernel_task) && p) {
		proc_lock(p);
		if (noinherit != 0) {
			/* workq threads will not inherit masks */
			uth->uu_sigmask = ~workq_threadmask;
		} else if (uth_parent) {
			if (uth_parent->uu_flag & UT_SAS_OLDMASK) {
				uth->uu_sigmask = uth_parent->uu_oldmask;
			} else {
				uth->uu_sigmask = uth_parent->uu_sigmask;
			}
		}
		uth->uu_context.vc_thread = thread;
		/*
		 * Do not add the uthread to proc uthlist for exec copy task,
		 * since they do not hold a ref on proc.
		 */
		if (!task_is_exec_copy(task)) {
			TAILQ_INSERT_TAIL(&p->p_uthlist, uth, uu_list);
		}
		proc_unlock(p);

#if CONFIG_DTRACE
		if (p->p_dtrace_ptss_pages != NULL && !task_is_exec_copy(task)) {
			uth->t_dtrace_scratch = dtrace_ptss_claim_entry(p);
		}
#endif
	}

	return ut;
}

/*
 * This routine frees the thread name field of the uthread_t structure. Split out of
 * uthread_cleanup() so thread name does not get deallocated while generating a corpse fork.
 */
void
uthread_cleanup_name(void *uthread)
{
	uthread_t uth = (uthread_t)uthread;

	/*
	 * <rdar://17834538>
	 * Set pth_name to NULL before calling free().
	 * Previously there was a race condition in the
	 * case this code was executing during a stackshot
	 * where the stackshot could try and copy pth_name
	 * after it had been freed and before if was marked
	 * as null.
	 */
	if (uth->pth_name != NULL) {
		void *pth_name = uth->pth_name;
		uth->pth_name = NULL;
		kfree(pth_name, MAXTHREADNAMESIZE);
	}
	return;
}

/*
 * This routine frees all the BSD context in uthread except the credential.
 * It does not free the uthread structure as well
 */
void
uthread_cleanup(task_t task, void *uthread, void * bsd_info)
{
	struct _select *sel;
	uthread_t uth = (uthread_t)uthread;
	proc_t p = (proc_t)bsd_info;

#if PROC_REF_DEBUG
	if (__improbable(uthread_get_proc_refcount(uthread) != 0)) {
		panic("uthread_cleanup called for uthread %p with uu_proc_refcount != 0", uthread);
	}
#endif

#ifndef __DARLING__
	if (uth->uu_lowpri_window || uth->uu_throttle_info) {
		/*
		 * task is marked as a low priority I/O type
		 * and we've somehow managed to not dismiss the throttle
		 * through the normal exit paths back to user space...
		 * no need to throttle this thread since its going away
		 * but we do need to update our bookeeping w/r to throttled threads
		 *
		 * Calling this routine will clean up any throttle info reference
		 * still inuse by the thread.
		 */
		throttle_lowpri_io(0);
	}
#endif

	/*
	 * Per-thread audit state should never last beyond system
	 * call return.  Since we don't audit the thread creation/
	 * removal, the thread state pointer should never be
	 * non-NULL when we get here.
	 */
	assert(uth->uu_ar == NULL);

	// we don't have kqueue in-kernel (at least not right now)
#ifndef __DARLING__
	if (uth->uu_kqr_bound) {
		kqueue_threadreq_unbind(p, uth->uu_kqr_bound);
	}
#endif

	// we don't handle BSD syscalls in the LKM
#ifndef __DARLING__
	sel = &uth->uu_select;
	/* cleanup the select bit space */
	if (sel->nbytes) {
		FREE(sel->ibits, M_TEMP);
		FREE(sel->obits, M_TEMP);
		sel->nbytes = 0;
	}
#endif

	// no in-kernel CWD tracking in Darling
#ifndef __DARLING__
	if (uth->uu_cdir) {
		vnode_rele(uth->uu_cdir);
		uth->uu_cdir = NULLVP;
	}
#endif

	// TODO: this is only disabled because of `FREE` and because it's not necessary
#ifndef __DARLING__
	if (uth->uu_wqset) {
		if (waitq_set_is_valid(uth->uu_wqset)) {
			waitq_set_deinit(uth->uu_wqset);
		}
		FREE(uth->uu_wqset, M_SELECT);
		uth->uu_wqset = NULL;
		uth->uu_wqstate_sz = 0;
	}
#endif

	// don't need exit reasons (yet?)
#ifndef __DARLING__
	os_reason_free(uth->uu_exit_reason);
#endif

	if ((task != kernel_task) && p) {
		// we currently don't really need to track advanced process state;
		// we really only have procs and uthreads in Darling right now as a place for certain
		// functions to dump their data (e.g. pthreads stuff)
#ifndef __DARLING__
		if (((uth->uu_flag & UT_VFORK) == UT_VFORK) && (uth->uu_proc != PROC_NULL)) {
			vfork_exit_internal(uth->uu_proc, 0, 1);
		}
#endif
		/*
		 * Remove the thread from the process list and
		 * transfer [appropriate] pending signals to the process.
		 * Do not remove the uthread from proc uthlist for exec
		 * copy task, since they does not have a ref on proc and
		 * would not have been added to the list.
		 */
		if (get_bsdtask_info(task) == p && !task_is_exec_copy(task)) {
			proc_lock(p);

			TAILQ_REMOVE(&p->p_uthlist, uth, uu_list);
			p->p_siglist |= (uth->uu_siglist & execmask & (~p->p_sigignore | sigcantmask));
			proc_unlock(p);
		}
#if CONFIG_DTRACE
		struct dtrace_ptss_page_entry *tmpptr = uth->t_dtrace_scratch;
		uth->t_dtrace_scratch = NULL;
		if (tmpptr != NULL && !task_is_exec_copy(task)) {
			dtrace_ptss_release_entry(p, tmpptr);
		}
#endif
	}
};

/* This routine releases the credential stored in uthread */
void
uthread_cred_free(void *uthread)
{
#ifndef __DARLING__
	uthread_t uth = (uthread_t)uthread;

	/* and free the uthread itself */
	if (IS_VALID_CRED(uth->uu_ucred)) {
		kauth_cred_t oldcred = uth->uu_ucred;
		uth->uu_ucred = NOCRED;
		kauth_cred_unref(&oldcred);
	}
#endif
}

/* This routine frees the uthread structure held in thread structure */
void
uthread_zone_free(void *uthread)
{
	uthread_t uth = (uthread_t)uthread;

	if (uth->t_tombstone) {
		kfree(uth->t_tombstone, sizeof(struct doc_tombstone));
		uth->t_tombstone = NULL;
	}

#ifdef __DARLING__
	lck_spin_destroy(&uth->uu_rethrottle_lock, LCK_GRP_NULL);
#else
	lck_spin_destroy(&uth->uu_rethrottle_lock, rethrottle_lock_grp);
#endif

	uthread_cleanup_name(uthread);
	/* and free the uthread itself */
	zfree(uthread_zone, uthread);
}

void
proc_lock(proc_t p)
{
#ifndef __DARLING__
	LCK_MTX_ASSERT(proc_list_mlock, LCK_MTX_ASSERT_NOTOWNED);
#endif
	lck_mtx_lock(&p->p_mlock);
}

void
proc_unlock(proc_t p)
{
	lck_mtx_unlock(&p->p_mlock);
}

// </copied>
