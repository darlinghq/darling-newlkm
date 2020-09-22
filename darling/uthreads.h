#ifndef _DARLING_LKM_UTHREADS_H_
#define _DARLING_LKM_UTHREADS_H_

#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <sys/proc_internal.h>
#include <sys/user.h>
#include <duct/duct_post_xnu.h>

/**
 * @brief Destroys the given BSD thread
 * @param uth The BSD thread to destroy
 */
void darling_uthread_destroy(uthread_t uth);

#endif // _DARLING_LKM_UTHREADS_H_
