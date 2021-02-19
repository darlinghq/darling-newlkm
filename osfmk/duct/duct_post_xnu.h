/*
Copyright (c) 2014-2017, Wenqi Chen

Shanghai Mifu Infotech Co., Ltd
B112-113, IF Industrial Park, 508 Chunking Road, Shanghai 201103, China


All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


*/
#ifndef DUCT_POST_XNU_H
#define DUCT_POST_XNU_H

// THIS FILE IS FOR MISSING FUNCTIONS ONLY
// IF ANYTHING IS MISSING HERE, THERE SHOULD BE A LINKING ERROR!

// #include <duct/duct_kern_zalloc.h>

// #undef zone_t
// typedef struct kmem_cache *    zone_t;


// #define zinit(size, max, alloc, name)
//     (zone_t) kmem_cache_create (name, size, 0, NULL, NULL)

// #undef kfree

// WC - should not be here
extern void duct_panic(const char* reason, ...);
#undef panic
#define panic       duct_panic


#undef hw_atomic_add
#undef hw_atomic_sub

/* hw_atomic_or and hw_atomic_and are implemented in duct_kern_locks.h */

#define hw_atomic_add(dst,i)        atomic_add_return (i, (atomic_t *) dst)
#define hw_atomic_sub(dst,i)        atomic_sub_return (i, (atomic_t *) dst)


// copyio
#undef copyin
#undef copyinmsg
#undef copyout
#undef copyoutmsg

// WC - should be removed
#ifndef EFAULT
#define EFAULT  14
#endif

#define copyin(user_addr, kern_addr, nbytes) copyin_linux(user_addr, kern_addr, nbytes)

#define copyinmsg(user_addr, kern_addr, nbytes) copyin_linux(user_addr, kern_addr, nbytes)

#define copyout(kern_addr, user_addr, nbytes) copyout_linux(kern_addr, user_addr, nbytes)

#define copyoutmsg(kern_addr, user_addr, nbytes) copyout_linux(kern_addr, user_addr, nbytes)


static inline int copyin_linux(const user_addr_t user_addr, void *kern_addr, size_t nbytes)
{
        if (linux_current->mm)
                return (copy_from_user (kern_addr, (const void __user *) user_addr, nbytes) ? EFAULT : 0);
        else
        {
                // kthreads may also call Mach APIs that use copyin/copyout
                memcpy(kern_addr, (void*) user_addr, nbytes);
                return 0;
        }
}

static inline int copyout_linux(const void *kern_addr, user_addr_t user_addr, size_t nbytes)
{
        if (linux_current->mm)
                return (copy_to_user ((void __user *) user_addr, kern_addr, nbytes) ? EFAULT : 0);
        else
        {
                // kthreads may also call Mach APIs that use copyin/copyout
                memcpy((void*) user_addr, kern_addr, nbytes);
                return 0;
        }
}

/* spin locks */
#undef lck_spin_init
#undef lck_spin_lock
#undef lck_spin_unlock


#define lck_spin_init(lck, grp, attr) do { \
        spin_lock_init ((spinlock_t *) lck); \
} while (0)

// should also do the following in the above init
// lck_grp_reference (grp);
// lck_grp_lckcnt_incr (grp, LCK_TYPE_SPIN);

#define lck_spin_lock(lck)          spin_lock ((spinlock_t *) lck)
#define lck_spin_unlock(lck)        spin_unlock ((spinlock_t *) lck)
#define lck_spin_try_lock(lck)      spin_trylock ((spinlock_t *) lck)
#define lck_spin_lock_grp(lck, grp)      lck_spin_lock(lck)
#define lck_spin_try_lock_grp(lck, grp)  lck_spin_try_lock(lck)

#define kdp_lck_spin_is_acquired(lck)  0

/* hw locks */
#undef hw_lock_init
#undef hw_lock_try
#undef hw_lock_held
#undef hw_lock_to
#undef hw_lock_unlock

#define hw_lock_init(lck)              spin_lock_init ((spinlock_t *) lck)
#define hw_lock_try(lck, grp)          spin_trylock ((spinlock_t *) lck)
#define hw_lock_held(lck)              spin_is_locked ((spinlock_t *) lck)
#define hw_lock_to(lock, timeout, grp) spin_lock ((spinlock_t *) lck)
#define hw_lock_unlock(lock)           spin_unlock ((spinlock_t *) lck)


/* wait queue locks */
#if 0
#undef wait_queue_lock
#undef wait_queue_unlock

#define wait_queue_lock(wq) do { \
        spin_lock ((spinlock_t *) &(wq)->linux_waitqh.lock); \
} while (0)

#define wait_queue_unlock(wq) do { \
        spin_unlock ((spinlock_t *) &(wq)->linux_waitqh.lock); \
} while (0)

#define wait_queue_lock_irqsave(wq, flags) do { \
        spin_lock_irqsave ((spinlock_t *) &(wq)->linux_waitqh.lock, flags); \
} while (0)

#define wait_queue_unlock_irqrestore(wq, flags) do { \
        spin_unlock_irqrestore ((spinlock_t *) &(wq)->linux_waitqh.lock, flags); \
} while (0)

// #undef wait_queue_init
// #define wait_queue_init         duct_wait_queue_init
#endif


/* mutexes */
#undef lck_mtx_init
#undef lck_mtx_init_ext
#undef lck_mtx_lock
#undef lck_mtx_unlock

#define lck_mtx_init(lck, grp, attr) do { \
        mutex_init ((struct mutex *) lck); \
} while (0)

#define lck_mtx_init_ext(lck, lck_ext, grp, attr)  do { \
        mutex_init ((struct mutex *) lck); \
} while (0)

// should also do the following in the above init
// lck_grp_reference (grp);
// lck_grp_lckcnt_incr (grp, LCK_TYPE_MTX);

#define lck_mtx_lock(lck)           mutex_lock ((struct mutex *) lck)
#define lck_mtx_unlock(lck)         mutex_unlock ((struct mutex *) lck)
#define lck_mtx_try_lock(lck)       mutex_trylock ((struct mutex *) lck)

#endif // DUCT_POST_XNU_H
