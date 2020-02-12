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
#include <linux/rwlock.h>
#include <linux/atomic.h>
#include <linux/semaphore.h>
#include <linux/list.h>
#include <linux/eventfd.h>
#include <linux/mutex.h>
#include "debug_print.h"

static rwlock_t my_task_lock, my_thread_lock;
static struct rb_root all_tasks = RB_ROOT;
static struct rb_root all_threads = RB_ROOT;
static unsigned int task_count = 0;

static struct registry_entry* darling_task_get_entry_unlocked(int pid);

#define THREAD_CANCELDISABLED	0x1
#define THREAD_CANCELED			0x2

struct registry_entry
{
	struct rb_node node;
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
	rwlock_init(&my_task_lock);
	rwlock_init(&my_thread_lock);
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
	read_lock(&my_task_lock);

	e = darling_task_get_entry_unlocked(pid);
	
	read_unlock(&my_task_lock);
	if (e != NULL)
		darling_task_post_notification_internal(e, event, extra);
}

static struct registry_entry* darling_task_get_current_entry(void)
{
	read_lock(&my_task_lock);
	struct registry_entry* e = darling_task_get_entry_unlocked(current->tgid);
	read_unlock(&my_task_lock);
	return e;
}

static struct registry_entry* darling_task_get_entry_unlocked(int pid)
{
	struct registry_entry* ret = NULL;
	struct rb_node* node;
	
	node = all_tasks.rb_node;
	
	while (node)
	{
		struct registry_entry* entry = container_of(node, struct registry_entry, node);
		
		if (pid < entry->tid)
			node = node->rb_left;
		else if (pid > entry->tid)
			node = node->rb_right;
		else
		{
			ret = entry;
			break;
		}
	}
	
	return ret;
}

task_t darling_task_get_current(void)
{
	struct registry_entry* ret = darling_task_get_current_entry();
	return (ret) ? ret->task : NULL;
}

extern void task_reference_wrapper(task_t);
task_t darling_task_get(int pid)
{
	read_lock(&my_task_lock);

	struct registry_entry* ret = darling_task_get_entry_unlocked(pid);
	if (ret != NULL)
		task_reference_wrapper(ret->task);

	read_unlock(&my_task_lock);
	return (ret) ? ret->task : NULL;
}

// Requires read_lock(&my_thread_lock)
struct registry_entry* darling_thread_get_entry(unsigned int pid)
{
	struct registry_entry* ret = NULL;
	struct rb_node* node;
	
	node = all_threads.rb_node;
	
	while (node)
	{
		struct registry_entry* entry = container_of(node, struct registry_entry, node);
		
		if (pid < entry->tid)
			node = node->rb_left;
		else if (pid > entry->tid)
			node = node->rb_right;
		else
		{
			ret = entry;
			break;
		}
	}
	
	return ret;
}

struct registry_entry* darling_thread_get_current_entry(void)
{
	struct registry_entry* e;

	read_lock(&my_thread_lock);
	e = darling_thread_get_entry(current->pid);
	read_unlock(&my_thread_lock);

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

thread_t darling_thread_get(unsigned int pid)
{
	thread_t ret = NULL;
	struct rb_node* node;
	
	read_lock(&my_thread_lock);
	
	node = all_threads.rb_node;
	
	while (node)
	{
		struct registry_entry* entry = container_of(node, struct registry_entry, node);
		
		if (pid < entry->tid)
			node = node->rb_left;
		else if (pid > entry->tid)
			node = node->rb_right;
		else
		{
			ret = entry->thread;
			break;
		}
	}
	
	read_unlock(&my_thread_lock);
	return ret;
}


void darling_task_register(task_t task)
{
	struct registry_entry* entry;
	struct rb_node **new, *parent = NULL;

	entry = (struct registry_entry*) kzalloc(sizeof(struct registry_entry), GFP_KERNEL);
	entry->task = task;
	entry->tid = current->tgid;

	sema_init(&entry->sem_fork, 0);
	INIT_LIST_HEAD(&entry->proc_notification);
	mutex_init(&entry->mut_proc_notification);
	init_completion(&entry->proc_dyld_info);

	write_lock(&my_task_lock);
	new = &all_tasks.rb_node;

	while (*new)
	{
		struct registry_entry* this = container_of(*new, struct registry_entry, node);
		parent = *new;

		if (entry->tid < this->tid)
			new = &(*new)->rb_left;
		else if (entry->tid > this->tid)
			new = &(*new)->rb_right;
		else // Tree already contains this tgid
		{
			debug_msg("Replacing task %p -> %p\n", this->task, task);
			if (task != NULL)
			{
				this->task = task;
				
				// exec case
				darling_task_post_notification_internal(this, NOTE_EXEC, 0);
			}
			else
			{
				// Remove task from tree
				rb_erase(*new, &all_tasks);
				kfree(this);
				task_count--;
			}

			kfree(entry);
			write_unlock(&my_task_lock);
			return;
		}
	}

	rb_link_node(&entry->node, parent, new);
	rb_insert_color(&entry->node, &all_tasks);
	task_count++;

	write_unlock(&my_task_lock);
}


void darling_thread_register(thread_t thread)
{
	struct registry_entry* entry;
	struct rb_node **new, *parent = NULL;

	entry = (struct registry_entry*) kzalloc(sizeof(struct registry_entry), GFP_KERNEL);
	entry->thread = thread;
	entry->tid = current->pid;
	entry->flags = 0;

	write_lock(&my_thread_lock);
	new = &all_threads.rb_node;

	while (*new)
	{
		struct registry_entry* this = container_of(*new, struct registry_entry, node);
		parent = *new;

		if (entry->tid < this->tid)
			new = &(*new)->rb_left;
		else if (entry->tid > this->tid)
			new = &(*new)->rb_right;
		else // Tree already contains this tid
		{
			if (thread != NULL)
			{
				if (this->thread != thread)
				{
					// Overwrite existing thread entry
					debug_msg("Overwriting thread entry %p -> %p\n", this->thread, thread);
					this->thread = thread;
				}
			}
			else
			{
				debug_msg("Erasing thread %p from tree\n", entry->thread);
				
				// Remove thread from tree
				rb_erase(*new, &all_threads);
				kfree(this);
			}

			kfree(entry);
			write_unlock(&my_thread_lock);
			return;
		}
	}

	rb_link_node(&entry->node, parent, new);
	rb_insert_color(&entry->node, &all_threads);

	write_unlock(&my_thread_lock);
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
	struct rb_node *node, *parent = NULL;

	write_lock(&my_task_lock);

	node = all_tasks.rb_node;

	while (node)
	{
		struct registry_entry* entry = container_of(node, struct registry_entry, node);
		
		if (current->tgid < entry->tid)
			node = node->rb_left;
		else if (current->tgid > entry->tid)
			node = node->rb_right;
		else
		{
			if (entry->task != t)
				break;

			rb_erase(node, &all_tasks);
			task_count--;

			darling_task_free(entry);
			write_unlock(&my_task_lock);

			return;
		}
	}

	write_unlock(&my_task_lock);
}

void darling_thread_deregister(thread_t t)
{
	struct rb_node *node;
	write_lock(&my_thread_lock);

	node = rb_first(&all_threads);

	while (node)
	{
		struct registry_entry* entry = container_of(node, struct registry_entry, node);
		
		if (entry->thread == t)
		{
			debug_msg("deregistering thread %p\n", t);
			rb_erase(node, &all_threads);
			kfree(entry);
			break;
		}
		node = rb_next(node);
	}

	write_unlock(&my_thread_lock);
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
	read_lock(&my_task_lock);
	struct registry_entry* entry = darling_task_get_entry_unlocked(current->real_parent->tgid);
	read_unlock(&my_task_lock);

	debug_msg("task_fork_child_done - notify entry %p\n", entry);
	if (entry != NULL)
		up(&entry->sem_fork);
}

_Bool darling_task_notify_register(unsigned int pid, struct evprocfd_ctx* efd)
{
	struct proc_notification* dn;
	struct registry_entry* e;
	_Bool rv = false;

	read_lock(&my_task_lock);

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
	read_unlock(&my_task_lock);
	return rv;
}

_Bool darling_task_notify_deregister(unsigned int pid, struct evprocfd_ctx* efd)
{
	struct registry_entry* e;
	_Bool rv = false;
	struct list_head* next;

	read_lock(&my_task_lock);

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
	read_unlock(&my_task_lock);
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
	read_lock(&my_task_lock);

	e = darling_task_get_entry_unlocked(pid);
	if (e != NULL)
	{
		if (wait_for_completion_interruptible(&e->proc_dyld_info) == 0)
		{
			*all_img_location = e->proc_dyld_info_allimg_loc;
			*all_img_length = e->proc_dyld_info_allimg_len;
		}
	}

	read_unlock(&my_task_lock);
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

	read_lock(&my_thread_lock);
	e = darling_thread_get_entry(pid);

	if (e != NULL && !(e->flags & THREAD_CANCELDISABLED))
	{
		e->flags |= THREAD_CANCELED;

		// FIXME: We should be calling wake_up_state(TASK_INTERRUPTIBLE),
		// but this function is not exported.
		wake_up_process(thread_get_linux_task(e->thread));
	}
	read_unlock(&my_thread_lock);
}

