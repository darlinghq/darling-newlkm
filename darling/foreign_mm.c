/*
 * Darling Mach Linux Kernel Module
 * Copyright (C) 2018 Lubos Dolezel
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

#include <duct/compiler/clang/asm-inline.h>
#include "foreign_mm.h"
#include <linux/kthread.h>

static struct kthread_worker* worker = NULL;

void foreignmm_init(void)
{
	worker = kthread_create_worker(0, "darling-foreign_mm");
}

void foreignmm_exit(void)
{
	kthread_destroy_worker(worker);
	worker = NULL;
}

struct foreignmm_work
{
	struct kthread_work kw;
	foreignmm_cb_t cb;
	void* param;
	struct mm_struct* mm;
};

static void foreignmm_run(struct kthread_work* kw)
{
	struct foreignmm_work* work = container_of(kw, struct foreignmm_work, kw);
	struct mm_struct* orig_mm = current->mm;

	current->mm = work->mm;

	work->cb(work->param);

	current->mm = orig_mm;
}

void foreignmm_execute(struct mm_struct* mm, foreignmm_cb_t cb, void* param)
{
	struct foreignmm_work work = {
		.cb = cb,
		.param = param,
		.mm = mm
	};

	kthread_init_work(&work.kw, foreignmm_run);
	kthread_queue_work(worker, &work.kw);
	kthread_flush_work(&work.kw);
}

