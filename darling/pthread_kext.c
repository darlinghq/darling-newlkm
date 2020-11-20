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
#include <sys/pthread_shims.h>
#include <duct/duct_post_xnu.h>

#include "pthread_kext.h"
#include "pthread_internal.h"
#include "psynch_support.h"
#include "syscall_args.h"

/**
 *
 * `pthread_shims.c` plumbing
 *
 */

static const struct pthread_functions_s _darling_pthread_functions = {
	.pthread_init               = darling_pthread_init,

	.pth_proc_hashinit          = darling_pth_proc_hashinit,
	.pth_proc_hashdelete        = darling_pth_proc_hashdelete,

	.bsdthread_create           = darling_bsdthread_create,
	.bsdthread_register         = darling_bsdthread_register,
	.bsdthread_terminate        = darling_bsdthread_terminate,

	.thread_selfid              = darling_thread_selfid,

	.psynch_mutexwait           = darling_psynch_mutexwait,
	.psynch_mutexdrop           = darling_psynch_mutexdrop,
	.psynch_cvbroad             = darling_psynch_cvbroad,
	.psynch_cvsignal            = darling_psynch_cvsignal,
	.psynch_cvwait              = darling_psynch_cvwait,
	.psynch_cvclrprepost        = darling_psynch_cvclrprepost,
	.psynch_rw_longrdlock       = darling_psynch_rw_longrdlock,
	.psynch_rw_rdlock           = darling_psynch_rw_rdlock,
	.psynch_rw_unlock           = darling_psynch_rw_unlock,
	.psynch_rw_wrlock           = darling_psynch_rw_wrlock,
	.psynch_rw_yieldwrlock      = darling_psynch_rw_yieldwrlock,

	.pthread_find_owner         = darling_pthread_find_owner,
	.pthread_get_thread_kwq     = darling_pthread_get_thread_kwq,

	.workq_create_threadstack   = darling_workq_create_threadstack,
	.workq_destroy_threadstack  = darling_workq_destroy_threadstack,
	.workq_setup_thread         = darling_workq_setup_thread,
	.workq_handle_stack_events  = darling_workq_handle_stack_events,
	.workq_markfree_threadstack = darling_workq_markfree_threadstack,
};

// called by our kernel module during initialization
//
// this is different from `darling_pthread_init`, because this function is the one that sets up
// the pthread kext plumbing, while the `pthread_init` is only called by some BSD code after the kext has already been set up
void darling_pthread_kext_init(void) {
	// we don't really need this, since we're not actually a kext and we have full access to the whole kernel,
	// but `pthread_shims.c` won't take "no" for an answer (it'll panic if we give it `NULL`)
	pthread_callbacks_t callbacks = NULL;

	pthread_kext_register(&_darling_pthread_functions, &callbacks);
	psynch_init();
};

// called by our kernel module when it's going to be unloaded
void darling_pthread_kext_exit(void) {
	psynch_exit();
};

/**
 * stubbed functions
 */

// called by BSD code through `pthread_init` in `pthread_shims.c`
void darling_pthread_init(void) {
	// we don't really need this, but it has to exist
};

/**
 * nobody really needs this next set of functions right now,
 * so we can just stub them for now
 */

int darling_bsdthread_create(proc_t p, user_addr_t user_func, user_addr_t user_funcarg, user_addr_t user_stack, user_addr_t user_pthread, uint32_t flags, user_addr_t* retval) {
	return ENOTSUP;
};

int darling_bsdthread_register(proc_t p, user_addr_t threadstart, user_addr_t wqthread, int pthsize, user_addr_t dummy_value, user_addr_t targetconc_ptr, uint64_t dispatchqueue_offset, int32_t* retval) {
	return ENOTSUP;
};

int darling_bsdthread_terminate(proc_t p, user_addr_t stackaddr, size_t size, uint32_t kthport, uint32_t sem, int32_t* retval) {
	return ENOTSUP;
};

int darling_thread_selfid(proc_t p, uint64_t* retval) {
	return ENOTSUP;
};

int darling_bsdthread_register2(proc_t p, user_addr_t threadstart, user_addr_t wqthread, uint32_t flags, user_addr_t stack_addr_hint, user_addr_t targetconc_ptr, uint32_t dispatchqueue_offset, uint32_t tsd_offset, int32_t* retval) {
	return ENOTSUP;
};

void darling_pthread_find_owner(thread_t thread, struct stackshot_thread_waitinfo *waitinfo) {

};

void* darling_pthread_get_thread_kwq(thread_t thread) {
	return NULL;
};

/**
 * now these are actually needed by `pthread_workqueue.c`
 */

int darling_workq_handle_stack_events(proc_t p, thread_t th, vm_map_t map, user_addr_t stackaddr, mach_port_name_t kport, user_addr_t events, int nevents, int upcall_flags) {
	return ENOTSUP;
};

int darling_workq_create_threadstack(proc_t p, vm_map_t vmap, mach_vm_offset_t* out_addr) {
	return ENOTSUP;
};

int darling_workq_destroy_threadstack(proc_t p, vm_map_t vmap, mach_vm_offset_t stackaddr) {
	return ENOTSUP;
};

void darling_workq_setup_thread(proc_t p, thread_t th, vm_map_t map, user_addr_t stackaddr, mach_port_name_t kport, int th_qos, int setup_flags, int upcall_flags) {

};

void darling_workq_markfree_threadstack(proc_t p, thread_t th, vm_map_t map, user_addr_t stackaddr) {

};
