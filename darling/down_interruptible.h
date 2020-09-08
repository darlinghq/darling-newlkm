#ifndef _LINUX_DOWN_INTERRUPTIBLE_H
#define _LINUX_DOWN_INTERRUPTIBLE_H
#include <duct/compiler/clang/asm-inline.h>
#include <linux/semaphore.h>

int down_interruptible_timeout(struct semaphore *sem, long timeout);

#endif

