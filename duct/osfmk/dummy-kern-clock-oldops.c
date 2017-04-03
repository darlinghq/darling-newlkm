/*
 * Copyright (c) 2000-2008 Apple Inc. All rights reserved.
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
 *  DEPRECATED INTERFACES - Should be removed
 *
 *  Purpose:    Routines for the creation and use of kernel
 *          alarm clock services. This file and the ipc
 *          routines in kern/ipc_clock.c constitute the
 *          machine-independent clock service layer.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <mach/mach_types.h>

#include <kern/lock.h>
#include <kern/host.h>
#include <kern/spl.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <kern/ipc_host.h>
#include <kern/clock.h>
#include <kern/zalloc.h>

#include <ipc/ipc_types.h>
#include <ipc/ipc_port.h>

#include <mach/mach_traps.h>
#include <mach/mach_time.h>

#include <mach/clock_server.h>
#include <mach/clock_reply.h>
#include <mach/clock_priv_server.h>

#include <mach/mach_host_server.h>
#include <mach/host_priv_server.h>

/*
 * Actual clock alarm structure. Used for user clock_sleep() and
 * clock_alarm() calls. Alarms are allocated from the alarm free
 * list and entered in time priority order into the active alarm
 * chain of the target clock.
 */
struct  alarm {
    struct  alarm   *al_next;       /* next alarm in chain */
    struct  alarm   *al_prev;       /* previous alarm in chain */
    int             al_status;      /* alarm status */
    mach_timespec_t al_time;        /* alarm time */
    struct {                /* message alarm data */
        int             type;       /* alarm type */
        ipc_port_t      port;       /* alarm port */
        mach_msg_type_name_t
                        port_type;  /* alarm port type */
        struct  clock   *clock;     /* alarm clock */
        void            *data;      /* alarm data */
    } al_alrm;
#define al_type     al_alrm.type
#define al_port     al_alrm.port
#define al_port_type    al_alrm.port_type
#define al_clock    al_alrm.clock
#define al_data     al_alrm.data
    long            al_seqno;       /* alarm sequence number */
};
typedef struct alarm    alarm_data_t;

/* alarm status */
#define ALARM_FREE  0       /* alarm is on free list */
#define ALARM_SLEEP 1       /* active clock_sleep() */
#define ALARM_CLOCK 2       /* active clock_alarm() */
#define ALARM_DONE  4       /* alarm has expired */

/* local data declarations */
decl_simple_lock_data(static,alarm_lock)    /* alarm synchronization */
static struct   zone        *alarm_zone;    /* zone for user alarms */
static struct   alarm       *alrmfree;      /* alarm free list pointer */
static struct   alarm       *alrmdone;      /* alarm done list pointer */
static struct   alarm       *alrmlist;
static long                 alrm_seqno;     /* uniquely identifies alarms */
static thread_call_data_t   alarm_done_call;
static timer_call_data_t    alarm_expire_timer;

extern  struct clock    clock_list[];
extern  int     clock_count;

// static void     post_alarm(
//                     alarm_t         alarm);

// static void     set_alarm(
//                     mach_timespec_t *alarm_time);

// static int      check_time(
//                     alarm_type_t    alarm_type,
//                     mach_timespec_t *alarm_time,
//                     mach_timespec_t *clock_time);

// static void     alarm_done(void);

// static void     alarm_expire(void);

// static kern_return_t    clock_sleep_internal(
//                             clock_t             clock,
//                             sleep_type_t        sleep_type,
//                             mach_timespec_t     *sleep_time);

int
rtclock_config(void)
{
        kprintf("not implemented: rtclock_config()\n");
        return 0;
}

// int
// rtclock_init(void)
// {
//         kprintf("not implemented: rtclock_init()\n");
//         return 0;
// }

kern_return_t   rtclock_gettime(
    mach_timespec_t         *cur_time);

kern_return_t   rtclock_getattr(
    clock_flavor_t          flavor,
    clock_attr_t            attr,
    mach_msg_type_number_t  *count);

// struct clock_ops sysclk_ops = {
//     rtclock_config,         rtclock_init,
//     rtclock_gettime,
//     rtclock_getattr,
// };

kern_return_t   calend_gettime(
    mach_timespec_t         *cur_time);

kern_return_t   calend_getattr(
    clock_flavor_t          flavor,
    clock_attr_t            attr,
    mach_msg_type_number_t  *count);

// struct clock_ops calend_ops = {
//     NULL, NULL,
//     calend_gettime,
//     calend_getattr,
// };

/*
 *  Macros to lock/unlock clock system.
 */
#define LOCK_ALARM(s)           \
    s = splclock();         \
    simple_lock(&alarm_lock);

#define UNLOCK_ALARM(s)         \
    simple_unlock(&alarm_lock); \
    splx(s);

// void
// clock_oldconfig(void)
// {
//     ;
// }

// void
// clock_oldinit(void)
// {
//     ;
// }

/*
 * Initialize the clock ipc service facility.
 */
// void
// clock_service_create(void)
// {
//     ;
// }

/*
 * Get the service port on a clock.
 */
// kern_return_t
// host_get_clock_service(
//     host_t          host,
//     clock_id_t      clock_id,
//     clock_t         *clock)     /* OUT */
// {
//     return 0;
// }

/*
 * Get the control port on a clock.
 */
// kern_return_t
// host_get_clock_control(
//     host_priv_t     host_priv,
//     clock_id_t      clock_id,
//     clock_t         *clock)     /* OUT */
// {
//     return 0;
// }

/*
 * Get the current clock time.
 */
// kern_return_t
// clock_get_time(
//     clock_t         clock,
//     mach_timespec_t *cur_time)  /* OUT */
// {
//     return 0;
// }

// kern_return_t
// rtclock_gettime(
//     mach_timespec_t     *time)  /* OUT */
// {
//     return 0;
// }

// kern_return_t
// calend_gettime(
//     mach_timespec_t     *time)  /* OUT */
// {
//     return 0;
// }

/*
 * Get clock attributes.
 */
// kern_return_t
// clock_get_attributes(
//     clock_t                 clock,
//     clock_flavor_t          flavor,
//     clock_attr_t            attr,       /* OUT */
//     mach_msg_type_number_t  *count)     /* IN/OUT */
// {
//     return 0;
// }

// kern_return_t
// rtclock_getattr(
//     clock_flavor_t          flavor,
//     clock_attr_t            attr,       /* OUT */
//     mach_msg_type_number_t  *count)     /* IN/OUT */
// {
//     return 0;
// }

// kern_return_t
// calend_getattr(
//     clock_flavor_t          flavor,
//     clock_attr_t            attr,       /* OUT */
//     mach_msg_type_number_t  *count)     /* IN/OUT */
// {
//     return 0;
// }

/*
 * Set the current clock time.
 */
// kern_return_t
// clock_set_time(
//     clock_t                 clock,
// __unused mach_timespec_t    new_time)
// {
//     return 0;
// }

/*
 * Set the clock alarm resolution.
 */
// kern_return_t
// clock_set_attributes(
//     clock_t                     clock,
// __unused clock_flavor_t         flavor,
// __unused clock_attr_t           attr,
// __unused mach_msg_type_number_t count)
// {
//     return 0;
// }

/*
 * Setup a clock alarm.
 */
// kern_return_t
// clock_alarm(
//     clock_t                 clock,
//     alarm_type_t            alarm_type,
//     mach_timespec_t         alarm_time,
//     ipc_port_t              alarm_port,
//     mach_msg_type_name_t    alarm_port_type)
// {
//     return 0;
// }

/*
 * Sleep on a clock. System trap. User-level libmach clock_sleep
 * interface call takes a mach_timespec_t sleep_time argument which it
 * converts to sleep_sec and sleep_nsec arguments which are then
 * passed to clock_sleep_trap.
 */
// kern_return_t
// clock_sleep_trap(
//     struct clock_sleep_trap_args *args)
// {
//     return 0;
// }

// static kern_return_t
// clock_sleep_internal(
//     clock_t             clock,
//     sleep_type_t        sleep_type,
//     mach_timespec_t     *sleep_time)
// {
//     return 0;
// }

/*
 * Service clock alarm expirations.
 */
// static void
// alarm_expire(void)
// {
//     ;
// }

// static void
// alarm_done(void)
// {
//     ;
// }

/*
 * Post an alarm on the active alarm list.
 *
 * Always called from within a LOCK_ALARM() code section.
 */
// static void
// post_alarm(
//     alarm_t             alarm)
// {
//     ;
// }

// static void
// set_alarm(
//     mach_timespec_t     *alarm_time)
// {
//     ;
// }

/*
 * Check the validity of 'alarm_time' and 'alarm_type'. If either
 * argument is invalid, return a negative value. If the 'alarm_time'
 * is now, return a 0 value. If the 'alarm_time' is in the future,
 * return a positive value.
 */
// static int
// check_time(
//     alarm_type_t        alarm_type,
//     mach_timespec_t     *alarm_time,
//     mach_timespec_t     *clock_time)
// {
//     return 0;
// }
