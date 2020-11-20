#ifndef _DARLING_LKM_PTHREAD_KEXT_H_
#define _DARLING_LKM_PTHREAD_KEXT_H_

#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <sys/proc.h>
#include <kern/thread.h>
#include <vm/vm_map.h>
#include <duct/duct_post_xnu.h>

#include "pthread_internal.h"

void darling_pthread_kext_init(void);
void darling_pthread_kext_exit(void);

extern void darling_pthread_init(void);

extern void darling_pth_proc_hashinit(proc_t p);
extern void darling_pth_proc_hashdelete(proc_t p);

extern int darling_bsdthread_create(proc_t p, user_addr_t user_func, user_addr_t user_funcarg, user_addr_t user_stack, user_addr_t user_pthread, uint32_t flags, user_addr_t* retval);
extern int darling_bsdthread_register(proc_t p, user_addr_t threadstart, user_addr_t wqthread, int pthsize, user_addr_t dummy_value, user_addr_t targetconc_ptr, uint64_t dispatchqueue_offset, int32_t* retval);
extern int darling_bsdthread_terminate(proc_t p, user_addr_t stackaddr, size_t size, uint32_t kthport, uint32_t sem, int32_t* retval);

extern int darling_thread_selfid(proc_t p, uint64_t* retval);

extern int darling_psynch_mutexwait(proc_t p, user_addr_t mutex, uint32_t mgen, uint32_t ugen, uint64_t tid, uint32_t flags, uint32_t* retval);
extern int darling_psynch_mutexdrop(proc_t p, user_addr_t mutex, uint32_t mgen, uint32_t ugen, uint64_t tid, uint32_t flags, uint32_t* retval);
extern int darling_psynch_cvbroad(proc_t p, user_addr_t cv, uint64_t cvlsgen, uint64_t cvudgen, uint32_t flags, user_addr_t mutex, uint64_t mugen, uint64_t tid, uint32_t* retval);
extern int darling_psynch_cvsignal(proc_t p, user_addr_t cv, uint64_t cvlsgen, uint32_t cvugen, int thread_port, user_addr_t mutex, uint64_t mugen, uint64_t tid, uint32_t flags, uint32_t* retval);
extern int darling_psynch_cvwait(proc_t p, user_addr_t cv, uint64_t cvlsgen, uint32_t cvugen, user_addr_t mutex, uint64_t mugen, uint32_t flags, int64_t sec, uint32_t nsec, uint32_t* retval);
extern int darling_psynch_cvclrprepost(proc_t p, user_addr_t cv, uint32_t cvgen, uint32_t cvugen, uint32_t cvsgen, uint32_t prepocnt, uint32_t preposeq, uint32_t flags, int* retval);
extern int darling_psynch_rw_longrdlock(proc_t p, user_addr_t rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags, uint32_t* retval);
extern int darling_psynch_rw_rdlock(proc_t p, user_addr_t rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags, uint32_t* retval);
extern int darling_psynch_rw_unlock(proc_t p, user_addr_t rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags, uint32_t* retval);
extern int darling_psynch_rw_wrlock(proc_t p, user_addr_t rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags, uint32_t* retval);
extern int darling_psynch_rw_yieldwrlock(proc_t p, user_addr_t rwlock, uint32_t lgenval, uint32_t ugenval, uint32_t rw_wc, int flags, uint32_t* retval);

extern int darling_bsdthread_register2(proc_t p, user_addr_t threadstart, user_addr_t wqthread, uint32_t flags, user_addr_t stack_addr_hint, user_addr_t targetconc_ptr, uint32_t dispatchqueue_offset, uint32_t tsd_offset, int32_t* retval);

extern void darling_pthread_find_owner(thread_t thread, struct stackshot_thread_waitinfo *waitinfo);
extern void* darling_pthread_get_thread_kwq(thread_t thread);

extern int darling_workq_handle_stack_events(proc_t p, thread_t th, vm_map_t map, user_addr_t stackaddr, mach_port_name_t kport, user_addr_t events, int nevents, int upcall_flags);
extern int darling_workq_create_threadstack(proc_t p, vm_map_t vmap, mach_vm_offset_t* out_addr);
extern int darling_workq_destroy_threadstack(proc_t p, vm_map_t vmap, mach_vm_offset_t stackaddr);
extern void darling_workq_setup_thread(proc_t p, thread_t th, vm_map_t map, user_addr_t stackaddr, mach_port_name_t kport, int th_qos, int setup_flags, int upcall_flags);
extern void darling_workq_markfree_threadstack(proc_t p, thread_t, vm_map_t map, user_addr_t stackaddr);

#endif // _DARLING_LKM_PTHREAD_KEXT_H_
