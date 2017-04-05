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
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/printk.h>
#include <linux/rwlock.h>
#include <linux/atomic.h>

static rwlock_t my_task_lock, my_thread_lock;
static struct rb_root all_tasks = RB_ROOT;
static struct rb_root all_threads = RB_ROOT;
static unsigned int task_count = 0;

struct registry_entry
{
	struct rb_node node;
	int tid;

	union
	{
		task_t task;
		thread_t thread;
	};

	void* keys[TASK_KEY_COUNT];
	task_key_dtor dtors[TASK_KEY_COUNT];
};

void darling_task_init(void)
{
	rwlock_init(&my_task_lock);
	rwlock_init(&my_thread_lock);
}

static struct registry_entry* darling_task_get_current_entry(void)
{
	struct registry_entry* ret = NULL;
	struct rb_node* node;
	
	read_lock(&my_task_lock);
	
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
			ret = entry;
			break;
		}
	}
	
	read_unlock(&my_task_lock);
	return ret;
}

task_t darling_task_get_current(void)
{
	struct registry_entry* ret = darling_task_get_current_entry();
	return (ret) ? ret->task : NULL;
}

thread_t darling_thread_get_current(void)
{
	return darling_thread_get(current->pid);
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

	entry = (struct registry_entry*) kmalloc(sizeof(struct registry_entry), GFP_KERNEL);
	entry->task = task;
	entry->tid = current->tgid;

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
			kfree(entry);

			if (task != NULL)
			{
				// Overwrite existing task entry
				printk(KERN_WARNING "darling_task_register() called twice with "
						"non-null task?!\n");

				this->task = task;
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

	entry = (struct registry_entry*) kmalloc(sizeof(struct registry_entry), GFP_KERNEL);
	entry->thread = thread;
	entry->tid = current->pid;

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
			kfree(entry);

			if (thread != NULL)
			{
				if (this->thread != thread)
				{
					// Overwrite existing thread entry
					printk(KERN_WARNING "darling_thread_register() called twice with "
							"non-null task?!\n");

					this->thread = thread;
				}
			}
			else
			{
				// Remove task from tree
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
			int i;

			for (i = 0; i < TASK_KEY_COUNT; i++)
			{
				if (entry->keys[i] != NULL)
					entry->dtors[i](entry->keys[i]);
			}

			rb_erase(node, &all_tasks);
			kfree(entry);
			task_count--;
			break;
		}
	}

	write_unlock(&my_task_lock);
}

void darling_thread_deregister(thread_t t)
{
	struct rb_node *node, *parent = NULL;
	write_lock(&my_thread_lock);

	node = all_threads.rb_node;

	while (node)
	{
		struct registry_entry* entry = container_of(node, struct registry_entry, node);
		
		if (current->pid < entry->tid)
			node = node->rb_left;
		else if (current->pid > entry->tid)
			node = node->rb_right;
		else
		{
			rb_erase(node, &all_threads);
			kfree(entry);
			break;
		}
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

