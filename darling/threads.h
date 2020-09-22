#ifndef _DARLING_LKM_THREADS_H_
#define _DARLING_LKM_THREADS_H_

// we don't include `kern/thread.h` so that we don't complicate the header with Mach stuff, so it can be
// easily imported into `task_registry.c` without modifiying that file

/**
 * @brief Destroys the given Mach thread
 * @param thread The Mach thread to destroy
 */
void darling_thread_destroy(thread_t thread);

#endif // _DARLING_LKM_THREADS_H_
