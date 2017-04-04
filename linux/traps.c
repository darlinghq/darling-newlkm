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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/file.h>
#include <linux/dcache.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/syscalls.h>
#include "traps.h"

MODULE_LICENSE("GPL");

typedef long (*trap_handler)(mach_task_t*, ...);

static struct file_operations mach_chardev_ops = {
	.open           = mach_dev_open,
	.release        = mach_dev_release,
	.unlocked_ioctl = mach_dev_ioctl,
	.compat_ioctl   = mach_dev_ioctl,
	.owner          = THIS_MODULE
};

#define TRAP(num, impl) [num - DARLING_MACH_API_BASE] = (trap_handler) impl

static const trap_handler mach_traps[40] = {
	TRAP(NR_get_api_version, mach_get_api_version),
};
#undef TRAP

static struct miscdevice mach_dev = {
	MISC_DYNAMIC_MINOR,
	"mach",
	&mach_chardev_ops,
};

extern void darling_xnu_init(void);
static int mach_init(void)
{
	int err = 0;
	darling_xnu_init();

	printk(KERN_INFO "Darling Mach: kernel emulation loaded, API version " DARLING_MACH_API_VERSION_STR "\n");
	return 0;

fail:
	printk(KERN_WARNING "Error loading Darling Mach: %d\n", err);
	return err;
}
static void mach_exit(void)
{
	printk(KERN_INFO "Darling Mach: kernel emulation unloaded\n");
}

int mach_dev_open(struct inode* ino, struct file* file)
{

	return 0;
}

int mach_dev_release(struct inode* ino, struct file* file)
{

	return 0;
}

long mach_dev_ioctl(struct file* file, unsigned int ioctl_num, unsigned long ioctl_paramv)
{
	const unsigned int num_traps = sizeof(mach_traps) / sizeof(mach_traps[0]);

	//darling_mach_port_t* task_port = (darling_mach_port_t*) file->private_data;
	//mach_task_t* task;

	kprintf("function 0x%x called...\n", ioctl_num);

	ioctl_num -= DARLING_MACH_API_BASE;

	if (ioctl_num >= num_traps)
		return -ENOSYS;

	if (!mach_traps[ioctl_num])
		return -ENOSYS;

	//task = ipc_port_get_task(task_port);

	//return mach_traps[ioctl_num](task, ioctl_paramv);
	return -1;
}

int mach_get_api_version(mach_task_t* task)
{
	return DARLING_MACH_API_VERSION;
}


module_init(mach_init);
module_exit(mach_exit);
