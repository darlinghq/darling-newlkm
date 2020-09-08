#include <duct/compiler/clang/asm-inline.h>
#include <linux/semaphore.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#else
#include <linux/sched.h>
#define __set_current_state(state_value)  \
        __set_task_state(current, state_value)
#endif

// Copied from kernel/locking/semaphore.c because down_interruptible_timeout()
// doesn't exist.
// Otherwise, Darwin sleep() would not be interruptible by signals.

struct semaphore_waiter {
        struct list_head list;
        struct task_struct *task;
        bool up;
};

extern _Bool darling_thread_canceled(void);

static inline int __down_common(struct semaphore *sem, long state,
                                                       long timeout) {
        struct task_struct *task = current;
        struct semaphore_waiter waiter;
 
        list_add_tail(&waiter.list, &sem->wait_list);
        waiter.task = task;
        waiter.up = false;

        for (;;) {
                if (signal_pending_state(state, task) || darling_thread_canceled())
                        goto interrupted;
                if (unlikely(timeout <= 0))
                        goto timed_out;
                __set_current_state(state);
                raw_spin_unlock_irq(&sem->lock);
                timeout = schedule_timeout(timeout);
                raw_spin_lock_irq(&sem->lock);
                if (waiter.up)
                        return 0;
        }
 
 timed_out:
        list_del(&waiter.list);
        return -ETIME;
 
interrupted:
        list_del(&waiter.list);
        return -EINTR;
}

static int __down_interruptible_timeout(struct semaphore *sem, long timeout)
{
        return __down_common(sem, TASK_INTERRUPTIBLE, timeout);
}

int down_interruptible_timeout(struct semaphore *sem, long timeout)
{
    unsigned long flags;
    int result = 0;

    raw_spin_lock_irqsave(&sem->lock, flags);
    if (likely(sem->count > 0))
        sem->count--;
    else
        result = __down_interruptible_timeout(sem, timeout);
    raw_spin_unlock_irqrestore(&sem->lock, flags);

    return result;
}

