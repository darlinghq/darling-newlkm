/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2017 Lubos Dolezel
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
#include "evprocfd.h"
#include "task_registry.h"
#include <duct/compiler/clang/asm-inline.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/anon_inodes.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/sched.h>
#include "api.h"
#include "debug_print.h"

struct evprocfd_ctx
{
	unsigned int pid, flags;
	struct list_head events;
	wait_queue_head_t queue;
	spinlock_t lock;
};

// Queued events
struct evprocfd_event_entry
{
	struct list_head list;
	struct evproc_event e;
	struct file* file;
};

static int evprocfd_release(struct inode* inode, struct file* file);
static unsigned int evprocfd_poll(struct file* file, poll_table* wait);
static ssize_t evprocfd_read(struct file* file, char __user* buf, size_t count, loff_t* ppos);
static ssize_t evprocfd_write(struct file* file, const char __user *buf, size_t count, loff_t* ppos);

static const struct file_operations evprocfd_ops =
{
	.release = evprocfd_release,
	.read = evprocfd_read,
	.write = evprocfd_write,
	.poll = evprocfd_poll,
};

int evprocfd_create(unsigned int pid, unsigned int flags, struct file** file)
{
	struct evprocfd_ctx* ctx;
	int fd, rv = 0;
	char name[32];

	ctx = (struct evprocfd_ctx*) kmalloc(sizeof(*ctx), GFP_KERNEL);
	if (ctx == NULL)
		return -ENOMEM;

	ctx->pid = pid;
	ctx->flags = flags & (NOTE_FORK | NOTE_EXEC | NOTE_EXIT | NOTE_TRACK);
	
	INIT_LIST_HEAD(&ctx->events);
	spin_lock_init(&ctx->lock);
	init_waitqueue_head(&ctx->queue);

	// register with task registry
	if (!darling_task_notify_register(pid, ctx))
	{
		debug_msg("!!! cannot register to monitor PID %d\n", pid);
		kfree(ctx);
		return -ESRCH;
	}

	sprintf(name, "[evprocfd:%d]", pid);

	if (file == NULL)
	{
		fd = anon_inode_getfd(name, &evprocfd_ops, ctx, O_RDWR);
		if (fd < 0)
		{
			darling_task_notify_deregister(pid, ctx);
			kfree(ctx);
		}
	}
	else
	{
		*file = anon_inode_getfile(name, &evprocfd_ops, ctx, O_RDWR);
		if (IS_ERR(*file))
		{
			fd = PTR_ERR(*file);
			*file = NULL;
		}
	}

	if (flags & NOTE_CHILD)
	{
		// Internal trick:
		// libkqueue/evprocfd_notify is telling is it wants us
		// to generate NOTE_CHILD immediately.
		// This is done when NOTE_TRACK is used.
		
		struct pid* p = task_pid(current->real_parent);
		evprocfd_notify(ctx, NOTE_CHILD, pid_vnr(p));
	}

	return fd;
}

void evprocfd_notify(struct evprocfd_ctx* ctx, unsigned int event, unsigned int extra)
{
	struct evprocfd_event_entry* ev;

	debug_msg("Received event 0x%x from PID %d with ctx %p\n", event, ctx->pid, ctx);
	if (event != NOTE_CHILD && !(event & ctx->flags)
		&& !(event == NOTE_FORK || (ctx->flags & NOTE_TRACK))
		&& event != NOTE_EXIT)
	{
		debug_msg("Ignoring event 0x%x from PID %d\n", event, ctx->pid);
		// Userspace isn't interested in this event
		return;
	}

	ev = (struct evprocfd_event_entry*) kmalloc(sizeof(*ev), GFP_KERNEL);
	ev->e.event = event;
	ev->e.extra = extra;
	ev->file = NULL;

	if (event == NOTE_FORK && (ctx->flags & NOTE_TRACK))
	{
		// Automatically create a new fd for tracking the subprocess.
		// Otherwise the userspace wouldn't be able to register for
		// the new process in time before it does something (e.g. execve).

		evprocfd_create(current->tgid, ctx->flags | NOTE_CHILD, &ev->file);
	}

	spin_lock(&ctx->lock);

	// Enqueue the event
	list_add_tail(&ev->list, &ctx->events);
	
	// Wake up everyone polling us
	wake_up(&ctx->queue);
	spin_unlock(&ctx->lock);
}

int evprocfd_release(struct inode* inode, struct file* file)
{
	struct evprocfd_ctx* ctx = (struct evprocfd_ctx*) file->private_data;
	struct list_head *next, *tmp;

	debug_msg("evprocfd_release with ctx %p\n", ctx);

	// deregister from task registry
	darling_task_notify_deregister(ctx->pid, ctx);

	// release queued events
	list_for_each_safe(next, tmp, &ctx->events)
	{
		struct evprocfd_event_entry* e = list_entry(next, struct evprocfd_event_entry, list);
		if (e->file)
			fput(e->file);
		kfree(e);
	}

	kfree(ctx);
	return 0;
}

unsigned int evprocfd_poll(struct file* file, poll_table* wait)
{
	struct evprocfd_ctx* ctx = (struct evprocfd_ctx*) file->private_data;

	poll_wait(file, &ctx->queue, wait);

	if (!list_empty_careful(&ctx->events))
		return POLLIN | POLLRDNORM;

	return 0;
}

ssize_t evprocfd_read(struct file* file, char __user* buf, size_t count, loff_t* ppos)
{
	struct evprocfd_ctx* ctx = (struct evprocfd_ctx*) file->private_data;
	ssize_t rd = 0;

	if (count < sizeof(struct evproc_event))
		return -EINVAL;

retry:
	spin_lock(&ctx->lock);
	while (rd < count)
	{
		if (list_empty(&ctx->events))
		{
			// debug_msg("No more events after %d bytes read\n", rd);
			break;
		}

		struct evprocfd_event_entry* entry = list_first_entry(&ctx->events, struct evprocfd_event_entry, list);

		// This is used with NOTE_TRACK to pass a new fd for the subprocess
		if (entry->file != NULL)
		{
			int fd = get_unused_fd_flags(O_RDWR);
			if (fd >= 0)
			{
				fd_install(fd, entry->file);
				entry->e.extra |= (fd << 16);
			}
		}

		if (copy_to_user(buf + rd, &entry->e, sizeof(struct evproc_event)))
		{
			rd = -EFAULT;
			break;
		}
		rd += sizeof(struct evproc_event);

		list_del(&entry->list);
		kfree(entry);
	}
	spin_unlock(&ctx->lock);

	if (ppos != NULL)
		*ppos = 0;

	if (rd == 0)
	{
		if (file->f_flags & O_NONBLOCK)
			return -EAGAIN;

		// Wait until there are any events
		int rv = wait_event_interruptible(ctx->queue, !list_empty_careful(&ctx->events));
		if (rv != 0)
			return rv;
		else
			goto retry;
	}
	return rd;
}

ssize_t evprocfd_write(struct file* file, const char __user *buf, size_t count, loff_t* ppos)
{
	struct evprocfd_ctx* ctx = (struct evprocfd_ctx*) file->private_data;
	unsigned int new_flags;

	if (count != sizeof(new_flags))
		return -EINVAL;
	
	if (copy_from_user(&new_flags, buf, sizeof(new_flags)))
		return -EFAULT;
	ctx->flags = new_flags;

	if (ppos != NULL)
		*ppos = 0;

	return sizeof(new_flags);
}

