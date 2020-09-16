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

//</copied>
