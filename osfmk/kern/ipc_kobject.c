/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * NOTICE: This file was modified by McAfee Research in 2004 to introduce
 * support for mandatory and extensible security protections.  This notice
 * is included in support of clause 2.2 (b) of the Apple Public License,
 * Version 2.0.
 * Copyright (c) 2005 SPARTA, Inc.
 */
/*
 */
/*
 *	File:	kern/ipc_kobject.c
 *	Author:	Rich Draves
 *	Date:	1989
 *
 *	Functions for letting a port represent a kernel object.
 */

#ifdef __DARLING__
#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>
#endif

#include <mach_debug.h>
#include <mach_ipc_test.h>
#include <mach/mig.h>
#include <mach/port.h>
#include <mach/kern_return.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/notify.h>
#include <mach/ndr.h>
#include <mach/vm_param.h>

#include <mach/mach_vm_server.h>
#include <mach/mach_port_server.h>
#include <mach/mach_host_server.h>
#include <mach/host_priv_server.h>
#include <mach/host_security_server.h>
#include <mach/clock_server.h>
#include <mach/clock_priv_server.h>
#include <mach/lock_set_server.h>
#include <mach/memory_entry_server.h>
#include <mach/memory_object_control_server.h>
#include <mach/memory_object_default_server.h>
#include <mach/processor_server.h>
#include <mach/processor_set_server.h>
#include <mach/task_server.h>
#include <mach/mach_voucher_server.h>
#include <mach/mach_voucher_attr_control_server.h>
#ifdef VM32_SUPPORT
#include <mach/vm32_map_server.h>
#endif
#include <mach/thread_act_server.h>
#include <mach/restartable_server.h>

#include <mach/exc_server.h>
#include <mach/mach_exc_server.h>
#include <mach/mach_eventlink_server.h>

#include <device/device_types.h>
#include <device/device_server.h>

#if     CONFIG_USER_NOTIFICATION
#include <UserNotification/UNDReplyServer.h>
#endif

#if     CONFIG_ARCADE
#include <mach/arcade_register_server.h>
#endif

#if     CONFIG_AUDIT
#include <kern/audit_sessionport.h>
#endif

#if     MACH_MACHINE_ROUTINES
#include <machine/machine_routines.h>
#endif  /* MACH_MACHINE_ROUTINES */
#if     XK_PROXY
#include <uk_xkern/xk_uproxy_server.h>
#endif  /* XK_PROXY */

#include <kern/counter.h>
#include <kern/ipc_tt.h>
#include <kern/ipc_mig.h>
#include <kern/ipc_misc.h>
#include <kern/ipc_kobject.h>
#include <kern/host_notify.h>
#include <kern/mk_timer.h>
#include <kern/misc_protos.h>

#if CONFIG_ARCADE
#include <kern/arcade.h>
#endif /* CONFIG_ARCADE */

#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_voucher.h>
#include <kern/sync_sema.h>
#include <kern/work_interval.h>
#include <kern/suid_cred.h>
#include <kern/task_ident.h>

#if HYPERVISOR
#include <kern/hv_support.h>
#endif

#include <vm/vm_protos.h>

#include <security/mac_mach_internal.h>

#ifdef __DARLING__
#include <duct/duct_post_xnu.h>
#include <darling/debug_print.h>
#endif

extern char *proc_name_address(void *p);
struct proc;
extern int proc_pid(struct proc *p);

/*
 *	Routine:	ipc_kobject_notify
 *	Purpose:
 *		Deliver notifications to kobjects that care about them.
 */
boolean_t
ipc_kobject_notify(
	mach_msg_header_t *request_header,
	mach_msg_header_t *reply_header);

typedef struct {
	mach_msg_id_t num;
	mig_routine_t routine;
	int size;
	int kobjidx;
} mig_hash_t;

#define MAX_MIG_ENTRIES 1031
#define MIG_HASH(x) (x)

#define KOBJ_IDX_NOT_SET (-1)

#ifndef max
#define max(a, b)        (((a) > (b)) ? (a) : (b))
#endif /* max */

static SECURITY_READ_ONLY_LATE(mig_hash_t) mig_buckets[MAX_MIG_ENTRIES];
static SECURITY_READ_ONLY_LATE(int) mig_table_max_displ;
SECURITY_READ_ONLY_LATE(int) mach_kobj_count; /* count of total number of kobjects */

static ZONE_DECLARE(ipc_kobject_label_zone, "ipc kobject labels",
    sizeof(struct ipc_kobject_label), ZC_NONE);

__startup_data
static const struct mig_subsystem *mig_e[] = {
	(const struct mig_subsystem *)&mach_vm_subsystem,
	(const struct mig_subsystem *)&mach_port_subsystem,
	(const struct mig_subsystem *)&mach_host_subsystem,
	(const struct mig_subsystem *)&host_priv_subsystem,
	(const struct mig_subsystem *)&host_security_subsystem,
	(const struct mig_subsystem *)&clock_subsystem,
	(const struct mig_subsystem *)&clock_priv_subsystem,
	(const struct mig_subsystem *)&processor_subsystem,
	(const struct mig_subsystem *)&processor_set_subsystem,
	(const struct mig_subsystem *)&is_iokit_subsystem,
	(const struct mig_subsystem *)&lock_set_subsystem,
	(const struct mig_subsystem *)&task_subsystem,
	(const struct mig_subsystem *)&thread_act_subsystem,
#ifndef __DARLING__
#ifdef VM32_SUPPORT
	(const struct mig_subsystem *)&vm32_map_subsystem,
#endif
#endif
#if CONFIG_USER_NOTIFICATION
	(const struct mig_subsystem *)&UNDReply_subsystem,
#endif
	(const struct mig_subsystem *)&mach_voucher_subsystem,
	(const struct mig_subsystem *)&mach_voucher_attr_control_subsystem,
	(const struct mig_subsystem *)&memory_entry_subsystem,
	(const struct mig_subsystem *)&task_restartable_subsystem,

#if     XK_PROXY
	(const struct mig_subsystem *)&do_uproxy_xk_uproxy_subsystem,
#endif /* XK_PROXY */
#if     MACH_MACHINE_ROUTINES
	(const struct mig_subsystem *)&MACHINE_SUBSYSTEM,
#endif  /* MACH_MACHINE_ROUTINES */
#if     MCMSG && iPSC860
	(const struct mig_subsystem *)&mcmsg_info_subsystem,
#endif  /* MCMSG && iPSC860 */
	(const struct mig_subsystem *)&catch_exc_subsystem,
	(const struct mig_subsystem *)&catch_mach_exc_subsystem,
#if CONFIG_ARCADE
	(const struct mig_subsystem *)&arcade_register_subsystem,
#endif
	(const struct mig_subsystem *)&mach_eventlink_subsystem,
};

static void
mig_init(void)
{
	unsigned int i, n = sizeof(mig_e) / sizeof(const struct mig_subsystem *);
	int howmany;
	mach_msg_id_t j, pos, nentry, range;

	for (i = 0; i < n; i++) {
		range = mig_e[i]->end - mig_e[i]->start;
		if (!mig_e[i]->start || range < 0) {
			panic("the msgh_ids in mig_e[] aren't valid!");
		}

		for (j = 0; j < range; j++) {
			if (mig_e[i]->routine[j].stub_routine) {
				/* Only put real entries in the table */
				nentry = j + mig_e[i]->start;
				for (pos = MIG_HASH(nentry) % MAX_MIG_ENTRIES, howmany = 1;
				    mig_buckets[pos].num;
				    pos++, pos = pos % MAX_MIG_ENTRIES, howmany++) {
					if (mig_buckets[pos].num == nentry) {
						printf("message id = %d\n", nentry);
						panic("multiple entries with the same msgh_id");
					}
					if (howmany == MAX_MIG_ENTRIES) {
						panic("the mig dispatch table is too small");
					}
				}

				mig_buckets[pos].num = nentry;
				mig_buckets[pos].routine = mig_e[i]->routine[j].stub_routine;
				if (mig_e[i]->routine[j].max_reply_msg) {
					mig_buckets[pos].size = mig_e[i]->routine[j].max_reply_msg;
				} else {
					mig_buckets[pos].size = mig_e[i]->maxsize;
				}
				mig_buckets[pos].kobjidx = KOBJ_IDX_NOT_SET;

				mig_table_max_displ = max(howmany, mig_table_max_displ);
				mach_kobj_count++;
			}
		}
	}
	printf("mig_table_max_displ = %d mach_kobj_count = %d\n",
	    mig_table_max_displ, mach_kobj_count);
}
STARTUP(MACH_IPC, STARTUP_RANK_FIRST, mig_init);

/*
 * Do a hash table lookup for given msgh_id. Return 0
 * if not found.
 */
static mig_hash_t *
find_mig_hash_entry(int msgh_id)
{
	unsigned int i = (unsigned int)MIG_HASH(msgh_id);
	int max_iter = mig_table_max_displ;
	mig_hash_t *ptr;

	do {
		ptr = &mig_buckets[i++ % MAX_MIG_ENTRIES];
	} while (msgh_id != ptr->num && ptr->num && --max_iter);

	if (!ptr->routine || msgh_id != ptr->num) {
		ptr = (mig_hash_t *)0;
	}

	return ptr;
}

/*
 *      Routine:	ipc_kobject_set_kobjidx
 *      Purpose:
 *              Set the index for the kobject filter
 *              mask for a given message ID.
 */
kern_return_t
ipc_kobject_set_kobjidx(
	int       msgh_id,
	int       index)
{
	mig_hash_t *ptr = find_mig_hash_entry(msgh_id);

	if (ptr == (mig_hash_t *)0) {
		return KERN_INVALID_ARGUMENT;
	}

	assert(index < mach_kobj_count);
	ptr->kobjidx = index;

	return KERN_SUCCESS;
}

/*
 *	Routine:	ipc_kobject_server
 *	Purpose:
 *		Handle a message sent to the kernel.
 *		Generates a reply message.
 *		Version for Untyped IPC.
 *	Conditions:
 *		Nothing locked.
 */

ipc_kmsg_t
ipc_kobject_server(
	ipc_kmsg_t      request,
	mach_msg_option_t __unused option)
{
	mach_msg_size_t reply_size;
	ipc_kmsg_t reply;
	kern_return_t kr;
	ipc_port_t  replyp = IPC_PORT_NULL;
	mach_msg_max_trailer_t *trailer;
	mig_hash_t *ptr;
	task_t task = TASK_NULL;
	uint32_t exec_token;
	boolean_t exec_token_changed = FALSE;
	int request_msgh_id = request->ikm_header->msgh_id;
	natural_t ikot;
	ipc_port_t port;

	reply = NULL;
	port = request->ikm_header->msgh_remote_port;
	if (IP_VALID(port)) {
		ikot = ip_kotype(port);
	} else {
		ikot = IKOT_UNKNOWN;
	}
	if (ikot == IKOT_UEXT_OBJECT) {
		kr = uext_server(request, &reply);
		if ((MIG_NO_REPLY == kr) || (KERN_SUCCESS == kr)) {
			ipc_kmsg_trace_send(request, option);
			goto msgdone;
		}
	}

	/* Find corresponding mig_hash entry, if any */
	ptr = find_mig_hash_entry(request_msgh_id);

	/* Get the reply_size. */
	if (ptr == (mig_hash_t *)0) {
		reply_size = sizeof(mig_reply_error_t);
	} else {
		reply_size = ptr->size;
	}

	/* round up for trailer size */
	reply_size += MAX_TRAILER_SIZE;
	reply = ipc_kmsg_alloc(reply_size);

	if (reply == IKM_NULL) {
		printf("ipc_kobject_server: dropping request\n");
		ipc_kmsg_trace_send(request, option);
		ipc_kmsg_destroy(request);
		return IKM_NULL;
	}

	/*
	 * Initialize reply message.
	 */
	{
#define InP     ((mach_msg_header_t *) request->ikm_header)
#define OutP    ((mig_reply_error_t *) reply->ikm_header)

		/*
		 * MIG should really assure no data leakage -
		 * but until it does, pessimistically zero the
		 * whole reply buffer.
		 */
		bzero((void *)OutP, reply_size);

		OutP->NDR = NDR_record;
		OutP->Head.msgh_size = sizeof(mig_reply_error_t);

		OutP->Head.msgh_bits =
		    MACH_MSGH_BITS_SET(MACH_MSGH_BITS_LOCAL(InP->msgh_bits), 0, 0, 0);
		OutP->Head.msgh_remote_port = InP->msgh_local_port;
		OutP->Head.msgh_local_port = MACH_PORT_NULL;
		OutP->Head.msgh_voucher_port = MACH_PORT_NULL;
		OutP->Head.msgh_id = InP->msgh_id + 100;

#undef  InP
#undef  OutP
	}

	/*
	 * Find the routine to call, and call it
	 * to perform the kernel function
	 */
	ipc_kmsg_trace_send(request, option);
	{
		if (ptr) {
#ifdef __DARLING__
			debug_msg("- kobject routine: %pF\n", ptr->routine);
#endif

			/*
			 * Check if the port is a task port, if its a task port then
			 * snapshot the task exec token before the mig routine call.
			 */
			if (ikot == IKOT_TASK_CONTROL) {
				task = convert_port_to_task_with_exec_token(port, &exec_token, TRUE);
			}

#if CONFIG_MACF
			int idx = ptr->kobjidx;
			task_t curtask = current_task();
			uint8_t *filter_mask = curtask->mach_kobj_filter_mask;

			/* Check kobject mig filter mask, if exists. */
			if (__improbable(filter_mask != NULL && idx != KOBJ_IDX_NOT_SET &&
			    !bitstr_test(filter_mask, idx))) {
				/* Not in filter mask, evaluate policy. */
				if (mac_task_kobj_msg_evaluate != NULL) {
					kr = mac_task_kobj_msg_evaluate(get_bsdtask_info(curtask),
					    request_msgh_id, idx);
					if (kr != KERN_SUCCESS) {
						((mig_reply_error_t *) reply->ikm_header)->RetCode = kr;
						goto skip_kobjcall;
					}
				}
			}
#endif /* CONFIG_MACF */

			(*ptr->routine)(request->ikm_header, reply->ikm_header);

#if CONFIG_MACF
skip_kobjcall:
#endif

			/* Check if the exec token changed during the mig routine */
			if (task != TASK_NULL) {
				if (exec_token != task->exec_token) {
					exec_token_changed = TRUE;
				}
				task_deallocate(task);
			}

			kernel_task->messages_received++;
		} else {
			if (!ipc_kobject_notify(request->ikm_header, reply->ikm_header)) {
#if DEVELOPMENT || DEBUG
				printf("ipc_kobject_server: bogus kernel message, id=%d\n",
				    request->ikm_header->msgh_id);
#endif  /* DEVELOPMENT || DEBUG */
				_MIG_MSGID_INVALID(request->ikm_header->msgh_id);

				((mig_reply_error_t *) reply->ikm_header)->RetCode
				        = MIG_BAD_ID;
			} else {
				kernel_task->messages_received++;
			}
		}
		kernel_task->messages_sent++;
	}

	if (!(reply->ikm_header->msgh_bits & MACH_MSGH_BITS_COMPLEX) &&
	    ((mig_reply_error_t *) reply->ikm_header)->RetCode != KERN_SUCCESS) {
		kr = ((mig_reply_error_t *) reply->ikm_header)->RetCode;
	} else {
		kr = KERN_SUCCESS;
	}

msgdone:
	/*
	 *	Destroy destination. The following code differs from
	 *	ipc_object_destroy in that we release the send-once
	 *	right instead of generating a send-once notification
	 *	(which would bring us here again, creating a loop).
	 *	It also differs in that we only expect send or
	 *	send-once rights, never receive rights.
	 *
	 *	We set msgh_remote_port to IP_NULL so that the kmsg
	 *	destroy routines don't try to destroy the port twice.
	 */
	switch (MACH_MSGH_BITS_REMOTE(request->ikm_header->msgh_bits)) {
	case MACH_MSG_TYPE_PORT_SEND:
		ipc_port_release_send(request->ikm_header->msgh_remote_port);
		request->ikm_header->msgh_remote_port = IP_NULL;
		break;

	case MACH_MSG_TYPE_PORT_SEND_ONCE:
		ipc_port_release_sonce(request->ikm_header->msgh_remote_port);
		request->ikm_header->msgh_remote_port = IP_NULL;
		break;

	default:
		panic("ipc_kobject_server: strange destination rights");
	}

	/*
	 *	Destroy voucher.  The kernel MIG servers never take ownership
	 *	of vouchers sent in messages.  Swallow any such rights here.
	 */
	if (IP_VALID(request->ikm_voucher)) {
		assert(MACH_MSG_TYPE_PORT_SEND ==
		    MACH_MSGH_BITS_VOUCHER(request->ikm_header->msgh_bits));
		ipc_port_release_send(request->ikm_voucher);
		request->ikm_voucher = IP_NULL;
	}

	if ((kr == KERN_SUCCESS) || (kr == MIG_NO_REPLY)) {
		/*
		 *	The server function is responsible for the contents
		 *	of the message.  The reply port right is moved
		 *	to the reply message, and we have deallocated
		 *	the destination port right, so we just need
		 *	to free the kmsg.
		 */
		ipc_kmsg_free(request);
	} else {
		/*
		 *	The message contents of the request are intact.
		 *	Destroy everthing except the reply port right,
		 *	which is needed in the reply message.
		 */
		request->ikm_header->msgh_local_port = MACH_PORT_NULL;
		ipc_kmsg_destroy(request);
	}

	if (kr == MIG_NO_REPLY) {
		/*
		 *	The server function will send a reply message
		 *	using the reply port right, which it has saved.
		 */

		if (reply) {
			ipc_kmsg_free(reply);
		}
		return IKM_NULL;
	}

	if (reply) {
		replyp = reply->ikm_header->msgh_remote_port;
	}

	if (!IP_VALID(replyp)) {
		/*
		 *	Can't queue the reply message if the destination
		 *	(the reply port) isn't valid.
		 */

		ipc_kmsg_destroy(reply);

		return IKM_NULL;
	} else if (replyp->ip_receiver == ipc_space_kernel) {
		/*
		 * Don't send replies to kobject kernel ports
		 */
#if DEVELOPMENT || DEBUG
		printf("%s: refusing to send reply to kobject %d port (id:%d)\n",
		    __func__, ip_kotype(replyp), request_msgh_id);
#endif  /* DEVELOPMENT || DEBUG */
		ipc_kmsg_destroy(reply);
		return IKM_NULL;
	}

	/* Fail the MIG call if the task exec token changed during the call */
	if (kr == KERN_SUCCESS && exec_token_changed) {
		/*
		 *	Create a new reply msg with error and destroy the old reply msg.
		 */
		ipc_kmsg_t new_reply = ipc_kmsg_alloc(reply_size);

		if (new_reply == IKM_NULL) {
			printf("ipc_kobject_server: dropping request\n");
			ipc_kmsg_destroy(reply);
			return IKM_NULL;
		}
		/*
		 *	Initialize the new reply message.
		 */
		{
#define OutP_new        ((mig_reply_error_t *) new_reply->ikm_header)
#define OutP_old        ((mig_reply_error_t *) reply->ikm_header)

			bzero((void *)OutP_new, reply_size);

			OutP_new->NDR = OutP_old->NDR;
			OutP_new->Head.msgh_size = sizeof(mig_reply_error_t);
			OutP_new->Head.msgh_bits = OutP_old->Head.msgh_bits & ~MACH_MSGH_BITS_COMPLEX;
			OutP_new->Head.msgh_remote_port = OutP_old->Head.msgh_remote_port;
			OutP_new->Head.msgh_local_port = MACH_PORT_NULL;
			OutP_new->Head.msgh_voucher_port = MACH_PORT_NULL;
			OutP_new->Head.msgh_id = OutP_old->Head.msgh_id;

			/* Set the error as KERN_INVALID_TASK */
			OutP_new->RetCode = KERN_INVALID_TASK;

#undef  OutP_new
#undef  OutP_old
		}

		/*
		 *	Destroy everything in reply except the reply port right,
		 *	which is needed in the new reply message.
		 */
		reply->ikm_header->msgh_remote_port = MACH_PORT_NULL;
		ipc_kmsg_destroy(reply);

		reply = new_reply;
	}

	trailer = (mach_msg_max_trailer_t *)
	    ((vm_offset_t)reply->ikm_header + (int)reply->ikm_header->msgh_size);
	bzero(trailer, sizeof(*trailer));
	trailer->msgh_sender = KERNEL_SECURITY_TOKEN;
	trailer->msgh_audit = KERNEL_AUDIT_TOKEN;
	trailer->msgh_trailer_type = MACH_MSG_TRAILER_FORMAT_0;
	trailer->msgh_trailer_size = MACH_MSG_TRAILER_MINIMUM_SIZE;

	return reply;
}

/*
 *	Routine:	ipc_kobject_set
 *	Purpose:
 *		Make a port represent a kernel object of the given type.
 *		The caller is responsible for handling refs for the
 *		kernel object, if necessary.
 *	Conditions:
 *		Nothing locked.  The port must be active if setting
 *		a kobject linkage.  Clearing a linkage is OK on an
 *		inactive port.
 */
void
ipc_kobject_set(
	ipc_port_t                      port,
	ipc_kobject_t           kobject,
	ipc_kobject_type_t      type)
{
	ip_lock(port);
	ipc_kobject_set_atomically(port, kobject, type);
	ip_unlock(port);
}

void
ipc_kobject_set_atomically(
	ipc_port_t                      port,
	ipc_kobject_t           kobject,
	ipc_kobject_type_t      type)
{
	assert(type == IKOT_NONE || ip_active(port));
#if     MACH_ASSERT
	port->ip_spares[2] = (port->ip_object.io_bits & IO_BITS_KOTYPE);
#endif  /* MACH_ASSERT */
	port->ip_object.io_bits = (port->ip_object.io_bits & ~IO_BITS_KOTYPE) | type;
	if (ip_is_kolabeled(port)) {
		ipc_kobject_label_t labelp = port->ip_kolabel;
		labelp->ikol_kobject = kobject;
	} else {
		port->ip_kobject = kobject;
	}
	if (type != IKOT_NONE) {
		/* Once set, this bit can never be unset */
		port->ip_object.io_bits |= IO_BITS_KOBJECT;
	}
}

/*
 *	Routine:	ipc_kobject_init_port
 *	Purpose:
 *		Initialize a kobject port with the given types and options.
 *
 *		This function never fails.
 */
static inline void
ipc_kobject_init_port(
	ipc_port_t port,
	ipc_kobject_t kobject,
	ipc_kobject_type_t type,
	ipc_kobject_alloc_options_t options)
{
	ipc_kobject_set_atomically(port, kobject, type);

	if (options & IPC_KOBJECT_ALLOC_MAKE_SEND) {
		ipc_port_make_send_locked(port);
	}
	if (options & IPC_KOBJECT_ALLOC_NSREQUEST) {
		ipc_port_make_sonce_locked(port);
		port->ip_nsrequest = port;
	}
	if (options & IPC_KOBJECT_ALLOC_NO_GRANT) {
		port->ip_no_grant = 1;
	}
	if (options & IPC_KOBJECT_ALLOC_IMMOVABLE_SEND) {
		port->ip_immovable_send = 1;
	}
	if (options & IPC_KOBJECT_ALLOC_PINNED) {
		port->ip_pinned = 1;
	}
}

/*
 *	Routine:	ipc_kobject_alloc_port
 *	Purpose:
 *		Allocate a kobject port in the kernel space of the specified type.
 *
 *		This function never fails.
 *
 *	Conditions:
 *		No locks held (memory is allocated)
 */
ipc_port_t
ipc_kobject_alloc_port(
	ipc_kobject_t           kobject,
	ipc_kobject_type_t      type,
	ipc_kobject_alloc_options_t     options)
{
	ipc_port_t port = ipc_port_alloc_kernel();

	if (port == IP_NULL) {
		panic("ipc_kobject_alloc_port(): failed to allocate port");
	}

	ipc_kobject_init_port(port, kobject, type, options);
	return port;
}

/*
 *	Routine:	ipc_kobject_alloc_labeled_port
 *	Purpose:
 *		Allocate a kobject port and associated mandatory access label
 *		in the kernel space of the specified type.
 *
 *		This function never fails.
 *
 *	Conditions:
 *		No locks held (memory is allocated)
 */

ipc_port_t
ipc_kobject_alloc_labeled_port(
	ipc_kobject_t           kobject,
	ipc_kobject_type_t      type,
	ipc_label_t             label,
	ipc_kobject_alloc_options_t     options)
{
	ipc_port_t port;
	ipc_kobject_label_t labelp;

	port = ipc_port_alloc_kernel();
	if (port == IP_NULL) {
		panic("ipc_kobject_alloc_port(): failed to allocate port");
	}

	labelp = (ipc_kobject_label_t)zalloc(ipc_kobject_label_zone);
	if (labelp == NULL) {
		panic("ipc_kobject_alloc_labeled_port(): failed to allocate label");
	}
	labelp->ikol_label = label;
	port->ip_kolabel = labelp;
	port->ip_object.io_bits |= IO_BITS_KOLABEL;

	ipc_kobject_init_port(port, kobject, type, options);
	return port;
}

static void
ipc_kobject_subst_once_notify(mach_msg_header_t *msg)
{
	mach_no_senders_notification_t *notification = (void *)msg;
	ipc_port_t port = notification->not_header.msgh_remote_port;

	require_ip_active(port);
	assert(IKOT_PORT_SUBST_ONCE == ip_kotype(port));

	ip_release((ipc_port_t)ip_get_kobject(port));
	ipc_port_dealloc_kernel(port);
}

/*
 *	Routine:	ipc_kobject_alloc_subst_once
 *	Purpose:
 *		Make a port that will be substituted by the kolabel
 *		rules once, preventing the next substitution (of its target)
 *		to happen if any.
 *
 *	Returns:
 *		A port with a send right, that will substitute to its "kobject".
 *
 *	Conditions:
 *		No locks held (memory is allocated)
 *		`target` has a refcount that this function consumes
 */
ipc_port_t
ipc_kobject_alloc_subst_once(
	ipc_port_t                  target)
{
	return ipc_kobject_alloc_labeled_port(target,
	           IKOT_PORT_SUBST_ONCE, IPC_LABEL_SUBST_ONCE,
	           IPC_KOBJECT_ALLOC_MAKE_SEND | IPC_KOBJECT_ALLOC_NSREQUEST);
}

/*
 *	Routine:	ipc_kobject_make_send_lazy_alloc_port
 *	Purpose:
 *		Make a send once for a kobject port.
 *
 *		A location owning this port is passed in port_store.
 *		If no port exists, a port is made lazily.
 *
 *		A send right is made for the port, and if this is the first one
 *		(possibly not for the first time), then the no-more-senders
 *		notification is rearmed.
 *
 *		When a notification is armed, the kobject must donate
 *		one of its references to the port. It is expected
 *		the no-more-senders notification will consume this reference.
 *
 *	Returns:
 *		TRUE if a notification was armed
 *		FALSE else
 *
 *	Conditions:
 *		Nothing is locked, memory can be allocated.
 *		The caller must be able to donate a kobject reference to the port.
 */
boolean_t
ipc_kobject_make_send_lazy_alloc_port(
	ipc_port_t              *port_store,
	ipc_kobject_t           kobject,
	ipc_kobject_type_t      type,
	ipc_kobject_alloc_options_t alloc_opts,
	boolean_t               __ptrauth_only should_ptrauth,
	uint64_t                __ptrauth_only ptrauth_discriminator)
{
	ipc_port_t port, previous, __ptrauth_only port_addr;
	boolean_t rc = FALSE;

	port = os_atomic_load(port_store, dependency);

#if __has_feature(ptrauth_calls)
	/* If we're on a ptrauth system and this port is signed, authenticate and strip the pointer */
	if (should_ptrauth && IP_VALID(port)) {
		port = ptrauth_auth_data(port,
		    ptrauth_key_process_independent_data,
		    ptrauth_blend_discriminator(port_store, ptrauth_discriminator));
	}
#endif // __has_feature(ptrauth_calls)

	if (!IP_VALID(port)) {
		port = ipc_kobject_alloc_port(kobject, type,
		    IPC_KOBJECT_ALLOC_MAKE_SEND | IPC_KOBJECT_ALLOC_NSREQUEST | alloc_opts);

#if __has_feature(ptrauth_calls)
		if (should_ptrauth) {
			port_addr = ptrauth_sign_unauthenticated(port,
			    ptrauth_key_process_independent_data,
			    ptrauth_blend_discriminator(port_store, ptrauth_discriminator));
		} else {
			port_addr = port;
		}
#else
		port_addr = port;
#endif // __has_feature(ptrauth_calls)

		if (os_atomic_cmpxchgv(port_store, IP_NULL, port_addr, &previous, release)) {
			return TRUE;
		}

		// undo what ipc_kobject_alloc_port() did above
		port->ip_nsrequest = IP_NULL;
		port->ip_mscount = 0;
		port->ip_sorights = 0;
		port->ip_srights = 0;
		ip_release(port);
		ip_release(port);
		ipc_port_dealloc_kernel(port);

		port = previous;
	}

	ip_lock(port);
	ipc_port_make_send_locked(port);
	if (port->ip_srights == 1) {
		ipc_port_make_sonce_locked(port);
		assert(port->ip_nsrequest == IP_NULL);
		port->ip_nsrequest = port;
		rc = TRUE;
	}
	ip_unlock(port);

	return rc;
}

/*
 *	Routine:	ipc_kobject_make_send_lazy_alloc_labeled_port
 *	Purpose:
 *		Make a send once for a kobject port.
 *
 *		A location owning this port is passed in port_store.
 *		If no port exists, a port is made lazily.
 *
 *		A send right is made for the port, and if this is the first one
 *		(possibly not for the first time), then the no-more-senders
 *		notification is rearmed.
 *
 *		When a notification is armed, the kobject must donate
 *		one of its references to the port. It is expected
 *		the no-more-senders notification will consume this reference.
 *
 *	Returns:
 *		TRUE if a notification was armed
 *		FALSE else
 *
 *	Conditions:
 *		Nothing is locked, memory can be allocated.
 *		The caller must be able to donate a kobject reference to the port.
 */
boolean_t
ipc_kobject_make_send_lazy_alloc_labeled_port(
	ipc_port_t              *port_store,
	ipc_kobject_t           kobject,
	ipc_kobject_type_t      type,
	ipc_label_t             label)
{
	ipc_port_t port, previous;
	boolean_t rc = FALSE;

	port = os_atomic_load(port_store, dependency);

	if (!IP_VALID(port)) {
		port = ipc_kobject_alloc_labeled_port(kobject, type, label,
		    IPC_KOBJECT_ALLOC_MAKE_SEND | IPC_KOBJECT_ALLOC_NSREQUEST);
		if (os_atomic_cmpxchgv(port_store, IP_NULL, port, &previous, release)) {
			return TRUE;
		}

		// undo what ipc_kobject_alloc_port() did above
		port->ip_nsrequest = IP_NULL;
		port->ip_mscount = 0;
		port->ip_sorights = 0;
		port->ip_srights = 0;
		ip_release(port);
		ip_release(port);
		zfree(ipc_kobject_label_zone, port->ip_kolabel);
		port->ip_object.io_bits &= ~IO_BITS_KOLABEL;
		port->ip_kolabel = NULL;
		ipc_port_dealloc_kernel(port);

		port = previous;
		assert(ip_is_kolabeled(port));
	}

	ip_lock(port);
	ipc_port_make_send_locked(port);
	if (port->ip_srights == 1) {
		ipc_port_make_sonce_locked(port);
		assert(port->ip_nsrequest == IP_NULL);
		port->ip_nsrequest = port;
		rc = TRUE;
	}
	ip_unlock(port);

	return rc;
}


/*
 *	Routine:	ipc_kobject_destroy
 *	Purpose:
 *		Release any kernel object resources associated
 *		with the port, which is being destroyed.
 *
 *		This path to free object resources should only be
 *      needed when resources are associated with a user's port.
 *      In the normal case, when the kernel is the receiver,
 *      the code calling ipc_port_dealloc_kernel should clean
 *      up the object resources.
 *
 *      Cleans up any kobject label that might be present.
 *	Conditions:
 *		The port is not locked, but it is dead.
 */

void
ipc_kobject_destroy(
	ipc_port_t              port)
{
	switch (ip_kotype(port)) {
	case IKOT_TIMER:
		mk_timer_port_destroy(port);
		break;

	case IKOT_NAMED_ENTRY:
		mach_destroy_memory_entry(port);
		break;

	case IKOT_HOST_NOTIFY:
		host_notify_port_destroy(port);
		break;

	case IKOT_SUID_CRED:
		suid_cred_destroy(port);
		break;

	default:
		break;
	}

	if (ip_is_kolabeled(port)) {
		ipc_kobject_label_t labelp = port->ip_kolabel;

		assert(labelp != NULL);
		assert(ip_is_kobject(port));
		port->ip_kolabel = NULL;
		port->ip_object.io_bits &= ~IO_BITS_KOLABEL;
		zfree(ipc_kobject_label_zone, labelp);
	}
}

/*
 *	Routine:	ipc_kobject_label_substitute_task
 *	Purpose:
 *		Substitute a task control port for its immovable
 *		equivalent when the receiver is that task.
 *	Conditions:
 *		Space is write locked and active.
 *		Port is locked and active.
 *	Returns:
 *		- IP_NULL port if no substitution is to be done
 *		- a valid port if a substitution needs to happen
 */
static ipc_port_t
ipc_kobject_label_substitute_task(
	ipc_space_t             space,
	ipc_port_t              port)
{
	ipc_port_t subst = IP_NULL;
	task_t task = ipc_kobject_get(port);

	if (task != TASK_NULL && task == space->is_task) {
		if ((subst = port->ip_alt_port)) {
			return subst;
		}
	}

	return IP_NULL;
}

/*
 *	Routine:	ipc_kobject_label_substitute_thread
 *	Purpose:
 *		Substitute a thread control port for its immovable
 *		equivalent when it belongs to the receiver task.
 *	Conditions:
 *		Space is write locked and active.
 *		Port is locked and active.
 *	Returns:
 *		- IP_NULL port if no substitution is to be done
 *		- a valid port if a substitution needs to happen
 */
static ipc_port_t
ipc_kobject_label_substitute_thread(
	ipc_space_t             space,
	ipc_port_t              port)
{
	ipc_port_t subst = IP_NULL;
	thread_t thread = ipc_kobject_get(port);

	if (thread != THREAD_NULL && space->is_task == thread->task) {
		if ((subst = port->ip_alt_port) != IP_NULL) {
			return subst;
		}
	}

	return IP_NULL;
}

/*
 *	Routine:	ipc_kobject_label_check
 *	Purpose:
 *		Check to see if the space is allowed to possess
 *		a right for the given port. In order to qualify,
 *		the space label must contain all the privileges
 *		listed in the port/kobject label.
 *
 *	Conditions:
 *		Space is write locked and active.
 *		Port is locked and active.
 *
 *	Returns:
 *		Whether the copyout is authorized.
 *
 *		If a port substitution is requested, the space is unlocked,
 *		the port is unlocked and its "right" consumed.
 *
 *		As of now, substituted ports only happen for send rights.
 */
bool
ipc_kobject_label_check(
	ipc_space_t                     space,
	ipc_port_t                      port,
	mach_msg_type_name_t            msgt_name,
	ipc_object_copyout_flags_t     *flags,
	ipc_port_t                     *subst_portp)
{
	ipc_kobject_label_t labelp;
	ipc_label_t label;

	assert(is_active(space));
	assert(ip_active(port));

	*subst_portp = IP_NULL;

	/* Unlabled ports/kobjects are always allowed */
	if (!ip_is_kolabeled(port)) {
		return true;
	}

	/* Never OK to copyout the receive right for a labeled kobject */
	if (msgt_name == MACH_MSG_TYPE_PORT_RECEIVE) {
		panic("ipc_kobject_label_check: attempted receive right "
		    "copyout for labeled kobject");
	}

	labelp = port->ip_kolabel;
	label = labelp->ikol_label;

	if ((*flags & IPC_OBJECT_COPYOUT_FLAGS_NO_LABEL_CHECK) == 0 &&
	    (label & IPC_LABEL_SUBST_MASK)) {
		ipc_port_t subst = IP_NULL;

		if (msgt_name != MACH_MSG_TYPE_PORT_SEND) {
			return false;
		}

		switch (label & IPC_LABEL_SUBST_MASK) {
		case IPC_LABEL_SUBST_TASK:
			subst = ipc_kobject_label_substitute_task(space, port);
			break;
		case IPC_LABEL_SUBST_THREAD:
			subst = ipc_kobject_label_substitute_thread(space, port);
			break;
		case IPC_LABEL_SUBST_ONCE:
			/* the next check will _not_ substitute */
			*flags |= IPC_OBJECT_COPYOUT_FLAGS_NO_LABEL_CHECK;
			subst = ip_get_kobject(port);
			break;
		default:
			panic("unexpected label: %llx\n", label);
		}

		if (subst != IP_NULL) {
			ip_reference(subst);
			is_write_unlock(space);
			ipc_port_release_send_and_unlock(port);
			port = ipc_port_make_send(subst);
			ip_release(subst);
			*subst_portp = port;
			return true;
		}
	}

	return (label & space->is_label & IPC_LABEL_SPACE_MASK) ==
	       (label & IPC_LABEL_SPACE_MASK);
}

boolean_t
ipc_kobject_notify(
	mach_msg_header_t *request_header,
	mach_msg_header_t *reply_header)
{
	mach_msg_max_trailer_t * trailer;
	ipc_port_t port = request_header->msgh_remote_port;

	((mig_reply_error_t *) reply_header)->RetCode = MIG_NO_REPLY;

	trailer = (mach_msg_max_trailer_t *)
	    ((vm_offset_t)request_header + request_header->msgh_size);

	/*
	 * The kobject notification is privileged and can change the
	 * refcount on kernel-internal objects - make sure
	 * that the message wasn't faked!
	 */
	if (0 != bcmp(&trailer->msgh_audit, &KERNEL_AUDIT_TOKEN,
	    sizeof(trailer->msgh_audit))) {
		return FALSE;
	}
	if (0 != bcmp(&trailer->msgh_sender, &KERNEL_SECURITY_TOKEN,
	    sizeof(trailer->msgh_sender))) {
		return FALSE;
	}

	switch (request_header->msgh_id) {
	case MACH_NOTIFY_NO_SENDERS:
		switch (ip_kotype(port)) {
		case IKOT_VOUCHER:
			ipc_voucher_notify(request_header);
			return TRUE;

		case IKOT_VOUCHER_ATTR_CONTROL:
			ipc_voucher_attr_control_notify(request_header);
			return TRUE;

		case IKOT_PORT_SUBST_ONCE:
			ipc_kobject_subst_once_notify(request_header);
			return TRUE;

		case IKOT_SEMAPHORE:
			semaphore_notify(request_header);
			return TRUE;

		case IKOT_EVENTLINK:
			ipc_eventlink_notify(request_header);
			return TRUE;

		case IKOT_TASK_CONTROL:
			task_port_notify(request_header);
			return TRUE;

		case IKOT_NAMED_ENTRY:
			ip_lock(port);

			/*
			 * Bring the sequence number and mscount in
			 * line with ipc_port_destroy assertion.
			 */
			port->ip_mscount = 0;
			port->ip_messages.imq_seqno = 0;
			ipc_port_destroy(port);         /* releases lock */
			return TRUE;

		case IKOT_UPL:
			upl_no_senders(
				request_header->msgh_remote_port,
				(mach_port_mscount_t)
				((mach_no_senders_notification_t *)
				request_header)->not_count);
			reply_header->msgh_remote_port = MACH_PORT_NULL;
			return TRUE;

#if     CONFIG_AUDIT
		case IKOT_AU_SESSIONPORT:
			audit_session_nosenders(request_header);
			return TRUE;
#endif
		case IKOT_FILEPORT:
			fileport_notify(request_header);
			return TRUE;

		case IKOT_WORK_INTERVAL:
			work_interval_port_notify(request_header);
			return TRUE;
		case IKOT_TASK_READ:
		case IKOT_TASK_INSPECT:
			task_port_with_flavor_notify(request_header);
			return TRUE;
		case IKOT_THREAD_READ:
		case IKOT_THREAD_INSPECT:
			thread_port_with_flavor_notify(request_header);
			return TRUE;
		case IKOT_SUID_CRED:
			suid_cred_notify(request_header);
			return TRUE;
		case IKOT_TASK_ID_TOKEN:
			task_id_token_notify(request_header);
			return TRUE;
#if HYPERVISOR
		case IKOT_HYPERVISOR:
			hv_port_notify(request_header);
			return TRUE;
#endif
		}

		break;

	case MACH_NOTIFY_PORT_DELETED:
	case MACH_NOTIFY_PORT_DESTROYED:
	case MACH_NOTIFY_SEND_ONCE:
	case MACH_NOTIFY_DEAD_NAME:
		break;

	default:
		return FALSE;
	}
	switch (ip_kotype(port)) {
#ifdef IOKIT
	case IKOT_IOKIT_OBJECT:
	case IKOT_IOKIT_CONNECT:
	case IKOT_IOKIT_IDENT:
	case IKOT_UEXT_OBJECT:
	{
		return iokit_notify(request_header);
	}
#endif
	case IKOT_TASK_RESUME:
	{
		return task_suspension_notify(request_header);
	}

	default:
		return FALSE;
	}
}
