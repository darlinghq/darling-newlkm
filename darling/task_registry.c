/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2015-2017 Lubos Dolezel
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <linux/types.h>
#include "task_registry.h"
#include "evprocfd.h"
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#else
#include <linux/sched.h>
#endif
#include <linux/completion.h>
#include <linux/printk.h>
#include <linux/hashtable.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/semaphore.h>
#include <linux/list.h>
#include <linux/eventfd.h>
#include <linux/mutex.h>
#include "debug_print.h"

DEFINE_SPINLOCK(my_task_lock);
DEFINE_SPINLOCK(my_thread_lock);
DEFINE_HASHTABLE(all_tasks, 6);
DEFINE_HASHTABLE(all_threads, 6);
static unsigned int task_count = 0;

static struct registry_entry* darling_task_get_entry_unlocked(int pid);

#define THREAD_CANCELDISABLED	0x1
#define THREAD_CANCELED			0x2

struct registry_entry
{
	struct hlist_node node;
	int tid;

	union
	{
		struct
		{
			task_t task;
			void* keys[TASK_KEY_COUNT];
			task_key_dtor dtors[TASK_KEY_COUNT];
			
			struct semaphore sem_fork;
			struct list_head proc_notification;
			struct mutex mut_proc_notification;
			struct completion proc_dyld_info;
			unsigned long proc_dyld_info_allimg_loc, proc_dyld_info_allimg_len;
			bool do_sigstop;
		};

		struct
		{
			thread_t thread;
			int flags;
		};
	};

};

struct proc_notification
{
	struct list_head list;
	struct evprocfd_ctx* efd;
};

void darling_task_init(void)
{
}

static void darling_task_post_notification_internal(struct registry_entry* entry, unsigned int event, unsigned int extra)
{
	struct list_head *pos, *tmp;

	debug_msg("Posting notification 0x%x for task %p\n", event, entry->task);
	
	mutex_lock(&entry->mut_proc_notification);
	
	list_for_each_safe(pos, tmp, &entry->proc_notification)
	{
		struct proc_notification* dn = list_entry(pos, struct proc_notification, list);
		evprocfd_notify(dn->efd, event, extra);
	}
	
	mutex_unlock(&entry->mut_proc_notification);
}

void darling_task_post_notification(unsigned int pid, unsigned int event, unsigned int extra)
{
	struct registry_entry* e;
	rcu_read_lock();

	e = darling_task_get_entry_unlocked(pid);
	
	rcu_read_unlock();
	if (e != NULL)
		darling_task_post_notification_internal(e, event, extra);
}

static struct registry_entry* darling_task_get_current_entry(void)
{
	rcu_read_lock();
	struct registry_entry* e = darling_task_get_entry_unlocked(current->tgid);
	rcu_read_unlock();
	return e;
}

static struct registry_entry* darling_task_get_entry_unlocked(int pid)
{
	struct registry_entry* ret;

	hash_for_each_possible_rcu(all_tasks, ret, node, pid)
	{
		if (ret->tid == pid)
			return ret;
	}

	return NULL;
}

task_t darling_task_get_current(void)
{
	struct registry_entry* ret = darling_task_get_current_entry();
	return (ret) ? ret->task : NULL;
}

extern void task_reference_wrapper(task_t);
task_t darling_task_get(int pid)
{
	task_t task = NULL;
	rcu_read_lock();

	struct registry_entry* ret = darling_task_get_entry_unlocked(pid);
	if (ret != NULL)
	{
		task_reference_wrapper(ret->task);
		task = ret->task;
	}

	rcu_read_unlock();

	return task;
}

// Requires read_lock(&my_thread_lock)
struct registry_entry* darling_thread_get_entry(unsigned int pid)
{
	struct registry_entry* ret;

	hash_for_each_possible_rcu(all_threads, ret, node, pid)
	{
		if (ret->tid == pid)
			return ret;
	}

	return NULL;
}

struct registry_entry* darling_thread_get_current_entry(void)
{
	struct registry_entry* e;

	rcu_read_lock();
	e = darling_thread_get_entry(current->pid);
	rcu_read_unlock();

	return e;
}

thread_t darling_thread_get_current(void)
{
	struct registry_entry* e = darling_thread_get_current_entry();
	if (!e)
	{
		debug_msg("No current thread in registry!\n");
		return NULL;
	}
	return e->thread;
}

/*
thread_t darling_thread_get(unsigned int pid)
{
	struct registry_entry* e;

	rcu_read_lock();
	e = darling_thread_get_entry(current->pid);
	rcu_read_unlock();

	if (e != NULL)
		return e->thread;
	return NULL;
}
*/

void darling_task_register(task_t task)
{
	struct registry_entry *entry, *this;

	rcu_read_lock();
	hash_for_each_possible_rcu(all_tasks, this, node, current->tgid)
	{
		if (this->tid == current->tgid)
		{
			debug_msg("Replacing task %p -> %p\n", this->task, task);
			
			this->task = task;
				
			// exec case
			darling_task_post_notification_internal(this, NOTE_EXEC, 0);
			
			rcu_read_unlock();
			return;
		}
	}

	rcu_read_unlock();

	entry = (struct registry_entry*) kzalloc(sizeof(struct registry_entry), GFP_KERNEL);
	entry->task = task;
	entry->tid = current->tgid;

	sema_init(&entry->sem_fork, 0);
	INIT_LIST_HEAD(&entry->proc_notification);
	mutex_init(&entry->mut_proc_notification);
	init_completion(&entry->proc_dyld_info);

	spin_lock(&my_task_lock);
	hash_add_rcu(all_tasks, &entry->node, entry->tid);
	task_count++;
	spin_unlock(&my_task_lock);
}


void darling_thread_register(thread_t thread)
{
	struct registry_entry *entry, *this;

	rcu_read_lock();

	hash_for_each_possible_rcu(all_threads, this, node, current->pid)
	{
		if (this->tid == current->pid)
		{
			if (this->thread != thread)
			{
				// Overwrite existing thread entry
				debug_msg("Overwriting thread entry %p -> %p\n", this->thread, thread);
				this->thread = thread;
			}
			
			rcu_read_unlock();
			return;
		}
	}

	rcu_read_unlock();

	entry = (struct registry_entry*) kzalloc(sizeof(struct registry_entry), GFP_KERNEL);
	entry->thread = thread;
	entry->tid = current->pid;
	entry->flags = 0;

	spin_lock(&my_thread_lock);
	hash_add_rcu(all_threads, &entry->node, entry->tid);
	spin_unlock(&my_thread_lock);
}

void darling_task_free(struct registry_entry* entry)
{
	int i;
	struct list_head *pos, *tmp;

	list_for_each_safe(pos, tmp, &entry->proc_notification)
	{
		struct proc_notification* dn = list_entry(pos, struct proc_notification, list);
		kfree(dn);
	}

	for (i = 0; i < TASK_KEY_COUNT; i++)
	{
		if (entry->keys[i] != NULL)
		{
			// printk(KERN_NOTICE "ptr %d = %p\n", i, entry->keys[i]);
			entry->dtors[i](entry->keys[i]);
		}
	}

	kfree(entry);
}

void darling_task_deregister(task_t t)
{
	struct registry_entry *entry;

	spin_lock(&my_task_lock);

	hash_for_each_possible(all_tasks, entry, node, current->tgid)
	{
		if (entry->tid == current->tgid)
		{
			if (entry->task != t)
				break;
			
			hash_del_rcu(&entry->node);
			task_count--;
			spin_unlock(&my_task_lock);
			synchronize_rcu();

			darling_task_free(entry);
			return;
		}
	}

	spin_unlock(&my_task_lock);
}

void darling_thread_deregister(thread_t t)
{
	struct registry_entry *entry;
	unsigned int temp;

	spin_lock(&my_thread_lock);

	hash_for_each(all_threads, temp, entry, node)
	{
		if (entry->thread == t)
		{
			debug_msg("deregistering thread %p\n", t);

			hash_del_rcu(&entry->node);
			spin_unlock(&my_thread_lock);
			synchronize_rcu();

			kfree(entry);
			return;
		}
	}

	spin_unlock(&my_thread_lock);
}

bool darling_task_key_set(unsigned int key, void* value, task_key_dtor dtor)
{
	struct registry_entry* entry;
	entry = darling_task_get_current_entry();

	if (entry->keys[key] != NULL)
		return false;

	if (cmpxchg(&entry->keys[key], NULL, value) != NULL)
		return false;

	entry->dtors[key] = dtor;

	return true;
}

void* darling_task_key_get(unsigned int key)
{
	struct registry_entry* entry;
	entry = darling_task_get_current_entry();
	return entry ? entry->keys[key] : NULL;
}

void darling_task_fork_wait_for_child(void)
{
	struct registry_entry* e = darling_task_get_current_entry();

	down_interruptible(&e->sem_fork);
}

void darling_task_fork_child_done(void)
{
	rcu_read_lock();
	struct registry_entry* entry = darling_task_get_entry_unlocked(current->real_parent->tgid);

	debug_msg("task_fork_child_done - notify entry %p\n", entry);
	if (entry != NULL)
		up(&entry->sem_fork);
	rcu_read_unlock();
}

_Bool darling_task_notify_register(unsigned int pid, struct evprocfd_ctx* efd)
{
	struct proc_notification* dn;
	struct registry_entry* e;
	_Bool rv = false;

	rcu_read_lock();

   	e = darling_task_get_entry_unlocked(pid);
   	
	if (e == NULL)
		goto out;

	dn = (struct proc_notification*) kmalloc(sizeof(*dn), GFP_KERNEL);
	dn->efd = efd;

	mutex_lock(&e->mut_proc_notification);
	list_add(&dn->list, &e->proc_notification);
	mutex_unlock(&e->mut_proc_notification);
	
	rv = true;

out:
	rcu_read_unlock();
	return rv;
}

_Bool darling_task_notify_deregister(unsigned int pid, struct evprocfd_ctx* efd)
{
	struct registry_entry* e;
	_Bool rv = false;
	struct list_head* next;

	rcu_read_lock();

	e = darling_task_get_entry_unlocked(pid);

	if (e == NULL)
	{
		debug_msg("darling_task_notify_deregister failed to get task for PID %d\n", pid);
		goto out;
	}

	mutex_lock(&e->mut_proc_notification);
	list_for_each(next, &e->proc_notification)
	{
		struct proc_notification* dn = list_entry(next, struct proc_notification, list);

		if (dn->efd == efd)
		{
			debug_msg("darling_task_notify_deregister found proc notification entry for ctx %p, deleting it...\n", efd);
			rv = true;
			list_del(&dn->list);
			kfree(dn);
			break;
		}
	}
	mutex_unlock(&e->mut_proc_notification);

out:
	rcu_read_unlock();
	return rv;
}

void darling_task_set_dyld_info(unsigned long all_img_location, unsigned long all_img_length)
{
	struct registry_entry* e = darling_task_get_current_entry();

	e->proc_dyld_info_allimg_loc = all_img_location;
	e->proc_dyld_info_allimg_len = all_img_length;

	complete_all(&e->proc_dyld_info);
}

void darling_task_get_dyld_info(unsigned int pid, unsigned long long* all_img_location, unsigned long long* all_img_length)
{
	struct registry_entry* e;

	*all_img_location = *all_img_length = 0;
	rcu_read_lock();

	e = darling_task_get_entry_unlocked(pid);
	if (e != NULL)
	{
		if (wait_for_completion_interruptible(&e->proc_dyld_info) == 0)
		{
			*all_img_location = e->proc_dyld_info_allimg_loc;
			*all_img_length = e->proc_dyld_info_allimg_len;
		}
	}

	rcu_read_unlock();
}

// Will send SIGSTOP at next darling_task_set_dyld_info() call
void darling_task_mark_start_suspended(void)
{
	struct registry_entry* e = darling_task_get_current_entry();
	e->do_sigstop = true;
}

_Bool darling_task_marked_start_suspended(void)
{
	struct registry_entry* e = darling_task_get_current_entry();
	return e->do_sigstop;
}

_Bool darling_thread_canceled(void)
{
	struct registry_entry* e = darling_thread_get_current_entry();
	if (e != NULL)
	{
		debug_msg("flags = %d\n", e->flags);
		return (e->flags & (THREAD_CANCELDISABLED|THREAD_CANCELED)) == THREAD_CANCELED;
	}
	else
		return false;
}

void darling_thread_cancelable(_Bool cancelable)
{
	struct registry_entry* e = darling_thread_get_current_entry();
	if (e != NULL)
	{
		if (cancelable)
			e->flags &= ~THREAD_CANCELDISABLED;
		else
			e->flags |= THREAD_CANCELDISABLED;
	}
}

extern struct task_struct* thread_get_linux_task(thread_t thread);
void darling_thread_markcanceled(unsigned int pid)
{
	struct registry_entry* e;

	debug_msg("Marking thread %d as canceled\n", pid);

	rcu_read_lock();
	e = darling_thread_get_entry(pid);

	if (e != NULL && !(e->flags & THREAD_CANCELDISABLED))
	{
		e->flags |= THREAD_CANCELED;

		// FIXME: We should be calling wake_up_state(TASK_INTERRUPTIBLE),
		// but this function is not exported.
		wake_up_process(thread_get_linux_task(e->thread));
	}
	rcu_read_unlock();
}

