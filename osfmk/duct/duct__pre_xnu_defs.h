/*
Copyright (c) 2014-2017, Wenqi Chen

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

// linux/include/asm-generic/resource.h
#define LINUX_RLIMIT_CPU            0     /* CPU time in sec */
#define LINUX_RLIMIT_FSIZE          1     /* Maximum filesize */
#define LINUX_RLIMIT_DATA           2     /* max data size */
#define LINUX_RLIMIT_STACK          3     /* max stack size */
#define LINUX_RLIMIT_CORE           4     /* max core file size */
#define LINUX_RLIMIT_RSS            5     /* max resident set size */
#define LINUX_RLIMIT_NPROC          6     /* max number of processes */
#define LINUX_RLIMIT_NOFILE         7     /* max number of open files */
#define LINUX_RLIMIT_MEMLOCK        8     /* max locked-in-memory address space */
#define LINUX_RLIMIT_AS             9     /* address space limit */
#define LINUX_RLIMIT_LOCKS          10    /* maximum file locks held */
#define LINUX_RLIMIT_SIGPENDING     11    /* max number of pending signals */
#define LINUX_RLIMIT_MSGQUEUE       12    /* maximum bytes in POSIX mqueues */
#define LINUX_RLIMIT_NICE           13    /* max nice prio allowed to raise to 0-39 for nice level 19 .. -20 */
#define LINUX_RLIMIT_RTPRIO         14    /* maximum realtime priority */
#define LINUX_RLIMIT_RTTIME         15    /* timeout for RT tasks in us */
#define LINUX_RLIM_NLIMITS          16
#define LINUX_RLIM_INFINITY         (~0UL)

#undef RLIMIT_CPU
#undef RLIMIT_FSIZE
#undef RLIMIT_DATA
#undef RLIMIT_STACK
#undef RLIMIT_CORE
#undef RLIMIT_RSS
#undef RLIMIT_NPROC
#undef RLIMIT_NOFILE
#undef RLIMIT_MEMLOCK
#undef RLIMIT_AS
#undef RLIMIT_LOCKS
#undef RLIMIT_SIGPENDING
#undef RLIMIT_MSGQUEUE
#undef RLIMIT_NICE
#undef RLIMIT_RTPRIO
#undef RLIMIT_RTTIME
#undef RLIM_NLIMITS
#undef RLIM_INFINITY


// linux/include/kernel.h
#define LINUX_USHRT_MAX         ((u16)(~0U))
#define LINUX_SHRT_MAX          ((s16)(LINUX_USHRT_MAX>>1))
#define LINUX_SHRT_MIN          ((s16)(-LINUX_SHRT_MAX - 1))
#define LINUX_INT_MAX           ((int)(~0U>>1))
#define LINUX_INT_MIN           (-LINUX_INT_MAX - 1)
#define LINUX_UINT_MAX          (~0U)
#define LINUX_LONG_MAX          ((long)(~0UL>>1))
#define LINUX_LONG_MIN          (-LINUX_LONG_MAX - 1)
#define LINUX_ULONG_MAX         (~0UL)
#define LINUX_LLONG_MAX         ((long long)(~0ULL>>1))
#define LINUX_LLONG_MIN         (-LINUX_LLONG_MAX - 1)
#define LINUX_ULLONG_MAX        (~0ULL)

#undef USHRT_MAX
#undef SHRT_MAX
#undef SHRT_MIN
#undef INT_MAX
#undef INT_MIN
#undef UINT_MAX
#undef LONG_MAX
#undef LONG_MIN
#undef ULONG_MAX
#undef LLONG_MAX
#undef LLONG_MIN
#undef ULLONG_MAX

// linux/include/linux/sched.h
#define LINUX_MAX_SCHEDULE_TIMEOUT      LINUX_LONG_MAX
#undef MAX_SCHEDULE_TIMEOUT



// linux/arch/arm/include/asm/current.h
#define linux_current       (get_current())
#undef current

// linux/include/linux/wait.h
#undef init_wait
#define linux_init_wait(wait) do { \
    (wait)->private     = linux_current;        \
    (wait)->func        = autoremove_wake_function;     \
    INIT_LIST_HEAD (&(wait)->task_list);        \
    (wait)->flags       = 0;                    \
} while (0)

#define linux_init_wait_proc(wait, task) do { \
    (wait)->private     = task;        \
    (wait)->func        = autoremove_wake_function;     \
    INIT_LIST_HEAD (&(wait)->task_list);        \
    (wait)->flags       = 0;                    \
} while (0)

#define linux_init_wait_func_proc(wait, funcp, task) do { \
    (wait)->private     = task;        \
    (wait)->func        = funcp;     \
    INIT_LIST_HEAD (&(wait)->task_list);        \
    (wait)->flags       = 0;                    \
} while (0)



// linux/include/linux/list.h
#define LINUX_LIST_HEAD_INIT(name) { &(name), &(name) }

#define LINUX_LIST_HEAD(name) \
        struct list_head name   = LINUX_LIST_HEAD_INIT (name)

#undef LIST_HEAD_INIT
#undef LIST_HEAD

// for arch/arm/include/asm/signal.h
/*
 * SA_FLAGS values:
 *
 * SA_NOCLDSTOP     flag to turn off SIGCHLD when children stop.
 * SA_NOCLDWAIT     flag on SIGCHLD to inhibit zombies.
 * SA_SIGINFO       deliver the signal with SIGINFO structs
 * SA_THIRTYTWO     delivers the signal in 32-bit mode, even if the task
 *          is running in 26-bit.
 * SA_ONSTACK       allows alternate signal stacks (see sigaltstack(2)).
 * SA_RESTART       flag to get restarting signals (which were the default long ago)
 * SA_NODEFER       prevents the current signal from being masked in the handler.
 * SA_RESETHAND     clears the handler when the signal is delivered.
 *
 * SA_ONESHOT and SA_NOMASK are the historical Linux names for the Single
 * Unix names RESETHAND and NODEFER respectively.
 */
#define LINUX_SA_NOCLDSTOP      0x00000001
#define LINUX_SA_NOCLDWAIT      0x00000002
#define LINUX_SA_SIGINFO        0x00000004
#define LINUX_SA_THIRTYTWO      0x02000000
#define LINUX_SA_RESTORER       0x04000000
#define LINUX_SA_ONSTACK        0x08000000
#define LINUX_SA_RESTART        0x10000000
#define LINUX_SA_NODEFER        0x40000000
#define LINUX_SA_RESETHAND      0x80000000

#define LINUX_SA_NOMASK   LINUX_SA_NODEFER
#define LINUX_SA_ONESHOT  LINUX_SA_RESETHAND

#undef SA_NOCLDSTOP
#undef SA_NOCLDWAIT
#undef SA_SIGINFO
#undef SA_THIRTYTWO
#undef SA_RESTORER
#undef SA_ONSTACK
#undef SA_RESTART
#undef SA_NODEFER
#undef SA_RESETHAND

// asm/include/signal.h
#undef MINSIGSTKSZ
#undef SIGSTKSZ

// for include/linux/signal.h
#undef sigmask

// for linux/include/asm-generic/siginfo.h
#undef SIGEV_NONE
#undef SIGEV_SIGNAL
#undef SIGEV_THREAD

#undef si_pid
#undef si_uid
#undef si_status
#undef si_addr
#undef si_value
#undef si_band

#undef sigev_notify_function
#undef sigev_notify_attributes

// include/asm-generic/signal-defs.h
#define LINUX_SIG_BLOCK         0       /* for blocking signals */
#define LINUX_SIG_UNBLOCK       1       /* for unblocking signals */
#define LINUX_SIG_SETMASK       2       /* for setting the signal mask */

#undef SIG_BLOCK
#undef SIG_UNBLOCK
#undef SIG_SETMASK

#define LINUX_SIG_DFL ((__force __sighandler_t)0)       /* default signal handling */
#define LINUX_SIG_IGN ((__force __sighandler_t)1)       /* ignore signal */
#define LINUX_SIG_ERR ((__force __sighandler_t)-1)      /* error return from signal */

#undef SIG_DFL
#undef SIG_IGN
#undef SIG_ERR

// for linux/arch/arm/include/asm/page.h
#define LINUX_PAGE_SHIFT        12
#define LINUX_PAGE_SIZE        (_AC(1,UL) << LINUX_PAGE_SHIFT)
#define LINUX_PAGE_MASK        (~(LINUX_PAGE_SIZE-1))

#undef PAGE_SIZE
#undef PAGE_SHIFT
#undef PAGE_MASK
