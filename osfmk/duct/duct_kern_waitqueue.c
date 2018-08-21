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

#include "duct.h"
#include <linux/delay.h>
#include "duct_pre_xnu.h"

#include "duct_kern_waitqueue.h"
#include "duct_kern_printf.h"

// #include <mach/mach_types.h>
#include <kern/mach_param.h>
#include <kern/thread.h>
#include <kern/spl.h>
#include <darling/task_registry.h>

#include "duct_post_xnu.h"


#define delay udelay
#define WAIT_QUEUE_MAX              thread_max
#define WAIT_QUEUE_SET_MAX          task_max * 3
#define WAIT_QUEUE_LINK_MAX         PORT_MAX / 2 + (WAIT_QUEUE_MAX * WAIT_QUEUE_SET_MAX) / 64

static zone_t _waitq_link_zone;
static zone_t _waitq_set_zone;
static zone_t _waitq_zone;

int     duct_event64_ready  = 0;

/* declare a unique type for wait queue link structures */
static unsigned int     _waitq_link;
static unsigned int     _waitq_link_noalloc;
static unsigned int     _waitq_unlinked;

#define WAIT_QUEUE_LINK             ((void *)&_waitq_link)
#define WAIT_QUEUE_LINK_NOALLOC     ((void *)&_waitq_link_noalloc)
#define WAIT_QUEUE_UNLINKED         ((void *)&_waitq_unlinked)

#define WAIT_QUEUE_ELEMENT_CHECK(wq, wqe) \
    assert ((wqe)->wqe_queue == (wq) && queue_next(queue_prev((queue_t) (wqe))) == (queue_t)(wqe))


#define WQSPREV(wqs, wql) ((waitq_link_t)queue_prev( \
    ((&(wqs)->wqs_setlinks == (queue_t)(wql)) ? \
    (queue_t)(wql) : &(wql)->wql_setlinks)))

#define WQSNEXT(wqs, wql) ((waitq_link_t)queue_next( \
    ((&(wqs)->wqs_setlinks == (queue_t)(wql)) ? \
    (queue_t)(wql) : &(wql)->wql_setlinks)))

#define WAIT_QUEUE_SET_LINK_CHECK(wqs, wql) \
    assert ((((wql)->wql_type == WAIT_QUEUE_LINK) || \
               ((wql)->wql_type == WAIT_QUEUE_LINK_NOALLOC)) && \
            ((wql)->wql_setqueue == (wqs)) && \
            (((wql)->wql_queue->wq_type == _WAIT_QUEUE_inited) || \
             ((wql)->wql_queue->wq_type == _WAIT_QUEUE_SET_inited)) && \
            (WQSNEXT((wqs), WQSPREV((wqs),(wql))) == (wql)) )


#if defined (_WAIT_QUEUE_DEBUG_) || 1

#define WAIT_QUEUE_CHECK(wq) \
MACRO_BEGIN \
    queue_t q2 = &(wq)->waitq_queue; \
    waitq_element_t wqe2 = (waitq_element_t) queue_first(q2); \
    while (!queue_end(q2, (queue_entry_t)wqe2)) { \
        WAIT_QUEUE_ELEMENT_CHECK((wq), wqe2); \
        wqe2 = (waitq_element_t) queue_next((queue_t) wqe2); \
    } \
MACRO_END

#define WAIT_QUEUE_SET_CHECK(wqs) \
MACRO_BEGIN \
    queue_t q2 = &(wqs)->wqs_setlinks; \
    waitq_link_t wql2 = (waitq_link_t) queue_first(q2); \
    while (!queue_end(q2, (queue_entry_t)wql2)) { \
            WAIT_QUEUE_SET_LINK_CHECK((wqs), wql2); \
            wql2 = (waitq_link_t) wql2->wql_setlinks.next; \
    } \
MACRO_END

#else /* _WAIT_QUEUE_DEBUG_ */

#define WAIT_QUEUE_CHECK(wq)
#define WAIT_QUEUE_SET_CHECK(wqs)

#endif /* _WAIT_QUEUE_DEBUG_ */




void duct_waitq_bootstrap (void)
{

        _waitq_zone    =
        zinit ( sizeof(struct waitq),
                WAIT_QUEUE_MAX * sizeof(struct waitq),
                sizeof(struct waitq ),
                "wait queues" );

        // WC zone_change(_waitq_zone, Z_NOENCRYPT, TRUE);

        _waitq_set_zone    =
        zinit ( sizeof(struct waitq_set),
                WAIT_QUEUE_SET_MAX * sizeof(struct waitq_set),
                sizeof(struct waitq_set),
                "wait queue sets" );
        // WC zone_change(_waitq_set_zone, Z_NOENCRYPT, TRUE);

        _waitq_link_zone   =
        zinit ( sizeof(struct waitq_link),
                WAIT_QUEUE_LINK_MAX * sizeof(struct waitq_link),
                sizeof(struct waitq_link),
                "wait queue links" );

        // WC zone_change(_waitq_link_zone, Z_NOENCRYPT, TRUE);
}


kern_return_t duct_waitq_init (waitq_t wq, int policy)
{
        /* only FIFO and LIFO for now */
        if ((policy & SYNC_POLICY_FIXED_PRIORITY) != 0)
            return KERN_INVALID_ARGUMENT;

        wq->wq_fifo     = ((policy & SYNC_POLICY_REVERSED) == 0);
        wq->wq_type     = _WAIT_QUEUE_inited;
        queue_init (&wq->waitq_queue);
        hw_lock_init (&wq->wq_interlock);

    #if defined (__DARLING__)
        // mutex_init (&wq->mutex_lock);
        init_waitqueue_head (&wq->linux_waitqh);
    #endif

        return KERN_SUCCESS;
}



kern_return_t duct_waitq_set_init (waitq_set_t wqset, int policy, uint64_t *reserved_link,
		                    void *prepost_hook)
{
        kern_return_t ret;

        ret     = duct_waitq_init (&wqset->wqs_waitq, policy);
        if (ret != KERN_SUCCESS)    return ret;

        wqset->wqs_waitq.wq_type = _WAIT_QUEUE_SET_inited;
        if (policy & SYNC_POLICY_PREPOST)
                wqset->wqs_waitq.wq_prepost    = TRUE;
        else
                wqset->wqs_waitq.wq_prepost    = FALSE;

        queue_init (&wqset->wqs_setlinks);
        queue_init (&wqset->wqs_preposts);
        return KERN_SUCCESS;
}



waitq_link_t duct_waitq_link_allocate (void)
{
        waitq_link_t       wql;

        wql     = zalloc (_waitq_link_zone); /* Can't fail */
        bzero (wql, sizeof(*wql));
        wql->wql_type   = WAIT_QUEUE_UNLINKED;

        return wql;
}

kern_return_t duct_waitq_link_free (waitq_link_t wql)
{
        zfree (_waitq_link_zone, wql);
        return KERN_SUCCESS;
}



static kern_return_t duct_waitq_link_internal (waitq_t wq, waitq_set_t wq_set, waitq_link_t wql)
{
        waitq_element_t wq_element;
        queue_t q;
        spl_t s;

        if (!waitq_is_valid(wq) || !waitq_is_set(wq_set))
                  return KERN_INVALID_ARGUMENT;

        /*
         * There are probably fewer threads and sets associated with
         * the wait queue than there are wait queues associated with
         * the set.  So let's validate it that way.
         */
        s = splsched();
        waitq_lock(wq);
        q = &wq->waitq_queue;
        wq_element = (waitq_element_t) queue_first(q);
        while (!queue_end(q, (queue_entry_t)wq_element)) {
            WAIT_QUEUE_ELEMENT_CHECK(wq, wq_element);
                if ((wq_element->wqe_type == WAIT_QUEUE_LINK ||
                     wq_element->wqe_type == WAIT_QUEUE_LINK_NOALLOC) &&
                    ((waitq_link_t)wq_element)->wql_setqueue == wq_set) {
                    waitq_unlock(wq);
                    splx(s);
                    return KERN_ALREADY_IN_SET;
                }
                wq_element = (waitq_element_t)
                        queue_next((queue_t) wq_element);
        }

        /*
         * Not already a member, so we can add it.
         */
        wqs_lock(wq_set);

        WAIT_QUEUE_SET_CHECK(wq_set);

        assert(wql->wql_type == WAIT_QUEUE_LINK ||
               wql->wql_type == WAIT_QUEUE_LINK_NOALLOC);

        wql->wql_queue = wq;
        wql_clear_prepost(wql);
        queue_enter(&wq->waitq_queue, wql, waitq_link_t, wql_links);
        wql->wql_setqueue = wq_set;
        queue_enter(&wq_set->wqs_setlinks, wql, waitq_link_t, wql_setlinks);

        wqs_unlock(wq_set);
        waitq_unlock(wq);
        splx(s);

        return KERN_SUCCESS;
}

kern_return_t duct_waitq_link_noalloc (waitq_t wq, waitq_set_t wq_set, waitq_link_t wql)
{
        wql->wql_type = WAIT_QUEUE_LINK_NOALLOC;
        return duct_waitq_link_internal (wq, wq_set, wql);
}



static void duct_waitq_unlink_locked (waitq_t wq, waitq_set_t wq_set, waitq_link_t wql)
{
        assert(waitq_held(wq));
        assert(waitq_held(&wq_set->wqs_waitq));

        wql->wql_queue = WAIT_QUEUE_NULL;
        queue_remove(&wq->waitq_queue, wql, waitq_link_t, wql_links);
        wql->wql_setqueue = WAIT_QUEUE_SET_NULL;
        queue_remove(&wq_set->wqs_setlinks, wql, waitq_link_t, wql_setlinks);
        if (wql_is_preposted(wql)) {
                queue_t ppq = &wq_set->wqs_preposts;
                queue_remove(ppq, wql, waitq_link_t, wql_preposts);
        }
        wql->wql_type = WAIT_QUEUE_UNLINKED;

        WAIT_QUEUE_CHECK(wq);
        WAIT_QUEUE_SET_CHECK(wq_set);
}

kern_return_t duct_waitq_unlink_nofree (waitq_t wq, waitq_set_t wq_set, waitq_link_t * wqlp)
{
        waitq_element_t wq_element;
        waitq_link_t wql;
        queue_t q;
        spl_t s;

        if (!waitq_is_valid(wq) || !waitq_is_set(wq_set)) {
            return KERN_INVALID_ARGUMENT;
        }
        s = splsched();
        waitq_lock(wq);

        q = &wq->waitq_queue;
        wq_element = (waitq_element_t) queue_first(q);
        while (!queue_end(q, (queue_entry_t)wq_element)) {
            WAIT_QUEUE_ELEMENT_CHECK(wq, wq_element);
            if (wq_element->wqe_type == WAIT_QUEUE_LINK ||
                wq_element->wqe_type == WAIT_QUEUE_LINK_NOALLOC) {

                   wql = (waitq_link_t)wq_element;

                if (wql->wql_setqueue == wq_set) {

                    wqs_lock(wq_set);
                    duct_waitq_unlink_locked(wq, wq_set, wql);
                    wqs_unlock(wq_set);
                    waitq_unlock(wq);
                    splx(s);
                    *wqlp = wql;
                    return KERN_SUCCESS;
                }
            }
            wq_element = (waitq_element_t)
                    queue_next((queue_t) wq_element);
        }
        waitq_unlock(wq);
        splx(s);
        return KERN_NOT_IN_SET;
}

static void
waitq_unlink_all_nofree_locked(
        waitq_t wq,
        queue_t links)
{
        waitq_element_t wq_element;
        waitq_element_t wq_next_element;
        waitq_set_t wq_set;
        waitq_link_t wql;
        queue_t q;

        q = &wq->waitq_queue;

        wq_element = (waitq_element_t) queue_first(q);
        while (!queue_end(q, (queue_entry_t)wq_element)) {

                WAIT_QUEUE_ELEMENT_CHECK(wq, wq_element);
                wq_next_element = (waitq_element_t)
                             queue_next((queue_t) wq_element);

                if (wq_element->wqe_type == WAIT_QUEUE_LINK ||
                    wq_element->wqe_type == WAIT_QUEUE_LINK_NOALLOC) {
                        wql = (waitq_link_t)wq_element;
                        wq_set = wql->wql_setqueue;
                        wqs_lock(wq_set);
                        duct_waitq_unlink_locked(wq, wq_set, wql);
                        wqs_unlock(wq_set);
                        enqueue(links, &wql->wql_links);
                }
                wq_element = wq_next_element;
        }
}       

kern_return_t
waitq_unlink_all_nofree(
        waitq_t wq,
        queue_t links)
{
        spl_t s;

        if (!waitq_is_valid(wq)) {
                return KERN_INVALID_ARGUMENT;
        }

        s = splsched();
        waitq_lock(wq);
        waitq_unlink_all_nofree_locked(wq, links);
        waitq_unlock(wq);
        splx(s);

        return(KERN_SUCCESS);
}       



wait_result_t waitq_assert_wait64_locked (
        waitq_t wq,
        event64_t event,
        wait_interrupt_t interruptible,
        uint64_t deadline,
        thread_t thread)
{
        printk (KERN_NOTICE "BUG: waitq_assert_wait64_locked () called\n");
        return 0;
}

kern_return_t
waitq_wakeup64_all_locked(
        waitq_t wq,
        event64_t event,
        wait_result_t result,
        boolean_t unlock)
{
        printk (KERN_NOTICE "BUG: waitq_wakeup64_all_locked () called\n");
        return 0;
}

// Needed for kevpsetfd
// It doesn't enqueue itself as an IPC thread waiting for a message,
// hence it doesn't get picked up in the list_for_each_entry_safe() loop above.

void wait_queue_notify(wait_queue_t waitq,
        event64_t event)
{
	xnu_wait_queue_t        walked_waitq        = duct__wait_queue_walkup (waitq, event);
	wake_up(&walked_waitq->linux_waitqh);
}

thread_t
waitq_wakeup64_identity_locked(
        waitq_t wq,
        event64_t event,
        wait_result_t result,
        boolean_t unlock)
{
	thread_t receiver;
	assert (wait_queue_held (waitq));

	// _wait_queue_select64_one
	xnu_wait_queue_t        walked_waitq        = duct__wait_queue_walkup (waitq, event);

	printk ( KERN_NOTICE "- waitq: 0x%p, walked: 0x%p\n",
			 waitq, walked_waitq);

	// printk (KERN_NOTICE "- walked_waitq->linux_waitqh: 0x%p\n", &(walked_waitq->linux_waitqh));

	// __wake_up (&walked_waitq->linux_waitqh, TASK_NORMAL, 1, (void *) event);
	unsigned long   flags;
	spin_lock_irqsave (&walked_waitq->linux_waitqh.lock, flags);

	receiver        = THREAD_NULL;
	linux_wait_queue_t    * lwait_curr;
	linux_wait_queue_t    * lwait_next;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
	list_for_each_entry_safe (lwait_curr, lwait_next, &walked_waitq->linux_waitqh.head, entry)
#else
	list_for_each_entry_safe (lwait_curr, lwait_next, &walked_waitq->linux_waitqh.task_list, task_list)
#endif
        {
			if ( lwait_curr->private) {
				struct task_struct    * ltask   = lwait_curr->private;
				receiver    = darling_thread_get(ltask->pid);

				if (receiver != NULL)
					thread_lock(receiver);
				else
					printf("- BUG? Cannot darling_thread_get(%d)\n", ltask->pid);

				if (lwait_curr->func (lwait_curr, TASK_NORMAL, 0, (void *) event) )
					break;
				if (receiver != NULL)
					thread_unlock(receiver);
			}
	}

	spin_unlock_irqrestore (&walked_waitq->linux_waitqh.lock, flags);

	printf ("- receiver: 0x%p\n", receiver);
	return receiver;
}

kern_return_t
waitq_wakeup64_one_locked(
        waitq_t wq,
        event64_t event,
        wait_result_t result,
        boolean_t unlock)
{
        printk (KERN_NOTICE "BUG: waitq_wakeup64_one_locked () called\n");
        return 0;
}


xnu_waitq_t duct__waitq_walkup (xnu_waitq_t waitq, event64_t event)
{
        waitq_element_t    wqe;
        waitq_element_t    wqe_next;

        queue_t                 q   = &waitq->waitq_queue;

        wqe     = (waitq_element_t) queue_first(q);
        while (!queue_end (q, (queue_entry_t) wqe)) {
                WAIT_QUEUE_ELEMENT_CHECK (waitq, wqe);

                wqe_next    = (waitq_element_t) queue_next ((queue_t) wqe);

                /*
                 * We may have to recurse if this is a compound wait queue.
                 */
                if (wqe->wqe_type == WAIT_QUEUE_LINK || wqe->wqe_type == WAIT_QUEUE_LINK_NOALLOC) {

                        waitq_link_t   wql         = (waitq_link_t) wqe;
                        waitq_set_t    set_queue   = wql->wql_setqueue;

                        /*
                         * We have to check the set wait queue. If the set
                         * supports pre-posting, it isn't already preposted,
                         * and we didn't find a thread in the set, then mark it.
                         *
                         * If we later find a thread, there may be a spurious
                         * pre-post here on this set.  The wait side has to check
                         * for that either pre- or post-wait.
                         */

                        wqs_lock (set_queue);

                        // WC - todo we did not recurivlly walk up but only support one level port->pset
                        if (event == NO_EVENT64 && set_queue->wqs_prepost && !wql_is_preposted (wql)) {
                                queue_t     ppq     = &set_queue->wqs_preposts;
                                queue_enter (ppq, wql, waitq_link_t, wql_preposts);
                        }

                        // WC: assuming that there is only one level from port->pset
                        wqs_unlock (set_queue);
                        return &set_queue->wqs_waitq;
                        // return duct__waitq_walkup (&set_queue->wqs_waitq);

                        // if (set_waitq != NULL) {
                        //         wqs_unlock (set_queue);
                        //         return set_waitq;
                        // }
                        // wqs_unlock (set_queue);
                        // continue;
                }

                /*
                 * Otherwise, its a thread.  If it is waiting on
                 * the event we are posting to this queue, pull
                 * it off the queue and stick it in out wake_queue.
                 */
                // t = (thread_t)wq_element;
                // if (t->wait_event == event) {
                //         thread_lock(t);
                //         remqueue((queue_entry_t) t);
                //         t->waitq = WAIT_QUEUE_NULL;
                //         t->wait_event = NO_EVENT64;
                //         t->at_safe_point = FALSE;
                //         return t;    /* still locked */
                // }
                return waitq;
        }

        return waitq;
}

int duct_autoremove_wake_function (linux_wait_queue_t * lwait, unsigned mode, int sync, void * key)
{
        struct task_struct    * ltask   = lwait->private;
        thread_t                thread;

		thread = darling_thread_get(ltask->pid);
        printk ( KERN_NOTICE "- thread: 0x%p wakeup_event: 0x%llx, thread's wait_event: 0x%llx\n",
                 thread, CAST_EVENT64_T (key), thread->wait_event );

        if ((CAST_EVENT64_T (key) != thread->wait_event)) {
                return 0;
        }

        // thread_lock (thread);
        thread->wait_event      = DUCT_EVENT64_READY;
        // thread_unlock (thread);

        return autoremove_wake_function (lwait, mode, sync, key);
}

/*
 *	Routine:	waitq_member_locked
 *	Purpose:
 *		Indicate if this set queue is a member of the queue
 *	Conditions:
 *		The wait queue is locked
 *		The set queue is just that, a set queue
 */
static boolean_t
waitq_member_locked(
	waitq_t wq,
	waitq_set_t wq_set)
{
	waitq_element_t wq_element;
	queue_t q;

	assert(waitq_held(wq));
	assert(waitq_is_set(wq_set));

	q = &wq->waitq_queue;

	wq_element = (waitq_element_t) queue_first(q);
	while (!queue_end(q, (queue_entry_t)wq_element)) {
		WAIT_QUEUE_ELEMENT_CHECK(wq, wq_element);
		if ((wq_element->wqe_type == WAIT_QUEUE_LINK) ||
		    (wq_element->wqe_type == WAIT_QUEUE_LINK_NOALLOC)) {
			waitq_link_t wql = (waitq_link_t)wq_element;

			if (wql->wql_setqueue == wq_set)
				return TRUE;
		}
		wq_element = (waitq_element_t)
			     queue_next((queue_t) wq_element);
	}
	return FALSE;
}
	

/*
 *	Routine:	waitq_member
 *	Purpose:
 *		Indicate if this set queue is a member of the queue
 *	Conditions:
 *		The set queue is just that, a set queue
 */
boolean_t
waitq_member(
	waitq_t wq,
	waitq_set_t wq_set)
{
	boolean_t ret;

	if (!waitq_is_set(wq_set))
		return FALSE;

	waitq_lock(wq);
	ret = waitq_member_locked(wq, wq_set);
	waitq_unlock(wq);

	return ret;
}

kern_return_t
wait_queue_set_unlink_all_nofree(
        wait_queue_set_t wq_set,
        queue_t         links)
{
        wait_queue_link_t wql;
        wait_queue_t wq;
        queue_t q;
        spl_t s;

        if (!wait_queue_is_set(wq_set)) {
                return KERN_INVALID_ARGUMENT;
        }

retry:
        s = splsched();
        wqs_lock(wq_set);

        /* remove the wait queues that are members of our set */
        q = &wq_set->wqs_setlinks;

        wql = (wait_queue_link_t)queue_first(q);
        while (!queue_end(q, (queue_entry_t)wql)) {
                WAIT_QUEUE_SET_LINK_CHECK(wq_set, wql);
                wq = wql->wql_queue;
                if (wait_queue_lock_try(wq)) {
                        duct_wait_queue_unlink_locked(wq, wq_set, wql);
                        wait_queue_unlock(wq);
                        enqueue(links, &wql->wql_links);
                        wql = (wait_queue_link_t)queue_first(q);
                } else {
                        wqs_unlock(wq_set);
                        splx(s);
                        delay(1);
                        goto retry;
                }
        }

        /* remove this set from sets it belongs to */
        wait_queue_unlink_all_nofree_locked(&wq_set->wqs_wait_queue, links);

        wqs_unlock(wq_set);
        splx(s);

        return(KERN_SUCCESS);
}

