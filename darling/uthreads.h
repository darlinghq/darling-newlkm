#ifndef _DARLING_LKM_UTHREADS_H_
#define _DARLING_LKM_UTHREADS_H_

struct uthread;

/**
 * @brief Destroys the given BSD thread
 *
 * This will deregister the BSD thread from its Mach thread and from its BSD process, and it will also free the given BSD thread.
 *
 * @param uth The BSD thread to destroy
 */
void darling_uthread_destroy(struct uthread* uth);

/**
 * @brief Checks if the given BSD thread has been marked for cancellation
 */
_Bool darling_uthread_is_canceling(struct uthread* uth);

/**
 * @brief Checks if the given BSD thread can be canceled
 */
_Bool darling_uthread_is_cancelable(struct uthread* uth);

/**
 * @brief Marks the given BSD thread for cancellation
 *
 * Note that this function checks to make sure the thread is cancelable before marking it.
 *
 * @returns `true` if the thread was successfully marked for cancellation, `false` otherwise
 */
_Bool darling_uthread_mark_canceling(struct uthread* uth);

/**
 * @brief Enables/disables cancellation for the given BSD thread
 */
void darling_uthread_change_cancelable(struct uthread* uth, _Bool cancelable);

#endif // _DARLING_LKM_UTHREADS_H_
