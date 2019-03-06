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

static struct rw_semaphore my_task_lock;
static struct rw_semaphore my_thread_lock;

static struct rb_root all_tasks = RB_ROOT;
static struct rb_root all_threads = RB_ROOT;
static unsigned int task_count = 0;

static struct registry_entry* darling_task_get_entry_unlocked(int pid);

#define THREAD_CANCELDISABLED      0x1
#define THREAD_CANCELED            0x2

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
    init_rwsem(&my_task_lock);
    init_rwsem(&my_thread_lock);
}

static inline void obtain_read_lock(struct rw_semaphore* l)
{
    down_read(l);
}

static inline void release_read_lock(struct rw_semaphore* l)
{
    up_read(l);
}

static inline void obtain_write_lock(struct rw_semaphore* l)
{
    down_write(l);
}

static inline void release_write_lock(struct rw_semaphore* l)
{
    up_write(l);
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
    obtain_read_lock(&my_task_lock);

    struct registry_entry* e = darling_task_get_entry_unlocked(pid);

    if(e != NULL)
        darling_task_post_notification_internal(e, event, extra);

    release_read_lock(&my_task_lock);
}

static struct registry_entry* darling_task_get_entry_unlocked(int pid)
{
    struct registry_entry* ret = NULL;
    struct rb_node* node;

    node = all_tasks.rb_node;

    while (node)
    {
        struct registry_entry* entry = container_of(node, struct registry_entry, node);

        if(pid < entry->tid)
            node = node->rb_left;
        else if(pid > entry->tid)
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
    obtain_read_lock(&my_task_lock);

    struct registry_entry* e = darling_task_get_entry_unlocked(current->tgid);
    task_t ret = (e) ? e->task : NULL;

    release_read_lock(&my_task_lock);

    return ret;
}

extern void task_reference_wrapper(task_t);
task_t darling_task_get(int pid)
{
    obtain_read_lock(&my_task_lock);

    struct registry_entry* e = darling_task_get_entry_unlocked(pid);

    task_t ret = NULL;
    if(e != NULL)
    {
        task_reference_wrapper(e->task);
        ret = e->task;
    }

    release_read_lock(&my_task_lock);

    return ret;
}

struct registry_entry* darling_thread_get_entry_unlocked(unsigned int pid)
{
    struct registry_entry* ret = NULL;
    struct rb_node* node;

    node = all_threads.rb_node;

    while (node)
    {
        struct registry_entry* entry = container_of(node, struct registry_entry, node);

        if(pid < entry->tid)
            node = node->rb_left;
        else if(pid > entry->tid)
            node = node->rb_right;
        else
        {
            ret = entry;
            break;
        }
    }

    return ret;
}

thread_t darling_thread_get_current(void)
{
    obtain_read_lock(&my_thread_lock);

    struct registry_entry* e = darling_thread_get_entry_unlocked(current->pid);
    thread_t t = (e) ? e->thread : NULL;

    release_read_lock(&my_thread_lock);

    return t;
}

thread_t darling_thread_get(unsigned int pid)
{
    thread_t ret = NULL;
    struct rb_node* node;

    obtain_read_lock(&my_thread_lock);

    node = all_threads.rb_node;

    while (node)
    {
        struct registry_entry* entry = container_of(node, struct registry_entry, node);

        if(pid < entry->tid)
            node = node->rb_left;
        else if(pid > entry->tid)
            node = node->rb_right;
        else
        {
            ret = entry->thread;
            break;
        }
    }

    release_read_lock(&my_thread_lock);

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

    obtain_write_lock(&my_task_lock);
    new = &all_tasks.rb_node;

    while (*new)
    {
        struct registry_entry* this = container_of(*new, struct registry_entry, node);
        parent = *new;

        if(entry->tid < this->tid)
            new = &(*new)->rb_left;
        else if(entry->tid > this->tid)
            new = &(*new)->rb_right;
        else // Tree already contains this tgid
        {
            debug_msg("Replacing task %p -> %p\n", this->task, task);
            if(task != NULL)
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
            release_write_lock(&my_task_lock);
            return;
        }
    }

    rb_link_node(&entry->node, parent, new);
    rb_insert_color(&entry->node, &all_tasks);
    task_count++;

    release_write_lock(&my_task_lock);
}


void darling_thread_register(thread_t thread)
{
    struct registry_entry* entry;
    struct rb_node **new, *parent = NULL;

    entry = (struct registry_entry*) kzalloc(sizeof(struct registry_entry), GFP_KERNEL);
    entry->thread = thread;
    entry->tid = current->pid;
    entry->flags = 0;

    obtain_write_lock(&my_thread_lock);
    new = &all_threads.rb_node;

    while (*new)
    {
        struct registry_entry* this = container_of(*new, struct registry_entry, node);
        parent = *new;

        if(entry->tid < this->tid)
            new = &(*new)->rb_left;
        else if(entry->tid > this->tid)
            new = &(*new)->rb_right;
        else // Tree already contains this tid
        {
            if(thread != NULL)
            {
                if(this->thread != thread)
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
            release_write_lock(&my_thread_lock);
            return;
        }
    }

    rb_link_node(&entry->node, parent, new);
    rb_insert_color(&entry->node, &all_threads);

    release_write_lock(&my_thread_lock);
}

void darling_task_free(struct registry_entry* entry)
{
    int i;
    struct list_head *pos, *tmp;

    darling_task_post_notification_internal(entry, NOTE_EXIT, current->exit_code);

    list_for_each_safe(pos, tmp, &entry->proc_notification)
    {
        struct proc_notification* dn = list_entry(pos, struct proc_notification, list);
        kfree(dn);
    }

    for (i = 0; i < TASK_KEY_COUNT; i++)
    {
        if(entry->keys[i] != NULL)
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
    obtain_write_lock(&my_task_lock);

    node = all_tasks.rb_node;

    while (node)
    {
        struct registry_entry* entry = container_of(node, struct registry_entry, node);

        if(current->tgid < entry->tid)
            node = node->rb_left;
        else if(current->tgid > entry->tid)
            node = node->rb_right;
        else
        {
            if(entry->task != t)
                break;

            rb_erase(node, &all_tasks);
            task_count--;

            release_write_lock(&my_task_lock);
            darling_task_free(entry);

            return;
        }
    }

    release_write_lock(&my_task_lock);
}

void darling_thread_deregister(thread_t t)
{
    obtain_write_lock(&my_thread_lock);

    struct rb_node* node = rb_first(&all_threads);

    while (node)
    {
        struct registry_entry* entry = container_of(node, struct registry_entry, node);

        if(entry->thread == t)
        {
            debug_msg("deregistering thread %p\n", t);
            rb_erase(node, &all_threads);
            kfree(entry);
            break;
        }
        node = rb_next(node);
    }

    release_write_lock(&my_thread_lock);
}

bool darling_task_key_set(unsigned int key, void* value, task_key_dtor dtor)
{
    bool ret = false;

    obtain_read_lock(&my_task_lock);
    struct registry_entry* e = darling_task_get_entry_unlocked(current->tgid);

    if(e->keys[key] == NULL)
    {
        if(cmpxchg(&e->keys[key], NULL, value) == NULL)
        {
            e->dtors[key] = dtor;
            ret = true;
        }
    }

    release_read_lock(&my_task_lock);

    return ret;
}

void* darling_task_key_get(unsigned int key)
{
    obtain_read_lock(&my_task_lock);

    struct registry_entry* e = darling_task_get_entry_unlocked(current->tgid);
    void* ret = (e) ? e->keys[key] : NULL;

    release_read_lock(&my_task_lock);

    return ret;
}

void darling_task_fork_wait_for_child(void)
{
    obtain_read_lock(&my_task_lock);

    struct registry_entry* e = darling_task_get_entry_unlocked(current->tgid);
    struct semaphore* sem = &e->sem_fork;

    release_read_lock(&my_task_lock);

    down_interruptible(sem);
}

void darling_task_fork_child_done(void)
{
    obtain_read_lock(&my_task_lock);

    struct registry_entry* entry = darling_task_get_entry_unlocked(current->real_parent->tgid);

    debug_msg("task_fork_child_done - notify entry %p\n", entry);
    if(entry != NULL)
        up(&entry->sem_fork);

    release_read_lock(&my_task_lock);
}

_Bool darling_task_notify_register(unsigned int pid, struct evprocfd_ctx* efd)
{
    struct proc_notification* dn;
    struct registry_entry* e;
    _Bool rv = false;

    obtain_read_lock(&my_task_lock);

    e = darling_task_get_entry_unlocked(pid);

    if(e == NULL)
        goto out;

    dn = (struct proc_notification*) kmalloc(sizeof(*dn), GFP_KERNEL);
    dn->efd = efd;

    mutex_lock(&e->mut_proc_notification);
    list_add(&dn->list, &e->proc_notification);
    mutex_unlock(&e->mut_proc_notification);

    rv = true;

out:
    release_read_lock(&my_task_lock);
    return rv;
}

_Bool darling_task_notify_deregister(unsigned int pid, struct evprocfd_ctx* efd)
{
    struct registry_entry* e;
    _Bool rv = false;
    struct list_head* next;

    obtain_read_lock(&my_task_lock);

    e = darling_task_get_entry_unlocked(pid);

    if(e == NULL)
        goto out;

    mutex_lock(&e->mut_proc_notification);
    list_for_each(next, &e->proc_notification)
    {
        struct proc_notification* dn = list_entry(next, struct proc_notification, list);

        if(dn->efd == efd)
        {
            rv = true;
            list_del(&dn->list);
            kfree(dn);
            break;
        }
    }
    mutex_unlock(&e->mut_proc_notification);

out:
    release_read_lock(&my_task_lock);
    return rv;
}

void darling_task_set_dyld_info(unsigned long all_img_location, unsigned long all_img_length)
{
    obtain_write_lock(&my_task_lock);

    struct registry_entry* e = darling_task_get_entry_unlocked(current->tgid);
    e->proc_dyld_info_allimg_loc = all_img_location;
    e->proc_dyld_info_allimg_len = all_img_length;

    complete_all(&e->proc_dyld_info);

    release_write_lock(&my_task_lock);
}

void darling_task_get_dyld_info(unsigned int pid, unsigned long long* all_img_location, unsigned long long* all_img_length)
{
    struct registry_entry* e;

    *all_img_location = *all_img_length = 0;
    obtain_read_lock(&my_task_lock);

    e = darling_task_get_entry_unlocked(pid);
    if(e != NULL)
    {
        if(wait_for_completion_interruptible(&e->proc_dyld_info) == 0)
        {
            *all_img_location = e->proc_dyld_info_allimg_loc;
            *all_img_length = e->proc_dyld_info_allimg_len;
        }
    }

    release_read_lock(&my_task_lock);
}

// Will send SIGSTOP at next darling_task_set_dyld_info() call
void darling_task_mark_start_suspended(void)
{
    obtain_write_lock(&my_task_lock);

    struct registry_entry* e = darling_task_get_entry_unlocked(current->tgid);
    e->do_sigstop = true;

    release_write_lock(&my_task_lock);
}

_Bool darling_task_marked_start_suspended(void)
{
    obtain_read_lock(&my_task_lock);

    struct registry_entry* e = darling_task_get_entry_unlocked(current->tgid);
    _Bool ret = e->do_sigstop;

    release_read_lock(&my_task_lock);

    return ret;
}

_Bool darling_thread_canceled(void)
{
    _Bool ret = false;

    obtain_read_lock(&my_thread_lock);

    struct registry_entry* e = darling_thread_get_entry_unlocked(current->pid);
    if(e != NULL)
    {
        debug_msg("flags = %d\n", e->flags);
        ret = (e->flags & (THREAD_CANCELDISABLED|THREAD_CANCELED)) == THREAD_CANCELED;
    }

    release_read_lock(&my_thread_lock);

    return ret;
}

void darling_thread_cancelable(_Bool cancelable)
{
    obtain_read_lock(&my_thread_lock);

    struct registry_entry* e = darling_thread_get_entry_unlocked(current->pid);
    if(e != NULL)
    {
        if(cancelable)
            e->flags &= ~THREAD_CANCELDISABLED;
        else
            e->flags |= THREAD_CANCELDISABLED;
    }

    release_read_lock(&my_thread_lock);
}

extern struct task_struct* thread_get_linux_task(thread_t thread);
void darling_thread_markcanceled(unsigned int pid)
{
    struct registry_entry* e;

    debug_msg("Marking thread %d as canceled\n", pid);

    obtain_read_lock(&my_thread_lock);
    e = darling_thread_get_entry_unlocked(pid);

    if(e != NULL && !(e->flags & THREAD_CANCELDISABLED))
    {
        e->flags |= THREAD_CANCELED;

        // FIXME: We should be calling wake_up_state(TASK_INTERRUPTIBLE),
        // but this function is not exported.
        thread_t t = e->thread;
        release_read_lock(&my_thread_lock);

        wake_up_process(thread_get_linux_task(t));
    }
    else
    {
        release_read_lock(&my_thread_lock);
    }
}

