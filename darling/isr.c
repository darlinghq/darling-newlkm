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
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <asm/desc.h>
#include <linux/ptrace.h>
#include <linux/semaphore.h>
#include "commpage.h"

#define INT_NUMBER 0x83

extern void isr_proc(void);
extern struct file* commpage_install(void);

extern struct file_operations mach_chardev_ops;
extern struct linux_binfmt macho_format;

void isr_proc_impl(struct pt_regs* regs)
{
	bool _64bit = !test_thread_flag(TIF_IA32);

	unsigned int call_nr = regs->orig_ax;
	unsigned long arg = regs->cx;

	printk(KERN_NOTICE "int 0x83: call_nr=%d, arg=0x%x\n", call_nr, arg);
	// Find the vm_area_struct governing the commpage
	// so that we can get the file* for this XNU task.

	struct vm_area_struct* vma;
	struct file* machdev_file = NULL;

	down_read(&current->mm->mmap_sem);

	vma = find_vma(current->mm, commpage_address(_64bit));

	if (vma && vma->vm_file)
	{
		if (vma->vm_file->f_op == &mach_chardev_ops)
		{
			machdev_file = vma->vm_file;
			atomic_long_inc(&machdev_file->f_count);
		}
	}

	up_read(&current->mm->mmap_sem);

	if (unlikely(machdev_file == NULL))
	{
		if (unlikely(current->mm->binfmt != &macho_format))
		{
			printk(KERN_NOTICE "int 0x83: ISR called from a non Mach-O process. Killing.\n");
			send_sig(SIGKILL, current, 0);

			return;
		}
		else
		{
			// Probably a process after forking. Set it up.
			machdev_file = commpage_install();
			if (IS_ERR(machdev_file))
			{
				printk(KERN_ERR "Failed to map commpage\n");
				send_sig(SIGKILL, current, 0);

				return;
			}
		}
	}

	long result = mach_chardev_ops.unlocked_ioctl(machdev_file, call_nr, arg);
	regs->ax = result;

	fput(machdev_file);
}

static unsigned long idtpage;
static struct desc_ptr idtreg;

int isr_install(void)
{
	struct desc_ptr newidtr;
	gate_desc *newidt, *oldidt;

	store_idt(&idtreg);
	oldidt = (gate_desc *) idtreg.address;

	idtpage =__get_free_page(GFP_KERNEL);
	if (!idtpage)
		return -ENOMEM;

	newidtr.address = idtpage;
	newidtr.size = idtreg.size;
	newidt = (gate_desc*) newidtr.address;

	memcpy(newidt, oldidt, idtreg.size);

	pack_gate(&newidt[INT_NUMBER], GATE_INTERRUPT, (unsigned long)isr_proc, /* DPL3 */ 0x3, 0, __KERNEL_CS);

	load_idt(&newidtr);
	smp_call_function((smp_call_func_t) load_idt, &newidtr, 1);

	return 0;
}

void isr_uninstall(void)
{
	struct desc_ptr cur_idtreg;
	store_idt(&cur_idtreg);

	if (cur_idtreg.address != idtpage)
	{
		printk(KERN_WARNING "BEWARE: There is more than one LKM manipulating the IDT! Cannot uninstall ISR for int 0x83 safely.\n");
	}
	else
	{
		load_idt(&idtreg);
		smp_call_function((smp_call_func_t) load_idt, &idtreg, 1);
		free_page(idtpage);
	}
}

#if 0
// This approach doesn't work, because "gate" is r/o
int isr_install(void)
{
	struct desc_ptr idtreg;
	gate_desc* gate;

	store_idt(&idtreg);

	gate = (gate_desc*) idtreg.address;

	pack_gate(&gate[INT_NUMBER], GATE_INTERRUPT, (unsigned long)isr_proc, 0x3, 0, __KERNEL_CS);

	return 0;
}

void isr_uninstall(void)
{
	struct desc_ptr idtreg;
	gate_desc* gate;

	store_idt(&idtreg);

	gate = (gate_desc*) idtreg.address;
	memset(&gate[INT_NUMBER], 0, sizeof(gate[0]));
}
#endif

