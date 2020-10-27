#ifndef _DARLING_LKM_KQUEUE_H_
#define _DARLING_LKM_KQUEUE_H_

struct task;
struct proc;

/**
 * @brief Creates a new kqueue descriptor.
 *
 * Darling's kqueues are implemented internally as regular Linux file descriptors that wait for XNU kqueues to wake up.
 *
 * For the actual filter implementations, we implement parts of it ourselves for certain things that our XNU code doesn't handle,
 * like files, processes, etc. However, for stuff that our XNU code *does* handle, like Mach ports, we defer to the filter implementations
 * in XNU and just implement the necessary plumbing to make them work.
 */
int darling_kqueue_create(struct task* task);

/**
 * @brief Notifies the LKM that the specified descriptor is being closed.
 *
 * This is used by our kqueue plumbing to delete knotes when their descriptors are closed.
 */
int darling_closing_descriptor(struct task* task, int fd);

/**
 * @brief Initializes Darling's kqueue plumbing.
 */
void dkqueue_init(void);

/**
 * @brief Drains and closes all kqueues attached to the given process, and removes them from it.
 *
 * Called with proc_fdlock held.
 */
void dkqueue_clear(struct proc* proc);

#endif // _DARLING_LKM_KQUEUE_H_
