/*
Copyright (c) 2014-2017, Wenqi Chen
Copyright (c) 2017 Lubos Dolezel

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

#include <duct/duct.h>
#include <linux/uaccess.h>
// #include <sys/proc_internal.h>
#include <duct/duct_pre_xnu.h>

#include <kern/wait_queue.h>
#include <mach/mach_types.h>
#include <machine/machlimits.h>
//#include "compat_kevmachportfd.h"

#include <duct/duct_post_xnu.h>

#undef kfree
extern void kfree (const void * objp);

extern wait_queue_head_t * duct_wait_queue_link_fdctx (xnu_wait_queue_t waitq, void * ctx);

struct compat_kevmachportfd_ctx {
        struct list_head        link;

        mach_port_name_t        port_name;
        wait_queue_head_t *     linux_waitqh;
        uint32_t                count;
};

static int compat_kevmachportfd_release (struct inode * inode, struct file * file);
static unsigned int compat_kevmachportfd_poll (struct file * file, poll_table * wait);
static ssize_t compat_kevmachportfd_read (struct file * file, char __user * buf, size_t count, loff_t * ppos);
static ssize_t compat_kevmachportfd_ctx_read (struct compat_kevmachportfd_ctx * ctx, int no_wait, uint32_t * count);

static const struct file_operations compat_kevmachportfd_fops    = {
        .release    = compat_kevmachportfd_release,
        .poll       = compat_kevmachportfd_poll,
        .read       = compat_kevmachportfd_read,
};


void compat_kevmachportfd_raise (void * fdctx)
{
        // if (!fdctx)     return;
        int     found   = 0;
        struct compat_kevmachportfd_ctx   * ctx         = fdctx;

        unsigned long   flags;
        spin_lock_irqsave (&ctx->linux_waitqh->lock, flags);

        printk (KERN_NOTICE "- raise ctx: 0x%p, count ++\n", ctx);

        if (ctx->count < ULONG_MAX) {
                ctx->count ++;
        }

        spin_unlock_irqrestore (&ctx->linux_waitqh->lock, flags);

        if (unlikely (waitqueue_active (ctx->linux_waitqh))) {
                wake_up (ctx->linux_waitqh);
        }
}

int compat_kevmachportfd (mach_port_name_t port_name, wait_queue_t waitq)
{
        int             retval  = 0;

        int             fd;
        struct file   * file;

        struct compat_kevmachportfd_ctx   * ctx    = NULL;


        ctx   = kmalloc (sizeof(struct compat_kevmachportfd_ctx), GFP_KERNEL);
        ctx->port_name      = port_name;
        ctx->linux_waitqh   = duct_wait_queue_link_fdctx (waitq, ctx);
        ctx->count          = 0;

        fd      = get_unused_fd_flags (O_RDWR);
        if (fd < 0) {
                retval = fd;
                goto out_free_ctx;
        }

        file    = anon_inode_getfile ("[kevmachportfd]", &compat_kevmachportfd_fops, ctx, O_RDWR);
        if (IS_ERR (file)) {
                retval  = PTR_ERR (file);
                goto out_free_fd;
        }

        fd_install (fd, file);

        // struct compat_proc   * uproc   = linux_current->uproc;
        // list_add (&ctx->link, &uproc->kevmachportfd_ctx_links);

        return fd;

out_free_fd:
        put_unused_fd (fd);

out_free_ctx:
        kfree (ctx);

        return retval;
}


static int compat_kevmachportfd_release (struct inode * inode, struct file * file)
{
        struct compat_kevmachportfd_ctx   * ctx     = file->private_data;

        // list_del (&ctx->link);
        // wake_up_poll(&ctx->wqh, POLLHUP);

        kfree (ctx);
        return 0;
}

static unsigned int compat_kevmachportfd_poll (struct file * file, poll_table * wait)
{
        struct compat_kevmachportfd_ctx   * ctx     = file->private_data;

        printk (KERN_NOTICE "- kevmachportfd poll, ctx->linux_waitqh: 0x%p\n", ctx->linux_waitqh);

        unsigned int        events = 0;
        unsigned long       flags;

        poll_wait (file, ctx->linux_waitqh, wait);

        spin_lock_irqsave (&ctx->linux_waitqh->lock, flags);
        if (ctx->count > 0) {
                printk (KERN_NOTICE "- wow - machportfd to return pollin\n");
                events    |= POLLIN;
        }

        if (ctx->count == ULONG_MAX) {
                events    |= POLLERR;
        }

        // if (ctx->count < ULONG_MAX) {
        //         events    |= POLLOUT;
        // }

        spin_unlock_irqrestore (&ctx->linux_waitqh->lock, flags);

        return events;
}

static ssize_t compat_kevmachportfd_read (struct file * file, char __user * buf, size_t count, loff_t * ppos)
{
        printk (KERN_NOTICE "- kevmachportfd read called\n");

        struct compat_kevmachportfd_ctx   * ctx     = file->private_data;

        ssize_t     res;
        uint32_t    read_count;

        if (count < sizeof(read_count))   return -LINUX_EINVAL;

        res     = compat_kevmachportfd_ctx_read (ctx, 1, &read_count);
        if (res < 0)    return res;

        if (copy_to_user(buf, &read_count, sizeof(read_count)))
            res = -LINUX_EFAULT;

        return res;
}

static ssize_t compat_kevmachportfd_ctx_read (struct compat_kevmachportfd_ctx * ctx, int no_wait, uint32_t * count)
{
        ssize_t                 res     = -LINUX_EAGAIN;

        // DECLARE_WAITQUEUE (wait, current);
        unsigned long   flags;
        spin_lock_irqsave (&ctx->linux_waitqh->lock, flags);

        *count  = ctx->count;
        // res     = (ctx->count > 0) ? sizeof(uint32_t) : -LINUX_EAGAIN;
        
        // mqueue_receive_on_thread ....
        // mach_port_name_t remote_port_name  = 0;
        // duct_filt_machport_process (ctx->port_name, remote_port_name);
        // printk mach_port

        if (ctx->count > 0) {
                ctx->count --;
                res     = sizeof(uint32_t);
        }
        else {
                res     = -LINUX_EAGAIN;
        }

        spin_unlock_irqrestore (&ctx->linux_waitqh->lock, flags);
        return res;
}
