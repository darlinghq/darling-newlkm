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
#include <asm/desc.h>

#define INT_NUMBER 0x83

extern void isr_proc(void);

void isr_proc_impl(void)
{
	printk(KERN_NOTICE "0x83 ISR called!\n");
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

	pack_gate(&newidt[INT_NUMBER], GATE_INTERRUPT, (unsigned long)isr_proc, 0x3, 0, __KERNEL_CS);

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


