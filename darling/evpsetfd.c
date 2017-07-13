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
};

static int evpsetfd_release(struct inode* inode, struct file* file);
static unsigned int evpsetfd_poll(struct file* file, poll_table* wait);
static ssize_t evpsetfd_read(struct file* file, char __user* buf, size_t count, loff_t* ppos);
static ssize_t evpsetfd_write(struct file* file, const char __user *buf, size_t count, loff_t* ppos);

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
	memcpy(&ctx->opts, opts, sizeof(*opts));

	sprintf(name, "[evpsetfd:%d]", port_name);
	fd = anon_inode_getfd(name, &evpsetfd_ops, ctx, O_RDWR);
	if (fd < 0)
	{
		kfree(ctx);
		ips_unlock(pset);

		return fd;
	}

	ips_reference(pset);
	ips_unlock(pset);

	return fd;
}

int evpsetfd_release(struct inode* inode, struct file* file)
{
	struct evpsetfd_ctx* ctx = (struct evpsetfd_ctx*) file->private_data;

	ips_release(ctx->pset);

	kfree(ctx);
	return 0;
}

unsigned int evpsetfd_poll(struct file* file, poll_table* wait)
{
	struct evpsetfd_ctx* ctx = (struct evpsetfd_ctx*) file->private_data;
	ipc_mqueue_t set_mq = &ctx->pset->ips_messages;

	xnu_wait_queue_t waitq = &set_mq->imq_wait_queue;
	xnu_wait_queue_t walked_waitq = duct__wait_queue_walkup(waitq, IPC_MQUEUE_RECEIVE);

	poll_wait(file, &walked_waitq->linux_waitqh, wait);

	if (ipc_mqueue_peek(set_mq))
	{
		debug_msg("evpsetfd_poll(): there is a pending msg\n");
		return POLLIN | POLLRDNORM;
	}
	debug_msg("evpsetfd_poll(): no pending msg\n");

	return 0;
}

ssize_t evpsetfd_read(struct file* file, char __user* buf, size_t count, loff_t* ppos)
{
	struct evpsetfd_ctx* ctx = (struct evpsetfd_ctx*) file->private_data;
	struct evpset_event out;

	debug_msg("evpsetfd_read() called\n");

	if (count != sizeof(struct evpset_event))
		return -LINUX_EINVAL;

	// Taken from XNU's filt_machport()
	// and adapted to return struct evpset_event
	
	mach_port_name_t        name = ctx->port_name;
	ipc_pset_t              pset = IPS_NULL;
	wait_result_t		wresult;
	thread_t		self = current_thread();
	kern_return_t           kr;
	mach_msg_option_t	option;
	mach_msg_size_t		size;

	/* never called from below */
	// assert(hint == 0);
	memset(&out, 0, sizeof(out));

	/*
	 * called from user context. Have to validate the
	 * name.  If it changed, we have an EOF situation.
	 */
	kr = ipc_object_translate(current_space(), name,
				  MACH_PORT_RIGHT_PORT_SET,
				  (ipc_object_t *)&pset);
	if (kr != KERN_SUCCESS || pset != ctx->pset || !ips_active(pset)) {
		out.port = 0;
		out.flags |= (EV_EOF | EV_ONESHOT);
		if (pset != IPS_NULL) {
			ips_unlock(pset);
		}
		if (copy_to_user(buf, &out, sizeof(out)))
			return -LINUX_EFAULT;

		return sizeof(out);
        }

	/* just use the reference from here on out */
	ips_reference(pset);
	ips_unlock(pset); 

	/*
	 * Only honor supported receive options. If no options are
	 * provided, just force a MACH_RCV_TOO_LARGE to detect the
	 * name of the port and sizeof the waiting message.
	 */
	option = ctx->opts.sfflags & (MACH_RCV_MSG|MACH_RCV_LARGE|MACH_RCV_TRAILER_MASK);
	if (option & MACH_RCV_MSG) {
		if (ctx->opts.rcvbuf_size == 0)
			option |= MACH_RCV_LARGE;
		self->ith_msg_addr = (mach_vm_address_t) ctx->opts.rcvbuf;
		size = (mach_msg_size_t)ctx->opts.rcvbuf_size;
	} else {
		option = MACH_RCV_LARGE;
		self->ith_msg_addr = 0;
		size = 0;
	}

	/*
	 * Set up to receive a message or the notification of a
	 * too large message.  But never allow this call to wait.
	 * If the user provided aditional options, like trailer
	 * options, pass those through here.  But we don't support
	 * scatter lists through this interface.
	 */
	self->ith_object = (ipc_object_t)pset;
	self->ith_msize = size;
	self->ith_option = option;
	self->ith_scatter_list_size = 0;
	self->ith_receiver_name = MACH_PORT_NULL;
	self->ith_continuation = NULL;
	option |= MACH_RCV_TIMEOUT; // never wait
	assert((self->ith_state = MACH_RCV_IN_PROGRESS) == MACH_RCV_IN_PROGRESS);

	wresult = ipc_mqueue_receive_on_thread(
			&pset->ips_messages,
			option,
			size, /* max_size */
			0, /* immediate timeout */
			THREAD_INTERRUPTIBLE,
			self);
	assert(wresult == THREAD_NOT_WAITING);
	assert(self->ith_state != MACH_RCV_IN_PROGRESS);

	debug_msg("- ith_state after receive_on_thread: %d\n", self->ith_state);
	/*
	 * If we timed out, just release the reference on the
	 * portset and return zero.
	 */
	if (self->ith_state == MACH_RCV_TIMED_OUT) {
		ips_release(pset);
		return -LINUX_EAGAIN;
	}

	/*
	 * If we weren't attempting to receive a message
	 * directly, we need to return the port name in
	 * the kevent structure.
	 */
	if ((option & MACH_RCV_MSG) != MACH_RCV_MSG) {
		assert(self->ith_state == MACH_RCV_TOO_LARGE);
		assert(self->ith_kmsg == IKM_NULL);
		// kn->kn_data = self->ith_receiver_name;
		out.port = self->ith_receiver_name;
		ips_release(pset);

		if (copy_to_user(buf, &out, sizeof(out)))
			return -LINUX_EFAULT;

		return sizeof(out);
	}

	/*
	 * Attempt to receive the message directly, returning
	 * the results in the fflags field.
	 */
	assert(option & MACH_RCV_MSG);
	out.port = self->ith_receiver_name;
	out.msg_size = self->ith_msize;
	out.receive_status = mach_msg_receive_results();
	/* kmsg and pset reference consumed */

	if (copy_to_user(buf, &out, sizeof(out)))
		return -LINUX_EFAULT;

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

