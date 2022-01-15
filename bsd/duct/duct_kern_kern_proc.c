#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/thread.h> // workaround an include dependency issue
#include <sys/proc_internal.h>
#include <duct/duct_post_xnu.h>

// <copied from="xnu://6153.61.1/bsd/kern/kern_proc.c">

uint64_t
proc_uniqueid(proc_t p)
{
	return p->p_uniqueid;
}

int
proc_pidversion(proc_t p)
{
	return p->p_idversion;
}

char *
proc_name_address(void *p)
{
	return &((proc_t)p)->p_comm[0];
}

int
proc_pid(proc_t p)
{
	if (p != NULL) {
		return p->p_pid;
	}
	return -1;
}

int
IS_64BIT_PROCESS(proc_t p)
{
	if (p && (p->p_flag & P_LP64)) {
		return 1;
	} else {
		return 0;
	}
}

void
proc_klist_lock(void)
{
	lck_mtx_lock(proc_klist_mlock);
}

void
proc_klist_unlock(void)
{
	lck_mtx_unlock(proc_klist_mlock);
}

void
proc_knote(struct proc * p, long hint)
{
	proc_klist_lock();
	KNOTE(&p->p_klist, hint);
	proc_klist_unlock();
}

//</copied>
