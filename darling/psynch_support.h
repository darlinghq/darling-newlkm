#ifndef _PSYNCH_SUPPORT_H
#define _PSYNCH_SUPPORT_H

#define proc_t task_t
int
psynch_mutexwait(proc_t p, struct psynch_mutexwait_args * uap, uint32_t * retval);
int
psynch_mutexdrop(proc_t p, struct psynch_mutexdrop_args * uap, uint32_t * retval);
int
psynch_cvbroad(proc_t p, struct psynch_cvbroad_args * uap, uint32_t * retval);
int
psynch_cvsignal(proc_t p, struct psynch_cvsignal_args * uap, uint32_t * retval);
int
psynch_cvwait(proc_t p, struct psynch_cvwait_args * uap, uint32_t * retval);
int
psynch_cvclrprepost(proc_t p, struct psynch_cvclrprepost_args * uap, uint32_t * retval);
int
psynch_rw_rdlock(proc_t p, struct psynch_rw_rdlock_args * uap, uint32_t * retval);
int
psynch_rw_wrlock(proc_t p, struct psynch_rw_wrlock_args * uap, uint32_t * retval);
int
psynch_rw_unlock(proc_t p, struct psynch_rw_unlock_args  * uap, uint32_t * retval);

void
pth_proc_hashinit(task_t p);

void
pth_proc_hashdelete(task_t p);

void
psynch_init(void);

void
psynch_exit(void);

#endif

