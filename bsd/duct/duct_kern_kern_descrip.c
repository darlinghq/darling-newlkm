#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/thread.h> // workaround an include dependency issue
#include <sys/proc_internal.h>
#include <duct/duct_post_xnu.h>

// <copied from="xnu://6153.61.1/bsd/kern/kern_descrip.c">

void
proc_fdlock(proc_t p)
{
	lck_mtx_lock(&p->p_fdmlock);
}

void
proc_fdunlock(proc_t p)
{
	lck_mtx_unlock(&p->p_fdmlock);
}

//</copied>
