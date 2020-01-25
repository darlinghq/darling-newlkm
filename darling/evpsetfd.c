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
#include <duct/duct.h>
#include "evpsetfd.h"
#include <mach/mach_types.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/anon_inodes.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <duct/duct_pre_xnu.h>
#include <duct/duct_kern_waitqueue.h>
#include <osfmk/ipc/ipc_types.h>
#include <osfmk/ipc/ipc_object.h>
#include <osfmk/ipc/ipc_space.h>
#include <osfmk/ipc/ipc_pset.h>
#include <duct/duct_post_xnu.h>
#include "debug_print.h"

#undef kmalloc
#undef kfree

struct evpsetfd_ctx
{
	unsigned int port_name;
	struct evpset_options opts;

	ipc_pset_t pset;
	SLIST_ENTRY(evpsetfd_ctx) kn_selnext;

	wait_queue_head_t wait_queue;
	struct evpset_event event;
	int has_event;
};

static int evpsetfd_release(struct inode* inode, struct file* file);
static unsigned int evpsetfd_poll(struct file* file, poll_table* wait);
static ssize_t evpsetfd_read(struct file* file, char __user* buf, size_t count, loff_t* ppos);
static ssize_t evpsetfd_write(struct file* file, const char __user *buf, size_t count, loff_t* ppos);
int knote_attach_evpset(struct klist* list, struct evpsetfd_ctx* kn);
int knote_detach_evpset(struct klist* list, struct evpsetfd_ctx* kn);

static const struct file_operations evpsetfd_ops =
{
	.release = evpsetfd_release,
	.read = evpsetfd_read,
	.write = evpsetfd_write,
	.poll = evpsetfd_poll,
};

int evpsetfd_create(unsigned int port_name, const struct evpset_options* opts)
{
	ipc_pset_t pset;
	kern_return_t kr;
	struct evpsetfd_ctx* ctx;
	char name[32];
	int fd;

	// TODO: Use ipc_right_lookup_read() instead and support plain Mach ports (instead of port sets only)
	kr = ipc_object_translate(current_space(), port_name, MACH_PORT_RIGHT_PORT_SET, (ipc_object_t*) &pset);
	if (kr != KERN_SUCCESS)
		return -LINUX_ENOENT;

	ctx = (struct evpsetfd_ctx*) kmalloc(sizeof(*ctx), GFP_KERNEL);
	if (ctx == NULL)
	{
		ips_unlock(pset);
		return -LINUX_ENOMEM;
	}

	ctx->port_name = port_name;
	ctx->pset = pset;

	memset(&ctx->event, 0, sizeof(ctx->event));

	init_waitqueue_head(&ctx->wait_queue);
	memcpy(&ctx->opts, opts, sizeof(*opts));

	sprintf(name, "[evpsetfd:%d]", port_name);
	fd = anon_inode_getfd(name, &evpsetfd_ops, ctx, O_RDWR);
	if (fd < 0)
	{
		kfree(ctx);
		ips_unlock(pset);

		return fd;
	}

	ipc_mqueue_t mqueue = &pset->ips_messages;
	knote_attach_evpset(&mqueue->imq_klist, ctx);
	ctx->has_event = ipc_mqueue_set_peek(mqueue);

	debug_msg("    are there events already? %d!\n", ctx->has_event);

	ips_reference(pset);
	ips_unlock(pset);

	return fd;
}

int evpsetfd_release(struct inode* inode, struct file* file)
{
	struct evpsetfd_ctx* ctx = (struct evpsetfd_ctx*) file->private_data;

	ipc_mqueue_t mqueue = &ctx->pset->ips_messages;
	knote_detach_evpset(&mqueue->imq_klist, ctx);

	ips_release(ctx->pset);

	kfree(ctx);
	return 0;
}

unsigned int evpsetfd_poll(struct file* file, poll_table* wait)
{
	struct evpsetfd_ctx* ctx = (struct evpsetfd_ctx*) file->private_data;
	ipc_mqueue_t set_mq = &ctx->pset->ips_messages;

	poll_wait(file, &ctx->wait_queue, wait); // For polling on Mach ports (not really supported, the code in this file assumes port sets)
	poll_wait(file, &set_mq->imq_wait_queue.linux_wq, wait); // For polling on Mach port sets

	if (!ctx->has_event)
		ctx->has_event = ipc_mqueue_set_peek(set_mq);
	
	return ctx->has_event ? (POLLIN | POLLRDNORM) : 0;
}

ssize_t evpsetfd_read(struct file* file, char __user* buf, size_t count, loff_t* ppos)
{
	struct evpsetfd_ctx* ctx = (struct evpsetfd_ctx*) file->private_data;
	struct evpset_event out;

	debug_msg("evpsetfd_read() called\n");

	ctx->has_event = false;
	char* process_data = ((struct evpset_event*) buf)->process_data;

	if (count != sizeof(struct evpset_event))
		return -LINUX_EINVAL;

	ipc_mqueue_t mqueue = &ctx->pset->ips_messages;
	ipc_object_t object = &ctx->pset->ips_object;
	thread_t self = current_thread();
	boolean_t used_filtprocess_data = FALSE;

	wait_result_t wresult;
	mach_msg_option_t option;
	mach_vm_address_t addr;
	mach_msg_size_t	size;

	imq_lock(mqueue);

	/* Capture current state */
	//*kev = kn->kn_kevent;

	/* If already deallocated/moved return one last EOF event */
	if (ctx->event.flags & EV_EOF) {
		imq_unlock(mqueue);
		//return 1;
		goto out;
        }

	/*
	 * Only honor supported receive options. If no options are
	 * provided, just force a MACH_RCV_TOO_LARGE to detect the
	 * name of the port and sizeof the waiting message.
	 */
	option = ctx->opts.sfflags & (MACH_RCV_MSG|MACH_RCV_LARGE|MACH_RCV_LARGE_IDENTITY|
	                           MACH_RCV_TRAILER_MASK|MACH_RCV_VOUCHER);

	if (option & MACH_RCV_MSG) {
		addr = (mach_vm_address_t) ctx->opts.rcvbuf;
		size = (mach_msg_size_t) ctx->opts.rcvbuf_size;

		/*
		 * If the kevent didn't specify a buffer and length, carve a buffer
		 * from the filter processing data according to the flags.
		 */
		if (size == 0 && process_data != NULL) {
			used_filtprocess_data = TRUE;

#ifdef __DARLING__
			addr = process_data;
			size = sizeof(ctx->event.process_data);
			option |= (MACH_RCV_LARGE | MACH_RCV_LARGE_IDENTITY);
#else

			addr = (mach_vm_address_t)process_data->fp_data_out;
			size = (mach_msg_size_t)process_data->fp_data_resid;
			option |= (MACH_RCV_LARGE | MACH_RCV_LARGE_IDENTITY);
			if (process_data->fp_flags & KEVENT_FLAG_STACK_DATA)
				option |= MACH_RCV_STACK;
#endif
		}
	} else {
		/* just detect the port name (if a set) and size of the first message */
		option = MACH_RCV_LARGE;
		addr = 0;
		size = 0;
	}

	/* just use the reference from here on out */
	io_reference(object);

	/*
	 * Set up to receive a message or the notification of a
	 * too large message.  But never allow this call to wait.
	 * If the user provided aditional options, like trailer
	 * options, pass those through here.  But we don't support
	 * scatter lists through this interface.
	 */
	self->ith_object = object;
	self->ith_msg_addr = addr;
	self->ith_rsize = size;
	self->ith_msize = 0;
	self->ith_option = option;
	self->ith_receiver_name = MACH_PORT_NULL;
	self->ith_continuation = NULL;
	option |= MACH_RCV_TIMEOUT; // never wait
	self->ith_state = MACH_RCV_IN_PROGRESS;

	wresult = ipc_mqueue_receive_on_thread(
			mqueue,
			option,
			size, /* max_size */
			0, /* immediate timeout */
			THREAD_INTERRUPTIBLE,
			self);
	/* mqueue unlocked */

	/*
	 * If we timed out, or the process is exiting, just release the
	 * reference on the ipc_object and return zero.
	 */
	if (wresult == THREAD_RESTART || self->ith_state == MACH_RCV_TIMED_OUT) {
		io_release(object);
		return -LINUX_EAGAIN;
	}

	assert(wresult == THREAD_NOT_WAITING);
	assert(self->ith_state != MACH_RCV_IN_PROGRESS);

	/*
	 * If we weren't attempting to receive a message
	 * directly, we need to return the port name in
	 * the kevent structure.
	 */
	if ((option & MACH_RCV_MSG) != MACH_RCV_MSG) {
		assert(self->ith_state == MACH_RCV_TOO_LARGE);
		assert(self->ith_kmsg == IKM_NULL);
		ctx->event.port = self->ith_receiver_name;
		io_release(object);

		goto out;
	}

	/*
	 * Attempt to receive the message directly, returning
	 * the results in the fflags field.
	 */
	ctx->event.receive_status = mach_msg_receive_results(&size);

	/* kmsg and object reference consumed */

	/*
	 * if the user asked for the identity of ports containing a
	 * a too-large message, return it in the data field (as we
	 * do for messages we didn't try to receive).
	 */
	if (ctx->event.receive_status == MACH_RCV_TOO_LARGE) {
		ctx->event.msg_size = self->ith_msize;
		if (option & MACH_RCV_LARGE_IDENTITY)
			ctx->event.port = self->ith_receiver_name;
		else
			ctx->event.port = MACH_PORT_NULL;
	} else {
		ctx->event.msg_size = size;
		ctx->event.port = MACH_PORT_NULL;
	}
#ifndef __DARLING__
	/*
	 * If we used a data buffer carved out from the filt_process data,
	 * store the address used in the knote and adjust the residual and
	 * other parameters for future use.
	 */
	if (used_filtprocess_data) {
		assert(process_data->fp_data_resid >= size);
		process_data->fp_data_resid -= size;
		if ((process_data->fp_flags & KEVENT_FLAG_STACK_DATA) == 0) {
			kev->ext[0] = process_data->fp_data_out;
			process_data->fp_data_out += size;
		} else {
			assert(option & MACH_RCV_STACK);
			kev->ext[0] = process_data->fp_data_out + 
				      process_data->fp_data_resid;
		}
	}

	/*
	 * Apply message-based QoS values to output kevent as prescribed.
	 * The kev->qos field gets max(msg-qos, kn->kn_qos).
	 * The kev->ext[2] field gets (msg-qos << 32) | (override-qos).
	 *
	 * The mach_msg_receive_results() call saved off the message
	 * QoS values in the continuation save area on successful receive.
	 */
	if (ctx->event.receive_status == MACH_MSG_SUCCESS) {
		kev->qos = mach_msg_priority_combine(self->ith_qos, kn->kn_qos);
		kev->ext[2] = ((uint64_t)self->ith_qos << 32) | 
		               (uint64_t)self->ith_qos_override;
	}
#endif

out:
	if (copy_to_user(buf, &ctx->event, sizeof(ctx->event)))
		return -LINUX_EFAULT;
	memset(&ctx->event, 0, sizeof(ctx->event));

	return sizeof(out);
}

ssize_t evpsetfd_write(struct file* file, const char __user *buf, size_t count, loff_t* ppos)
{
	struct evpsetfd_ctx* ctx = (struct evpsetfd_ctx*) file->private_data;

	if (count != sizeof(struct evpset_options))
		return -LINUX_EINVAL;

	if (copy_from_user(&ctx->opts, buf, sizeof(ctx->opts)))
		return -LINUX_EFAULT;

	if (ppos != NULL)
		*ppos = 0;

	return count;
}

void knote(struct klist* list, long hint)
{
    debug_msg("knote() on list 0x%x called with hint=0x%x\n", list, hint);
	struct evpsetfd_ctx* kn;

	SLIST_FOREACH(kn, list, kn_selnext)
	{
		if (hint == NOTE_REVOKE)
			kn->event.flags = EV_EOF | EV_ONESHOT;

		kn->has_event = true;
		debug_msg("knote() is waking up a Linux wait queue 0x%x\n", &kn->wait_queue);
		wake_up_interruptible(&kn->wait_queue);
	}
}

void klist_init(struct klist* list)
{
	SLIST_INIT(list);
}

int knote_attach_evpset(struct klist* list, struct evpsetfd_ctx* kn)
{
	int ret = SLIST_EMPTY(list);
	debug_msg("Attaching to klist 0x%x\n", list);
	SLIST_INSERT_HEAD(list, kn, kn_selnext);
	return ret;
}

int knote_detach_evpset(struct klist* list, struct evpsetfd_ctx* kn)
{
	debug_msg("Detaching from klist 0x%x\n", list);
	SLIST_REMOVE(list, kn, evpsetfd_ctx, kn_selnext);
	return SLIST_EMPTY(list);
}

void waitq_set__CALLING_PREPOST_HOOK__(void* kq_hook, void* knote_hook, int qos)
{
}


