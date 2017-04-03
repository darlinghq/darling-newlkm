#ifndef _LINUX_DOWN_INTERRUPTIBLE_H
#define _LINUX_DOWN_INTERRUPTIBLE_H
#include <linux/semaphore.h>

int down_interruptible_timeout(struct semaphore *sem, long timeout);

#endif

