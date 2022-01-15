#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/task.h>
#include <ipc/ipc_importance.h>
#include <duct/duct_post_xnu.h>

// <copied from="xnu://6153.61.1/osfmk/kern/task_policy.c">

/*
 * Check if this task should donate importance.
 *
 * May be called without taking the task lock. In that case, donor status can change
 * so you must check only once for each donation event.
 */
boolean_t
task_is_importance_donor(task_t task)
{
	if (task->task_imp_base == IIT_NULL) {
		return FALSE;
	}
	return ipc_importance_task_is_donor(task->task_imp_base);
}

static void
task_importance_update_live_donor(task_t target_task)
{
#if IMPORTANCE_INHERITANCE

	ipc_importance_task_t task_imp;

	task_imp = ipc_importance_for_task(target_task, FALSE);
	if (IIT_NULL != task_imp) {
		ipc_importance_task_update_live_donor(task_imp);
		ipc_importance_task_release(task_imp);
	}
#endif /* IMPORTANCE_INHERITANCE */
}

#if IMPORTANCE_INHERITANCE
#define __imp_only
#else
#define __imp_only __unused
#endif

void
task_importance_reset(__imp_only task_t task)
{
#if IMPORTANCE_INHERITANCE
	ipc_importance_task_t task_imp;

	/* TODO: Lower importance downstream before disconnect */
	task_imp = task->task_imp_base;
	ipc_importance_reset(task_imp, FALSE);
	task_importance_update_live_donor(task);
#endif /* IMPORTANCE_INHERITANCE */
}

// </copied>
