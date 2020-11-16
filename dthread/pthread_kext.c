/**
 * a brief rationale on why we need this
 * ---
 *
 * so, sometime around the release of macOS 10.9 (with xnu-2422.1.72),
 * Apple decided to stuff pthread support in-kernel into a kext (pthread.kext)
 *
 * we already have most of the functions implemented in `psync_support.c`, this file just takes care of the rest
 * and the necessary plumbing for XNU's `pthread_shims.c` so that we can use XNU's own interface as much as possible
 *
 * ignore the following; i've actually decided against building `pthread_workqueue.c`
 * (turns out its usage in `turnstile.c` only matters when workqueues are already being used for something else)
 *
 * > up until recently, we didn't really build any parts of XNU that needed it
 * >
 * > however, the new turnstile subsystem requires functions that are implemented in `pthread_workqueue.c`,
 * > and it's relatively simple to build that file by adding support for the functions it calls out to
 * > (and easy enough that it's better than stubbing them)
 *
 * so like i said, that reason is no longer valid. i decided to leave this in anyways because i had already completely
 * added it in and it works so ¯\_(ツ)_/¯
 */

#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/kern_types.h>
#include <sys/proc.h>
#include "kern_internal.h"
#include <duct/duct_post_xnu.h>

#include "pthread_kext.h"

/**
 *
 * `pthread_shims.c` plumbing
 *
 */

static const struct pthread_functions_s _darling_pthread_functions = {
	.pthread_init               = _pthread_init,

	.pth_proc_hashinit          = _pth_proc_hashinit,
	.pth_proc_hashdelete        = _pth_proc_hashdelete,

	.bsdthread_create           = _bsdthread_create,
	.bsdthread_register         = _bsdthread_register,
	.bsdthread_terminate        = _bsdthread_terminate,

	.thread_selfid              = _thread_selfid,

	.psynch_mutexwait           = _psynch_mutexwait,
	.psynch_mutexdrop           = _psynch_mutexdrop,
	.psynch_cvbroad             = _psynch_cvbroad,
	.psynch_cvsignal            = _psynch_cvsignal,
	.psynch_cvwait              = _psynch_cvwait,
	.psynch_cvclrprepost        = _psynch_cvclrprepost,
	.psynch_rw_longrdlock       = _psynch_rw_longrdlock,
	.psynch_rw_rdlock           = _psynch_rw_rdlock,
	.psynch_rw_unlock           = _psynch_rw_unlock,
	.psynch_rw_wrlock           = _psynch_rw_wrlock,
	.psynch_rw_yieldwrlock      = _psynch_rw_yieldwrlock,

	.pthread_find_owner         = _pthread_find_owner,
	.pthread_get_thread_kwq     = _pthread_get_thread_kwq,

	.workq_create_threadstack   = workq_create_threadstack,
	.workq_destroy_threadstack  = workq_destroy_threadstack,
	.workq_setup_thread         = workq_setup_thread,
	.workq_handle_stack_events  = workq_handle_stack_events,
	.workq_markfree_threadstack = workq_markfree_threadstack,
};

// called by our kernel module during initialization
//
// this is different from `darling_pthread_init`, because this function is the one that sets up
// the pthread kext plumbing, while the `pthread_init` is only called by some BSD code after the kext has already been set up
void darling_pthread_kext_init(void) {
	// we don't really need the callbacks, since we're not actually a kext and we have full access to the whole kernel,
	// but it's easier to provide the callbacks than it is to modify every instance of `pthread_kern->whatever(...)`.
	// plus, `pthread_shims.c` won't take "no" for an answer (it'll panic if we give it `NULL`).
	// we have this a local variable though, because since we *aren't* a kext, we have `pthread_kern` already defined in `pthread_shims.c`
	pthread_callbacks_t callbacks = NULL;

	pthread_kext_register(&_darling_pthread_functions, &callbacks);
};

// called by our kernel module when it's going to be unloaded
void darling_pthread_kext_exit(void) {};

// temporarily copied over from kern_support.c (until we start building that file)
// <copied from="libpthread://416.60.2/kern/kern_support.c" modified="true">
#ifdef __DARLING__
uint32_t pthread_debug_tracing = 0;
#else
uint32_t pthread_debug_tracing = 1;
#endif

#ifdef __DARLING__
static lck_grp_attr_t the_real_pthread_lck_grp_attr;
static lck_grp_t the_real_pthread_lck_grp;
static lck_attr_t the_real_pthread_lck_attr;
static lck_mtx_t the_real_pthread_list_mlock;

lck_grp_attr_t* pthread_lck_grp_attr = &the_real_pthread_lck_grp_attr;
lck_grp_t* pthread_lck_grp = &the_real_pthread_lck_grp;
lck_attr_t* pthread_lck_attr = &the_real_pthread_lck_attr;
#else
lck_grp_attr_t   *pthread_lck_grp_attr;
lck_grp_t    *pthread_lck_grp;
lck_attr_t   *pthread_lck_attr;
#endif

void
_pthread_init(void)
{
#ifdef __DARLING__
	lck_grp_attr_setdefault(pthread_lck_grp_attr);
	lck_grp_init(pthread_lck_grp, "pthread", pthread_lck_grp_attr);

	lck_attr_setdefault(pthread_lck_attr);
	pthread_list_mlock = &the_real_pthread_list_mlock;
	lck_mtx_init(pthread_list_mlock, pthread_lck_grp, pthread_lck_attr);
#else
	pthread_lck_grp_attr = lck_grp_attr_alloc_init();
	pthread_lck_grp = lck_grp_alloc_init("pthread", pthread_lck_grp_attr);

	/*
	 * allocate the lock attribute for pthread synchronizers
	 */
	pthread_lck_attr = lck_attr_alloc_init();
	pthread_list_mlock = lck_mtx_alloc_init(pthread_lck_grp, pthread_lck_attr);
#endif

	pth_global_hashinit();
	psynch_thcall = thread_call_allocate(psynch_wq_cleanup, NULL);
	psynch_zoneinit();

#ifndef __DARLING__
	int policy_bootarg;
	if (PE_parse_boot_argn("pthread_mutex_default_policy", &policy_bootarg, sizeof(policy_bootarg))) {
		pthread_mutex_default_policy = policy_bootarg;
	}

	sysctl_register_oid(&sysctl__kern_pthread_mutex_default_policy);
#endif
}
// </copied>

/**
 * stubbed functions
 */

/**
 * nobody really needs this next set of functions right now,
 * so we can just stub them for now
 */

int _bsdthread_create(proc_t p, user_addr_t user_func, user_addr_t user_funcarg, user_addr_t user_stack, user_addr_t user_pthread, uint32_t flags, user_addr_t* retval) {
	return ENOTSUP;
};

int _bsdthread_register(proc_t p, user_addr_t threadstart, user_addr_t wqthread, int pthsize, user_addr_t dummy_value, user_addr_t targetconc_ptr, uint64_t dispatchqueue_offset, int32_t* retval) {
	return ENOTSUP;
};

int _bsdthread_terminate(proc_t p, user_addr_t stackaddr, size_t size, uint32_t kthport, uint32_t sem, int32_t* retval) {
	return ENOTSUP;
};

int _thread_selfid(proc_t p, uint64_t* retval) {
	return ENOTSUP;
};

int _bsdthread_register2(proc_t p, user_addr_t threadstart, user_addr_t wqthread, uint32_t flags, user_addr_t stack_addr_hint, user_addr_t targetconc_ptr, uint32_t dispatchqueue_offset, uint32_t tsd_offset, int32_t* retval) {
	return ENOTSUP;
};

/**
 * now these are actually needed by `pthread_workqueue.c`
 */

int workq_handle_stack_events(proc_t p, thread_t th, vm_map_t map, user_addr_t stackaddr, mach_port_name_t kport, user_addr_t events, int nevents, int upcall_flags) {
	return ENOTSUP;
};

int workq_create_threadstack(proc_t p, vm_map_t vmap, mach_vm_offset_t* out_addr) {
	return ENOTSUP;
};

int workq_destroy_threadstack(proc_t p, vm_map_t vmap, mach_vm_offset_t stackaddr) {
	return ENOTSUP;
};

void workq_setup_thread(proc_t p, thread_t th, vm_map_t map, user_addr_t stackaddr, mach_port_name_t kport, int th_qos, int setup_flags, int upcall_flags) {

};

void workq_markfree_threadstack(proc_t p, thread_t th, vm_map_t map, user_addr_t stackaddr) {

};
