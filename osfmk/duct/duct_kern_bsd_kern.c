#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/task.h>
#include <sys/proc_internal.h>
#include <duct/duct_post_xnu.h>

// <copied from="xnu://6153.61.1/osfmk/kern/bsd_kern.c">

/*
 *
 */
void  *
get_bsdtask_info(task_t t)
{
	return t->bsd_info;
}

/*
 *
 */
void *
get_bsdthreadtask_info(thread_t th)
{
	return th->task != TASK_NULL ? th->task->bsd_info : NULL;
}

/*
 *
 */
void
set_bsdtask_info(task_t t, void * v)
{
	t->bsd_info = v;
}

/*
 *
 */
void *
get_bsdthread_info(thread_t th)
{
	return th->uthread;
}

int
get_task_version(task_t task)
{
	if (task->bsd_info) {
		return proc_pidversion(task->bsd_info);
	} else {
		return INT_MAX;
	}
}

uint64_t
get_task_uniqueid(task_t task)
{
	if (task->bsd_info) {
		return proc_uniqueid(task->bsd_info);
	} else {
		return UINT64_MAX;
	}
}

// </copied>
