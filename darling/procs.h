#ifndef _DARLING_LKM_PROCS_H_
#define _DARLING_LKM_PROCS_H_

#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <sys/proc_internal.h>
#include <duct/duct_post_xnu.h>

/**
 * @brief Initializes the `procs` subsystem.
 */
void darling_procs_init(void);

/**
 * @brief Performs any `procs`-related cleanup necessary when the LKM is being unloaded.
 */
void darling_procs_exit(void);

/**
 * @brief Creates a new BSD process for a given Mach task.
 *
 * This function (along with `darling_proc_destroy`) is Darling-specific; XNU does process creation and destruction at the same time
 * that it performs other "process lifetime"-related like forking.
 *
 * This function needs to be callable before any threads are attached, and as such, it will not retrieve the PID from
 * the corresponding Linux task (and only Mach threads have Linux tasks associated with them).
 * **Make sure** you set the PID yourself.
 *
 * @param task The corresponding task for this new process
 * @returns A brand new BSD process
 */
proc_t darling_proc_create(task_t task);

/**
 * @brief Destroys the given BSD process and all of its associated data.
 *
 * Note that this function only destroys the BSD data; the Mach task is left almost entirely intact.
 * The only thing modified in the Mach task is the pointer to the BSD process (it's set to `NULL`).
 *
 * Make sure all BSD uthreads have been destroyed and removed from the process before calling this function.
 *
 * Take care not to use the process pointer after calling this function, as it will be freed by this function.
 * In general, it's best to set the pointer to `NULL` after calling `proc_destroy`.
 *
 * @param proc The BSD process to destroy
 */
void darling_proc_destroy(proc_t proc);

#endif // _DARLING_LKM_PROCS_H_
