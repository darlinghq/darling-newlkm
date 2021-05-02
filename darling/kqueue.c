#include <duct/duct.h>

#include <duct/compiler/clang/asm-inline.h>
#include <linux/slab.h>
#include <linux/anon_inodes.h>
#include <linux/fs.h>
#define current linux_current
#include <linux/fdtable.h>
#undef current
#include <linux/poll.h>
#if 0 // TODO: EVFILT_SOCK support (we've gotta fix some header collisions)
#include <linux/net.h>
#include <net/sock.h>
#endif
#include <linux/fsnotify_backend.h>
#include <linux/workqueue.h>

#if 0
// unconditionally print messages; useful for debugging kqueue by itself (enabling debug output for the entire LKM produces *lots* of output)
#define dkqueue_log(x, ...) printk("\0015<%d> dkqueue: " x "\n", linux_current->pid, ##__VA_ARGS__)
#else
#include "debug_print.h"
#define dkqueue_log(x, ...) debug_msg("dkqueue: " x "\n", ##__VA_ARGS__)
#endif

// we include it so we can avoid modifiying it unnecesarily to get some of its static functions
#include "../bsd/kern/kern_event.c"

#include <duct/duct_pre_xnu.h>
#include <vm/vm_map.h>
#include <sys/queue.h>
#include <duct/duct_post_xnu.h>

#include "kqueue.h"
#include "task_registry.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0)
#	define darling_fcheck_files fcheck_files
#	define darling_close_fd ksys_close
#else
#	define darling_fcheck_files files_lookup_fd_rcu
#	define darling_close_fd close_fd
#endif


// re-define `fcheck` because we use `linux_current`
#define fcheck(fd) darling_fcheck_files(linux_current->files, fd)

struct dkqueue_pte;
typedef SLIST_HEAD(dkqueue_pte_head, dkqueue_pte) dkqueue_pte_head_t;
typedef SLIST_ENTRY(dkqueue_pte) dkqueue_pte_entry_t;

// pte = poll table entry
typedef struct dkqueue_pte {
	wait_queue_entry_t wqe;
	wait_queue_head_t* wqh;
	dkqueue_pte_entry_t link;
} dkqueue_pte_t;

// ptc = poll table container
typedef struct dkqueue_ptc {
	poll_table pt;
	struct knote* kn;
	dkqueue_pte_head_t pte_head;
	struct task_struct* polling_task;
	struct work_struct activator;

	bool triggered;

	// socket state tracking
	bool connected: 1;
	bool send_shutdown: 1;
	bool receive_shutdown: 1;
} dkqueue_ptc_t;

typedef struct dkqueue_proc_context {
	struct knote* kn;
	int xnu_pid;
	int64_t registration_id;
	int32_t saved_data;
	bool destroy_it;
} dkqueue_proc_context_t;

typedef struct dkqueue_vnode_context {
	struct fsnotify_mark fs_mark;
	struct knote* kn;
	loff_t cached_size; // used to determine if a file was extended or not (for NOTE_EXTEND)
	unsigned int cached_nlink; // used to determine if a file's link count changed (for NOTE_LINK)
} dkqueue_vnode_context_t;

static struct kmem_cache* container_cache = NULL;
static struct kmem_cache* entry_cache = NULL;
static struct kmem_cache* proc_context_cache = NULL;
static struct kmem_cache* dkqueue_list_entry_cache = NULL;
static struct kmem_cache* vnode_context_cache = NULL;

static struct workqueue_struct* file_activation_wq = NULL;

// TODO: fs filter
const struct filterops fs_filtops = {
	.f_attach  = filt_no_attach,
	.f_detach  = filt_no_detach,
	.f_event   = filt_bad_event,
	.f_touch   = filt_bad_touch,
	.f_process = filt_bad_process,
};

// our own functions are prefixed with `dkq` or `dkqueue` (Darling kqueue) to avoid possible conflicts
// with current or future XNU kqueue functions (because we try to defer to XNU's code as much as possible)

static unsigned int dkqueue_poll(struct file* file, poll_table* wait);
static int dkqueue_release(struct inode* inode, struct file* file);

static int dkqueue_file_attach(struct knote* kn, struct kevent_qos_s* kev);
static void dkqueue_file_detach(struct knote* kn);
static int dkqueue_file_event(struct knote* kn, long hint);
static int dkqueue_file_touch(struct knote* kn, struct kevent_qos_s* kev);
static int dkqueue_file_process(struct knote* kn, struct kevent_qos_s* kev);
static int dkqueue_file_peek(struct knote* kn);

static void dkqueue_file_init(void);
static int dkqueue_file_peek_with_poll_result(struct knote* kn, __poll_t poll_result);
static void dkqueue_poll_unsubscribe_all(dkqueue_ptc_t* ptc);
static int dkqueue_perform_default_wakeup(wait_queue_entry_t* wqe, unsigned int mode, int sync, void* context);
static int dkqueue_poll_wakeup_callback(wait_queue_entry_t* wqe, unsigned int mode, int sync, void* context);
static void dkqueue_poll_wait_callback(struct file* file, wait_queue_head_t* wqh, poll_table* pt);

static int dkqueue_proc_attach(struct knote* kn, struct kevent_qos_s* kev);
static void dkqueue_proc_detach(struct knote* kn);
static int dkqueue_proc_event(struct knote* kn, long hint);
static int dkqueue_proc_touch(struct knote* kn, struct kevent_qos_s* kev);
static int dkqueue_proc_process(struct knote* kn, struct kevent_qos_s* kev);

static void dkqueue_proc_init(void);
static void dkqueue_proc_listener(int pid, void* context, darling_proc_event_t event, unsigned long extra);

static void dkqueue_fork_listener(int pid, void* context, darling_proc_event_t event, unsigned long extra);

static int dkqueue_vnode_attach(struct knote* kn, struct kevent_qos_s* kev);
static void dkqueue_vnode_detach(struct knote* kn);
static int dkqueue_vnode_event(struct knote* kn, long hint);
static int dkqueue_vnode_touch(struct knote* kn, struct kevent_qos_s* kev);
static int dkqueue_vnode_process(struct knote* kn, struct kevent_qos_s* kev);
static int dkqueue_vnode_peek(struct knote* kn);

static void dkqueue_vnode_init(void);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,9,0)
static int dkqueue_vnode_fs_handle_event(struct fsnotify_group* group, u32 mask, const void* data, int data_type, struct inode* dir, const struct qstr* file_name, u32 cookie, struct fsnotify_iter_info* iter_info);
#else
static int dkqueue_vnode_fs_handle_event(struct fsnotify_group* group, struct inode* inode, u32 mask, const void* data, int data_type, const struct qstr* file_name, u32 cookie, struct fsnotify_iter_info* iter_info);
#endif
static void dkqueue_vnode_free_group(struct fsnotify_group *group);
static void dkqueue_vnode_fs_free_event(struct fsnotify_event* event);
static void dkqueue_vnode_fs_free_mark(struct fsnotify_mark* mark);
static void dkqueue_vnode_update_wanted_events(dkqueue_vnode_context_t* ctx);
static void vnode_context_lock(dkqueue_vnode_context_t* ctx);
static void vnode_context_unlock(dkqueue_vnode_context_t* ctx);

static const struct file_operations dkqueue_ops = {
	//.read    = dkqueue_read,
	//.write   = dkqueue_write,
	.poll    = dkqueue_poll,
	.release = dkqueue_release,
};

const struct fsnotify_ops dkqueue_vnode_fsops = {
	.handle_event    = dkqueue_vnode_fs_handle_event,
	.free_group_priv = dkqueue_vnode_free_group,
	.free_event      = dkqueue_vnode_fs_free_event,
	.free_mark       = dkqueue_vnode_fs_free_mark,
};

//static ssize_t dkqueue_read(struct file* file, char __user* buf, size_t count, loff_t* ppos) {};

//static ssize_t dkqueue_write(struct file* file, const char __user *buf, size_t count, loff_t* ppos) {};

static unsigned int dkqueue_poll(struct file* file, poll_table* wait) {
	struct kqfile* kqf = (struct kqfile*)file->private_data;
	int result;

	dkqueue_log("Linux wants to poll kqueue %p on file %p", kqf, file);

	// tell Linux to wait for the Linux wait_queue attached to our kqueue.
	// it'll be woken up in `osfmk/kern/waitq.c` when the XNU waitq receives NO_EVENT64
	// (which is what `KQ_EVENT` is defined to)
	poll_wait(file, &kqf->kqf_wqs.wqset_q.linux_wq, wait);

	if (!poll_does_not_wait(wait)) {
		kqlock(kqf);
		// remember to set this; otherwise no one will wake us up when a knote gets activated!
		// we only set it when we begin to wait, though, to mimic XNU's behavior
		kqf->kqf_state |= KQ_SEL;
		kqunlock(kqf);
	}

	// XNU `select` normally provides a value for wq_link_id so that the fileop links its waitq onto the thread's waitq_set
	// (much like Linux `file_operations::poll`), however we *don't* want kqueue to do that because:
	// 1. we grab and wait for the kqueue's waitq directly in the above `poll_wait` call
	// 2. we don't even use the uthread's waitq_set, so it's useless for kqueue to attach its waitq onto it
	result = kqueue_select(kqf, FREAD, NULL);

	dkqueue_log("is kqueue already active? %s", result ? "yes" : "no");

	return (result > 0) ? (POLLIN | POLLRDNORM) : 0;
};

static int dkqueue_release(struct inode* inode, struct file* file) {
	struct kqfile* kqf = (struct kqfile*)file->private_data;
	proc_t proc;
	dkqueue_list_entry_t* curr;
	dkqueue_list_entry_t* temp;

	if (!kqf) {
		// `dkqueue_clear` already cleaned us up
		return 0;
	}

	proc = kqf->kqf_p;

	dkqueue_log("Linux file pointer %p for kqueue %p released", file, kqf);

	proc_fdlock(proc);
	LIST_FOREACH_SAFE(curr, &proc->p_fd->kqueue_list, link, temp) {
		if (curr->kq == kqf) {
			dkqueue_log("releasing kqueue %p", kqf);

			LIST_REMOVE(curr, link);

			if (kqf->kqf_kqueue.dkq_fs_group) {
				dkqueue_log("destroying fsnotify group for kqueue");
				fsnotify_put_group(kqf->kqf_kqueue.dkq_fs_group);
			}

			// XNU always calls `drain` then `close`; let's do likewise
			kqueue_drain(kqf);

			// `kqueue_close` takes `proc_fdlock`, so make sure it's unlocked
			proc_fdunlock(proc);
			kqueue_close(kqf);
			proc_fdlock(proc);

			kmem_cache_free(dkqueue_list_entry_cache, curr);
			break;
		}
	}
	proc_fdunlock(proc);

	return 0;
};

// i don't know why `fget_task` isn't exported in Linux!
// (we shouldn't use `fget`, because that's only for the current process, and we might not be dealing with the current process)

// prevent XNU macros from overriding Linux inline functions
#undef task_lock
#undef task_unlock

// <copied from="linux://5.8.14/fs/file.c" modified="true">
static struct file *__fget_files(struct files_struct *files, unsigned int fd,
				 fmode_t mask, unsigned int refs)
{
	struct file *file;

	rcu_read_lock();
loop:
	file = darling_fcheck_files(files, fd);
	if (file) {
		/* File object ref couldn't be taken.
		 * dup2() atomicity guarantee is the reason
		 * we loop to catch the new file (or NULL pointer)
		 */
		if (file->f_mode & mask)
			file = NULL;
		else if (!get_file_rcu_many(file, refs))
			goto loop;
	}
	rcu_read_unlock();

	return file;
}

#ifdef __DARLING__
static struct file *fget_task_noconflict(struct task_struct *task, unsigned int fd)
#else
struct file *fget_task(struct task_struct *task, unsigned int fd)
#endif
{
	struct file *file = NULL;

	task_lock(task);
	if (task->files)
		file = __fget_files(task->files, fd, 0, 1);
	task_unlock(task);

	return file;
}
// </copied>

int darling_fd_lookup(proc_t proc, int fd, struct file** out_file) {
	struct file* tmp = fget_task_noconflict(((task_t)proc->task)->map->linux_task, fd);
	if (!tmp) {
		return EBADF;
	}
	if (out_file) {
		*out_file = tmp;
	}
	return 0;
};

/**
 * @brief Check if `events` are enabled for the given knote
 * @param kn The knote to check
 * @param events The events to check for
 * @param all_or_nothing If `true`, this function will only return `true` if *all* the events are present
 */
static bool knote_wants_events(struct knote* kn, int events, bool all_or_nothing) {
	return all_or_nothing ? ((kn->kn_sfflags & events) == events) : ((kn->kn_sfflags & events) != 0);
};

//
// file descriptor filter
//

// alright, so, the way we do this is kind of a hack.
// (but not really; it looks like Linux's APIs took into account the possibility of someone wanting to do this)
//
// we essentially mimic what Linux `poll` and `select` do on files.
// we call `file_operation`'s `poll` for the file and hand it our own `poll_table`.
// it's not a regular `poll_table`, though; it's extended to contain a pointer to the knote
// so that our `poll_queue_proc` replacement can connect the Linux waitqueues it gets with the XNU knote

static void dkqueue_file_init(void) {
	container_cache = kmem_cache_create("dkqueue_ptc_t", sizeof(dkqueue_ptc_t), 0, 0, NULL);
	entry_cache = kmem_cache_create("dkqueue_pte_t", sizeof(dkqueue_pte_t), 0, 0, NULL);
	file_activation_wq = create_singlethread_workqueue("dkqueue_activator");
};

static int dkqueue_file_peek_with_poll_result(struct knote* kn, __poll_t poll_result) {
	int result = 0;
	dkqueue_ptc_t* ptc = kn->kn_hook;

	if ((poll_result & POLLHUP) != 0 || (poll_result & POLLRDHUP) != 0 || (poll_result & POLLERR) != 0) {
		// possible bug: are we supposed to set EV_EOF for EVFILT_SOCK?
		result = 1;
		kn->kn_flags |= EV_EOF;
	}

	switch (kn->kn_filter) {
		case EVFILT_READ: {
			if ((poll_result & (POLLIN | POLLRDNORM)) != 0) {
				dkqueue_log("read filter can be activated");
				result = 1;
			}
			if ((poll_result & (POLLPRI | POLLRDBAND)) != 0) {
				kn->kn_flags |= EV_OOBAND;
			}
		} break;

		case EVFILT_WRITE: {
			if ((poll_result & (POLLOUT | POLLWRNORM)) != 0) {
				dkqueue_log("write filter can be activated");
				result = 1;
			}
		} break;

		case EVFILT_EXCEPT: {
			if ((poll_result & POLLPRI) != 0) {
				dkqueue_log("exception filter can be activated");
				result = 1;
			}
		} break;

#if 0
		// TODO: we have the most interesting socket notifications implemented,
		//       but it'd be great to have all of them implemented
		case EVFILT_SOCK: {
			// we don't need to check for failure; we already checked this is a socket in `dkqueue_file_attach`
			struct socket* sock = sock_from_file(kn->kn_fp, &result);
			int updated = 0;

			if (!ptc->connected && (ptc->connected = (sock->state & SS_CONNECTED) != 0)) {
				// we weren't connected, but we are now
				updated |= NOTE_CONNECTED;
			} else if (ptc->connected && (ptc->connected = (sock->state & SS_CONNECTED) == 0)) {
				// we were connected, but we aren't now
				updated |= NOTE_DISCONNECTED;
			}

			if ((updated & (NOTE_CONNECTED | NOTE_DISCONNECTED)) != 0) {
				// if the state changed to either connected or disconnected, the connection info was updated
				updated |= NOTE_CONNINFO_UPDATED;
			}

			// try to get some more specific information
			if (sock->sk) {
				if (!ptc->send_shutdown && (ptc->send_shutdown = (sock->sk->sk_shutdown & SEND_SHUTDOWN) != 0)) {
					// we can no longer send data on this socket
					updated |= NOTE_WRITECLOSED;
				}
				if (!ptc->receive_shutdown && (ptc->receive_shutdown = (sock->sk->sk_shutdown & RCV_SHUTDOWN) != 0)) {
					// we can no longer receive data on this socket
					updated |= NOTE_READCLOSED;
				}
			}

			// only deliver events the user asked for
			updated &= kn->kn_sfflags & EVFILT_SOCK_ALL_MASK;
			if (updated == 0) {
				// if the user didn't want any of our events, don't report ourselves as active
				result = 0;
			} else {
				// otherwise, if the user *did* want our events; set them and report ourselves as active
				kn->kn_fflags |= updated;
				result = 1;
			}
		} break;
#endif
	};

	return result;
};

static void dkqueue_poll_unsubscribe_all(dkqueue_ptc_t* ptc) {
	dkqueue_log("unsubscribing from wait queues on ptc %p", ptc);

	while (!SLIST_EMPTY(&ptc->pte_head)) {
		dkqueue_pte_t* pte = SLIST_FIRST(&ptc->pte_head);

		dkqueue_log("unsubscribing from wait queue %p", pte->wqh);

		// remove our wait queue subscription
		remove_wait_queue(pte->wqh, &pte->wqe);

		// remove it from the list
		SLIST_REMOVE_HEAD(&ptc->pte_head, link);

		// finally, deallocate it
		kmem_cache_free(entry_cache, pte);
	}
};

static int dkqueue_perform_default_wakeup(wait_queue_entry_t* wqe, unsigned int mode, int sync, void* context) {
	dkqueue_ptc_t* ptc = wqe->private;
	struct kqueue* kq = knote_get_kq(ptc->kn);
	DECLARE_WAITQUEUE(dummy_wait, ptc->polling_task);

	// see `__pollwake` in Linux's `fs/select.c`
	smp_wmb();
	ptc->triggered = true;

	return default_wake_function(&dummy_wait, mode, sync, context);
};

static int dkqueue_poll_wakeup_callback(wait_queue_entry_t* wqe, unsigned int mode, int sync, void* context) {
	dkqueue_pte_t* pte = (dkqueue_pte_t*)wqe;
	dkqueue_ptc_t* ptc = wqe->private;
	__poll_t poll_result;

	dkqueue_log("wait queue for file %p woke up", ptc->kn->kn_fp);

	// imitate `poll` here:
	// we already subscribed to all the wait queues the driver told us to subscribe to when we called it in `dkqueue_file_attach`,
	// so now we disable the `poll_wait` callback and just keep the queues we have now
	if (ptc->pt._qproc != NULL) {
		ptc->pt._qproc = NULL;
	} else {
		// again, see `__pollwake` in Linux's `fs/select.c`
		smp_store_mb(ptc->triggered, false);
	}

	// check again to make sure there's really an event for us
	poll_result = vfs_poll(ptc->kn->kn_fp, &ptc->pt);

	if (dkqueue_file_peek_with_poll_result(ptc->kn, poll_result)) {
		dkqueue_log("activating knote %p for file-based filter with file %p", ptc->kn, ptc->kn->kn_fp);

		// queue an activator for the knote
		queue_work(file_activation_wq, &ptc->activator);
	}

	return dkqueue_perform_default_wakeup(wqe, mode, sync, context);
};

// follows along the same lines of what `__pollwait` in Linux (in `fs/select.c`) does
static void dkqueue_poll_wait_callback(struct file* file, wait_queue_head_t* wqh, poll_table* pt) {
	dkqueue_ptc_t* ptc = (dkqueue_ptc_t*)pt;

	dkqueue_log("subscribing to wait queue %p for file %p", wqh, file);

	// allocate a pte
	dkqueue_pte_t* pte = kmem_cache_alloc(entry_cache, GFP_KERNEL);
	memset(pte, 0, sizeof(dkqueue_pte_t));
	SLIST_INSERT_HEAD(&ptc->pte_head, pte, link);

	// intialize the pte
	init_waitqueue_func_entry(&pte->wqe, dkqueue_poll_wakeup_callback);
	pte->wqh = wqh;
	pte->wqe.private = ptc;

	// subscribe to the wait queue we were given
	add_wait_queue(wqh, &pte->wqe);
};

static void dkqueue_file_activate(struct work_struct* work) {
	dkqueue_ptc_t* ptc = container_of(work, dkqueue_ptc_t, activator);
	struct kqueue* kq = knote_get_kq(ptc->kn);

	// tell kqueue we're awake
	kqlock(kq);
	knote_activate(kq, ptc->kn, FILTER_ACTIVE);
	kqunlock(kq);
};

static int dkqueue_file_attach(struct knote* kn, struct kevent_qos_s* kev) {
	int result;
	struct file* file = kn->kn_fp;
	struct kqueue* kq;
	dkqueue_ptc_t* ptc;
	__poll_t poll_result;
#if 0
	struct socket* sock = NULL;
#endif
	bool connected = false;
	bool send_shutdown = false;
	bool receive_shutdown = false;

	dkqueue_log("attaching a file-based filter onto file %p", file);

	kq = knote_get_kq(kn);

#if 0
	// if we're asked to report socket events, we need to know its initial status
	// (polling a socket doesn't tell us what its previous status was)
	// we check up here as soon as we can in case the state changes after the call to `vfs_poll` (or even when allocating the ptc)
	if (kn->kn_filter == EVFILT_SOCK) {
		sock = sock_from_file(file, &result);
		if (!sock) {
			// don't even bother checking `result`; `sock_from_file` only ever returns EBADF
			knote_set_error(kn, EBADF);
			return 0;
		}
		connected = (sock->state & SS_CONNECTED) != 0;

		if (sock->sk) {
			send_shutdown = (sock->sk->sk_shutdown & SEND_SHUTDOWN) != 0;
			receive_shutdown = (sock->sk->sk_shutdown & RCV_SHUTDOWN) != 0;
		}
	}
#else
	// we need to resolve some header conflicts in order to include Linux's socket headers
	if (kn->kn_filter == EVFILT_SOCK) {
		knote_set_error(kn, ENOTSUP);
		return 0;
	}
#endif

	// allocate a ptc
	ptc = kmem_cache_alloc(container_cache, GFP_KERNEL);
	memset(ptc, 0, sizeof(dkqueue_ptc_t));
	kn->kn_hook = ptc;

	// initialize the ptc
	ptc->polling_task = linux_current;
	init_poll_funcptr(&ptc->pt, dkqueue_poll_wait_callback);
	ptc->kn = kn;
	SLIST_INIT(&ptc->pte_head);
	ptc->connected = connected;
	ptc->send_shutdown = send_shutdown;
	ptc->receive_shutdown = receive_shutdown;
	INIT_WORK(&ptc->activator, dkqueue_file_activate);

	if (!file_can_poll(file)) {
		dkqueue_log("file is always active");

		// if the file doesn't have a `poll` operation, that means it's always ready.
		// how nice of kqueue to have something for just this occasion :)
		// kqueue is still going to try `f_peek`ing us, though (but we'll just always tell it we have data for it)
		knote_markstayactive(kn);
		return 1;
	} else {
		// otherwise, the file *can* be polled, so do so
		poll_result = vfs_poll(file, &ptc->pt);
	}

	// check if the filter is already active
	result = dkqueue_file_peek_with_poll_result(kn, poll_result);

	dkqueue_log("file already active? %s", result ? "yes" : "no");

	return result;
};

static void dkqueue_file_detach(struct knote* kn) {
	dkqueue_ptc_t* ptc = kn->kn_hook;

	dkqueue_log("detaching from file %p", kn->kn_fp);

	// we might get called if `dkqueue_file_attach` fails, so we might not have a ptc yet
	if (!ptc) {
		return;
	}

	// unsubscribe from all wait queues and destroy the `pte`s
	dkqueue_poll_unsubscribe_all(ptc);

	// make sure there aren't any more activators running
	cancel_work_sync(&ptc->activator);

	// deallocate the ptc
	kmem_cache_free(container_cache, ptc);
};

static int dkqueue_file_event(struct knote* kn, long hint) {
	// TODO
	return 0;
};

static int dkqueue_file_touch(struct knote* kn, struct kevent_qos_s* kev) {
	kn->kn_sfflags = kev->fflags;
	return 0;
};

static int dkqueue_file_process(struct knote* kn, struct kevent_qos_s* kev) {
	__poll_t poll_result;
	dkqueue_ptc_t* ptc = kn->kn_hook;
	struct file* file = kn->kn_fp;
	int64_t data = 0;

	// make sure we still have an event one last time
	if (file_can_poll(file)) {
		dkqueue_log("making sure file-based filter for %p still has events...", file);

		// see `dkqueue_poll_wakeup_callback`
		if (ptc->pt._qproc != NULL) {
			ptc->pt._qproc = NULL;
		} else {
			smp_store_mb(ptc->triggered, false);
		}

		poll_result = vfs_poll(file, &ptc->pt);

		// no events
		if (!dkqueue_file_peek_with_poll_result(kn, poll_result)) {
			dkqueue_log("file-based filter had events, but not anymore");
			return 0;
		}
	}

	// if we made it here, we do have events available
	switch (kn->kn_filter) {
		case EVFILT_READ: {
			if (vfs_ioctl(file, FIONREAD, &data) < 0) {
				data = 0;
				dkqueue_log("failed to determine available bytes for EVFILT_READ");
			}
			dkqueue_log("found %ld bytes to read", data);
		} break;
		case EVFILT_WRITE: {
			if (vfs_ioctl(file, TIOCOUTQ, &data) < 0) {
				data = 0;
				dkqueue_log("failed to determine available bytes for EVFILT_WRITE");
			}
			dkqueue_log("found %ld bytes to write", data);
		};
	}

	knote_fill_kevent(kn, kev, data);

	return 1;
};

static int dkqueue_file_peek(struct knote* kn) {
	dkqueue_log("file-based filter for %p is being peeked at", kn->kn_fp);

	// this function is only ever called when our file can't poll.
	// so as long as the file exists, we're active.
	return 1;
};

const static struct filterops file_filtops = {
	.f_isfd = true,
	.f_attach = dkqueue_file_attach,
	.f_detach = dkqueue_file_detach,
	.f_event = dkqueue_file_event,
	.f_touch = dkqueue_file_touch,
	.f_process = dkqueue_file_process,
	.f_peek = dkqueue_file_peek,
};

//
// process filter
//

// TODO: NOTE_REAP delivery.
//       the task registry is already set up for this, but we need to actually send notifications for
//       when a process is reaping another (most likely via an LKM hook injected into the various `wait()` system calls in userspace)
// actually, TODO: a couple more NOTEs

static void dkqueue_proc_init(void) {
	proc_context_cache = kmem_cache_create("dkq_proc_context", sizeof(dkqueue_proc_context_t), 0, 0, NULL);
};

static void dkqueue_proc_listener(int pid, void* context, darling_proc_event_t event, unsigned long extra) {
	dkqueue_proc_context_t* ctx = context;
	struct knote* kn = ctx->kn;
	struct kqueue* kq = knote_get_kq(kn);
	uint32_t* saved_data;

	// since we don't actually save a process to `kn_proc`,
	// we use it to save our data (we can't use kn_hook32 because we already assign our context to kn_hook)
	saved_data = (uint32_t*)&kn->kn_proc;
	*saved_data = 0;

	dkqueue_log("received event %d from Darling's process notification system for Linux PID %d with extra=%ld", event, pid, extra);

	switch (event) {
		case DTE_EXITED: {
			kn->kn_fflags |= NOTE_EXIT;
			if ((kn->kn_sfflags & NOTE_EXITSTATUS) != 0) {
				*saved_data = extra & NOTE_PDATAMASK;
			}

			// we won't get any more events; destroy our context
			// note that we don't need to call `darling_proc_notify_deregister` (because won't get called again)
			kn->kn_hook = NULL;
			kmem_cache_free(proc_context_cache, ctx);

			// also, since we won't get any more events, we can go ahead and tell the user that we've reached EOF
			// (we do it even if they didn't ask to know about NOTE_EXIT; this is how XNU does it)
			//
			// BUG: XNU checks that either the event is NOTE_REAP or the event is NOTE_EXIT AND the user hasn't requested NOTE_REAP.
			//      since we don't have NOTE_REAP implemented yet, we always remove it on NOTE_EXIT.
			// TODO: actually implement NOTE_REAP and check if the user wants that before telling them we're done.
			kn->kn_flags |= EV_EOF | EV_ONESHOT;

			dkqueue_log("process is exiting; marking knote as done");
		} break;
		case DTE_FORKED: {
			kn->kn_fflags |= NOTE_FORK;

			// XNU doesn't seem to pass the child PID through to userspace?
			//*saved_data = extra & NOTE_PDATAMASK;
		} break;
		case DTE_REPLACED: {
			kn->kn_fflags |= NOTE_EXEC;
		} break;
	}

	// ignore events the user doesn't want
	kn->kn_fflags &= kn->kn_sfflags;

	if (kn->kn_fflags != 0) {
		dkqueue_log("activating process filter knote %p on PID %ld", kn, kn->kn_id);

		// tell kqueue we've got some events for it
		kqlock(kq);
		knote_activate(kq, kn, FILTER_ACTIVE);
		kqunlock(kq);
	}
};

static int dkqueue_proc_attach(struct knote* kn, struct kevent_qos_s* kev) {
	struct pid* linux_pid = NULL;
	int xnu_pid = 0;
	dkqueue_proc_context_t* ctx = NULL;

	// NOTE: XNU has completely removed support for these notes.
	//       do we want to implement them in Darling for backwards compat?
	if ((kn->kn_sfflags & (NOTE_TRACK | NOTE_TRACKERR | NOTE_CHILD)) != 0) {
		knote_set_error(kn, ENOTSUP);
		goto error_out;
	}

	// TODO: XNU's EVFILT_PROC checks to make sure the process asking to listen to another process
	//       actually has permission to do so (i.e. it's a parent or a debugger)

	kn->kn_flags |= EV_CLEAR;
	kn->kn_sdata = 0;
	kn->kn_fflags = 0;

	rcu_read_lock();
	linux_pid = find_vpid(kn->kn_id);
	if (!linux_pid) {
		rcu_read_unlock();
		knote_set_error(kn, ESRCH);
		dkqueue_log("failed to find XNU task for Darling PID %ld", kn->kn_id);
		goto error_out;
	}
	xnu_pid = pid_nr(linux_pid);
	rcu_read_unlock();

	dkqueue_log("attaching process filter to process with Darling PID %ld and Linux PID %d", kn->kn_id, xnu_pid);

	ctx = kmem_cache_alloc(proc_context_cache, GFP_KERNEL);
	if (!ctx) {
		knote_set_error(kn, ENOMEM);
		goto error_out;
	}
	memset(ctx, 0, sizeof(dkqueue_proc_context_t));

	kn->kn_hook = ctx;

	ctx->kn = kn;
	ctx->xnu_pid = xnu_pid;

	ctx->registration_id = darling_proc_notify_register(xnu_pid, dkqueue_proc_listener, ctx, DTE_ALL_EVENTS);
	if (ctx->registration_id < 0) {
		// `darling_proc_notify_register` currently only fails if it didn't find the task
		knote_set_error(kn, ESRCH);
		dkqueue_log("failed to attach a listener onto process with Darling PID %ld", kn->kn_id);
		goto error_out;
	}

	// we *might* already have events
	return kn->kn_fflags != 0;

error_out:
	if (ctx) {
		if (ctx->registration_id >= 0) {
			darling_proc_notify_deregister(xnu_pid, ctx->registration_id);
		}
		kmem_cache_free(proc_context_cache, ctx);
	}
	return 0;
};

static void dkqueue_proc_detach(struct knote* kn) {
	dkqueue_proc_context_t* ctx = kn->kn_hook;

	// the task we were watching might've died before we detached.
	// in that case, our event listener would already have cleaned up for us.
	if (!ctx) {
		return;
	}

	dkqueue_log("detaching process filter from process with Linux PID %d", ctx->xnu_pid);

	darling_proc_notify_deregister(ctx->xnu_pid, ctx->registration_id);
	kmem_cache_free(proc_context_cache, ctx);
};

static int dkqueue_proc_event(struct knote* kn, long hint) {
	return kn->kn_fflags != 0;
};

static int dkqueue_proc_touch(struct knote* kn, struct kevent_qos_s* kev) {
	kn->kn_sfflags = kev->fflags;

	// XNU explicitly does NOT restrict the current events with the new flags, for backwards compatibility
	//kn->kn_fflags &= kn->kn_sfflags;

	return kn->kn_fflags != 0;
};

static int dkqueue_proc_process(struct knote* kn, struct kevent_qos_s* kev) {
	uint32_t* saved_data = (uint32_t*)&kn->kn_proc;

	if (kn->kn_fflags == 0) {
		// we've got no events ready
		return 0;
	}

	dkqueue_log("filling kevent data from process filter knote %p", kn);

	knote_fill_kevent(kn, kev, *saved_data);
	*saved_data = 0;

	return 1;
};

const static struct filterops proc_filtops = {
	.f_attach = dkqueue_proc_attach,
	.f_detach = dkqueue_proc_detach,
	.f_event = dkqueue_proc_event,
	.f_touch = dkqueue_proc_touch,
	.f_process = dkqueue_proc_process,
};

//
// dkqueue-specific vnode filter
//
// on XNU, EVFILT_VNODE just refers to file_filtops, but for us, it's easier to implement a separate filter
// because we have to listen for events very differently than we do for regular files
//

// for our entire implementation, we'll be referring to Linux's implementation of inotify on top of fsnotify

// for the correlation between fsnotify events and EVFILT_VNODE notes,
// see XNU's `tests/kqueue_file_tests.c` to see the expected triggers of each EVFILT_VNODE note

static void dkqueue_vnode_init(void) {
	vnode_context_cache = kmem_cache_create("dkq_vnode_ctx", sizeof(dkqueue_vnode_context_t), 0, 0, NULL);
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,9,0)
static int dkqueue_vnode_fs_handle_event(struct fsnotify_group *group, u32 mask, const void *data, int data_type, struct inode *dir, const struct qstr* file_name, u32 cookie, struct fsnotify_iter_info* iter_info) {
#else
static int dkqueue_vnode_fs_handle_event(struct fsnotify_group* group, struct inode* inode, u32 mask, const void* data, int data_type, const struct qstr* file_name, u32 cookie, struct fsnotify_iter_info* iter_info) {
#endif
	struct fsnotify_mark* fs_mark = fsnotify_iter_inode_mark(iter_info);
	dkqueue_vnode_context_t* ctx = (dkqueue_vnode_context_t*)fs_mark;
	uint32_t vnode_event_mask = 0;
	struct kqueue* kq = fs_mark->group->private;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,9,0)
	struct inode* inode = fsnotify_data_inode(data, data_type);
	if (!inode) {
		dkqueue_log("received fsnotify event without an inode; ignoring...");
		return 0;
	}
#endif

	dkqueue_log("received fsnotify event for vnode filter");

	if ((mask & FS_DELETE_SELF) != 0) {
		vnode_event_mask |= NOTE_DELETE;
	}
	if ((mask & FS_MOVE_SELF) != 0) {
		vnode_event_mask |= NOTE_RENAME;
	}
	if ((mask & FS_UNMOUNT) != 0) {
		vnode_event_mask |= NOTE_REVOKE;
	}

	if ((mask & FS_CREATE) != 0) {
		// it's not clear from `kqueue_file_tests.c` whether creating a directory in a watched directory
		// produces both NOTE_WRITE and NOTE_LINK, or just NOTE_LINK.
		// for now, we assume it produces both.
		// TODO: test this on a mac and determine the proper behavior.
		vnode_event_mask |= NOTE_WRITE | ((mask & FS_ISDIR) != 0 ? NOTE_LINK : 0);
	}

	if ((mask & FS_MOVE) != 0) {
		// same TODO as with FS_CREATE
		vnode_event_mask |= NOTE_WRITE | ((mask & FS_ISDIR) != 0 ? NOTE_LINK : 0);
	}

	if ((mask & FS_DELETE) != 0) {
		// same TODO as with FS_CREATE
		vnode_event_mask |= NOTE_WRITE | ((mask & FS_ISDIR) != 0 ? NOTE_LINK : 0);
	}

	if ((mask & FS_MODIFY) != 0) {
		vnode_event_mask |= NOTE_WRITE;
		// make sure the file has actually been extended
		if (ctx->cached_size > (ctx->cached_size = i_size_read(inode))) {
			vnode_event_mask |= NOTE_EXTEND;
		}
	}

	if ((mask & FS_ATTRIB) != 0) {
		// BUG: NOTE_ATTRIB should NOT be set if only the inode's link count has changed (that's what NOTE_LINK is for)
		//      ...but how do we determine if that's the *only* thing that changed?
		//      we *could* cache all the other possible attributes and check if they've changed, but there *must* be a better way
		vnode_event_mask |= NOTE_ATTRIB;
		if (ctx->cached_nlink != (ctx->cached_nlink = inode->i_nlink)) {
			vnode_event_mask |= NOTE_LINK;
		}
	}

	vnode_context_lock(ctx);
	ctx->kn->kn_fflags |= vnode_event_mask & ctx->kn->kn_sfflags;
	if (ctx->kn->kn_fflags != 0) {
		dkqueue_log("activating vnode filter knote %p", ctx->kn);

		kqlock(kq);
		knote_activate(kq, ctx->kn, FILTER_ACTIVE);
		kqunlock(kq);
	}
	vnode_context_unlock(ctx);

	return 0;
};

static void dkqueue_vnode_free_group(struct fsnotify_group *group) {
	// remove the group from the kqueue
	// this shouldn't be necessary, however, as the only time the group should be dying is due to `dkqueue_release`
	// we normally don't "get" or "put" the group because we keep it alive as long as the kqueue is alive
	struct kqueue* kq = group->private;
	kq->dkq_fs_group = NULL;
};

static void dkqueue_vnode_fs_free_event(struct fsnotify_event* event) {
	// unused but required by fsnotify
};

static void dkqueue_vnode_fs_free_mark(struct fsnotify_mark* mark) {
	dkqueue_vnode_context_t* ctx = (dkqueue_vnode_context_t*)mark;
	kmem_cache_free(vnode_context_cache, ctx);
};

/**
 * @brief Update the fsnotify mark with the events we want to listen for.
 *
 * Called with the vnode context (`ctx`) locked
 */
static void dkqueue_vnode_update_wanted_events(dkqueue_vnode_context_t* ctx) {
	ctx->fs_mark.mask = 0;

	#define VNODE_TRANSLATE(a, b) if ((ctx->kn->kn_sfflags & (a)) != (a)) { ctx->fs_mark.mask |= (b); }
	VNODE_TRANSLATE(NOTE_DELETE, FS_DELETE_SELF);
	VNODE_TRANSLATE(NOTE_WRITE, FS_MODIFY | ALL_FSNOTIFY_DIRENT_EVENTS);
	VNODE_TRANSLATE(NOTE_EXTEND, FS_MODIFY);
	VNODE_TRANSLATE(NOTE_ATTRIB, FS_ATTRIB);
	VNODE_TRANSLATE(NOTE_LINK, FS_ATTRIB | ALL_FSNOTIFY_DIRENT_EVENTS);
	VNODE_TRANSLATE(NOTE_RENAME, FS_MOVE_SELF);
	VNODE_TRANSLATE(NOTE_REVOKE, FS_UNMOUNT); // not sure how we're supposed to report revoke(2) usage
	VNODE_TRANSLATE(NOTE_NONE, 0); // ???
	VNODE_TRANSLATE(NOTE_FUNLOCK, 0); // not sure how we're supposed to detect this (Darling userspace hook and hope that Linux doesn't touch the file?)
	#undef VNODE_TRANSLATE
};

static void vnode_context_lock(dkqueue_vnode_context_t* ctx) {
	// group->mark_mutex prevents the group from trying to use our mark while we're modifying it
	// fs_mark->lock prevents someone else from modifying our mark while we are
	mutex_lock(&ctx->fs_mark.group->mark_mutex);
	spin_lock(&ctx->fs_mark.lock);
};

static void vnode_context_unlock(dkqueue_vnode_context_t* ctx) {
	spin_unlock(&ctx->fs_mark.lock);
	mutex_unlock(&ctx->fs_mark.group->mark_mutex);
};

static int dkqueue_vnode_attach(struct knote* kn, struct kevent_qos_s* kev) {
	struct kqueue* kq = knote_get_kq(kn);
	struct fsnotify_group* group = NULL;
	dkqueue_vnode_context_t* ctx = NULL;
	struct fsnotify_mark* fs_mark = NULL;
	struct inode* inode = file_inode(kn->kn_fp);

	// use the existing fsnotify group if there is one
	if (kq->dkq_fs_group) {
		dkqueue_log("reusing fsnotify group %p for kqueue %p", kq->dkq_fs_group, kq);
		group = kq->dkq_fs_group;
		//fsnotify_get_group(group);
	} else {
		// otherwise, create it
		dkqueue_log("creating an fsnotify group for kqueue %p", kq);
		kq->dkq_fs_group = group = fsnotify_alloc_group(&dkqueue_vnode_fsops);
		if (IS_ERR(group)) {
			// `fsnotify_alloc_group` only ever fails due to ENOMEM
			knote_set_error(kn, ENOMEM);
			kq->dkq_fs_group = NULL;
			dkqueue_log("failed to allocate fsnotify group");
			goto error_out;
		}
		group->private = kq;
	}

	ctx = kmem_cache_alloc(vnode_context_cache, GFP_KERNEL);
	if (!ctx) {
		knote_set_error(kn, ENOMEM);
		dkqueue_log("failed to allocate fsnotify mark context");
		goto error_out;
	}
	memset(ctx, 0, sizeof(dkqueue_vnode_context_t));

	fsnotify_init_mark(&ctx->fs_mark, group);
	fs_mark = &ctx->fs_mark;

	// attach it onto the node (`true` = allow duplicates).
	// allowing duplicates greatly simplifies mark management but also means we can't use `fsnotify_find_mark` to find our mark.
	fsnotify_add_inode_mark(fs_mark, inode, true);

	ctx->cached_size = i_size_read(inode);
	ctx->cached_nlink = inode->i_nlink;

	// the event listener callback will check our knote for the events we want to listen for and activate us when it gets them,
	// so we don't need to do anything except make our knote accessible to it and update fsnotify about the events our mark wants
	ctx->kn = kn;

	kn->kn_hook = ctx;

	vnode_context_lock(ctx);
	dkqueue_vnode_update_wanted_events(ctx);
	vnode_context_unlock(ctx);

	// as long as our listener exists, we hold a reference to the group and another to the mark

	return 0;

error_out:
	if (fs_mark) {
		// will free `ctx` for us
		fsnotify_put_mark(fs_mark);
	}
	//if (group) {
	//	fsnotify_put_group(group);
	//}
	return 0;
};

static void dkqueue_vnode_detach(struct knote* kn) {
	dkqueue_vnode_context_t* ctx = kn->kn_hook;
	dkqueue_log("detaching vnode filter with mark %p", &ctx->fs_mark);
	fsnotify_put_mark(&ctx->fs_mark);
};

static int dkqueue_vnode_event(struct knote* kn, long hint) {
	// we don't take hints
	return kn->kn_fflags != 0;
};

static int dkqueue_vnode_touch(struct knote* kn, struct kevent_qos_s* kev) {
	dkqueue_vnode_context_t* ctx = kn->kn_hook;

	kn->kn_sfflags = kev->fflags;

	vnode_context_lock(ctx);
	dkqueue_vnode_update_wanted_events(ctx);
	kn->kn_fflags &= kn->kn_sfflags; // ignore queued events that the user is no longer interested in
	vnode_context_unlock(ctx);

	return kn->kn_fflags != 0;
};

static int dkqueue_vnode_process(struct knote* kn, struct kevent_qos_s* kev) {
	// `dkqueue_vnode_fs_handle_event` does all the processing for us
	return kn->kn_fflags != 0;
};

static int dkqueue_vnode_peek(struct knote* kn) {
	return kn->kn_fflags != 0;
};

const static struct filterops dkqueue_vnode_filtops = {
	.f_isfd = true,
	.f_attach = dkqueue_vnode_attach,
	.f_detach = dkqueue_vnode_detach,
	.f_event = dkqueue_vnode_event,
	.f_touch = dkqueue_vnode_touch,
	.f_process = dkqueue_vnode_process,
	.f_peek = dkqueue_vnode_peek,
};

//
// other functions
//

// used to close kqueues on fork
static void dkqueue_fork_listener(int pid, void* context, darling_proc_event_t event, unsigned long extra) {
	task_t task = darling_task_get(pid);
	proc_t parent_proc = get_bsdtask_info(task);
	dkqueue_list_entry_t* curr;

	// we're called in the execution context of the child, so we can just use `ksys_close` to close the child's fds for the parent's kqueues
	proc_fdlock(parent_proc);
	LIST_FOREACH(curr, &parent_proc->p_fd->kqueue_list, link) {
		dkqueue_log("closing kqueue with fd %d on fork", curr->fd);
		proc_fdunlock(parent_proc);
		darling_close_fd(curr->fd);
		proc_fdlock(parent_proc);
	}
	proc_fdunlock(parent_proc);

	task_deallocate(task);
};

static void dkqueue_list_init(void) {
	dkqueue_list_entry_cache = kmem_cache_create("dkq_le_cache", sizeof(dkqueue_list_entry_t), 0, 0, NULL);
};

//
// functions for the rest of the LKM
//

void dkqueue_init() {
	dkqueue_file_init();
	dkqueue_proc_init();
	dkqueue_list_init();
	dkqueue_vnode_init();
};

void dkqueue_clear(struct proc* proc) {
	struct filedesc* fdp = proc->p_fd;
	struct dkqueue_list* list = &fdp->kqueue_list;
	struct file* curr_file;

	while (!LIST_EMPTY(list)) {
		dkqueue_list_entry_t* curr = LIST_FIRST(list);
		LIST_REMOVE(curr, link);

		// nullify the file's private data so dkqueue_release doesn't do anything
		curr->kq->kqf_kqueue.dkq_fp->private_data = NULL;

		kqueue_drain(curr->kq);

		proc_fdunlock(proc);
		kqueue_close(curr->kq);
		proc_fdlock(proc);

		kmem_cache_free(dkqueue_list_entry_cache, curr);
	}
};

int darling_kqueue_create(struct task* task) {
	proc_t proc = PROC_NULL;
	struct kqfile* kq = NULL;
	int fd = -1;
	int result = 0;
	dkqueue_list_entry_t* le = NULL;

	dkqueue_log("creating kqueue on task %p", task);

	proc = (proc_t)get_bsdtask_info(task);
	if (!proc) {
		// we can't create a kqueue for a plain Mach task.
		// this check is kind of useless at the moment, though,
		// since it's currently impossible to create a plain Mach task in Darling
		result = -LINUX_EINVAL;
		dkqueue_log("failed to create kqueue on plain Mach task");
		goto error_out;
	}

	// it says it allocates a kqueue, but it's actually a kqfile
	kq = (struct kqfile*)kqueue_alloc(proc);
	if (!kq) {
		result = -LINUX_ENOMEM;
		dkqueue_log("failed to allocate kqueue");
		goto error_out;
	}

	le = kmem_cache_alloc(dkqueue_list_entry_cache, GFP_KERNEL);
	if (!le) {
		result = -LINUX_ENOMEM;
		dkqueue_log("failed to allocate list entry for kqueue list");
		goto error_out;
	}
	memset(le, 0, sizeof(dkqueue_list_entry_t));
	le->kq = kq;

	fd = anon_inode_getfd("kqueue", &dkqueue_ops, kq, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		result = fd;
		dkqueue_log("failed to get anon fd for kqueue");
		goto error_out;
	}
	le->fd = fd;

	// we use `fcheck` because we DON'T want to increment the file's refcount
	// this pointer will only be used as long as the file is open anyways,
	// since the kqueue is alive as long as the file is
	rcu_read_lock();
	kq->kqf_kqueue.dkq_fp = fcheck(fd);
	rcu_read_unlock();

	// attach it onto the list of open kqueues
	proc_fdlock(proc);
	LIST_INSERT_HEAD(&proc->p_fd->kqueue_list, le, link);
	proc_fdunlock(proc);

	dkqueue_log("created kqueue %p on fd %d", kq, fd);

	proc_lock(proc);
	if (proc->kqueue_fork_listener_id < 0) {
		dkqueue_log("setting up fork listener for process");

		proc->kqueue_fork_listener_id = darling_proc_notify_register(task->map->linux_task->tgid, dkqueue_fork_listener, NULL, DTE_FORKED);
		if (proc->kqueue_fork_listener_id < 0) {
			proc_unlock(proc);
			result = -LINUX_ESRCH; // `darling_proc_notify_register` only fails with ESRCH
			dkqueue_log("darling_kqueue_create: failed to set up fork listener (with status %ld)", proc->kqueue_fork_listener_id);
			goto error_out;
		}
	}
	proc_unlock(proc);

	return fd;

error_out:
	if (fd >= 0) {
		darling_close_fd(fd);
	} else {
		// we only cleanup the rest ourselves if the fd still hasn't been created.
		// otherwise (if it *has* been created), Linux will call `dkqueue_release` on the file
		// and that will cleanup the kqueue and the list entry for us
		if (kq) {
			kqueue_dealloc(&kq->kqf_kqueue);
		}
		if (le) {
			kmem_cache_free(dkqueue_list_entry_cache, le);
		}
	}
	return result;
};

int darling_closing_descriptor(struct task* task, int fd) {
	struct knote* kn;
	proc_t proc = get_bsdtask_info(task);

	proc_fdlock(proc);

	rcu_read_lock();
	if (!fcheck(fd)) {
		rcu_read_unlock();
		goto out;
	}
	rcu_read_unlock();

	if (fd < proc->p_fd->fd_knlistsize) {
		dkqueue_log("fd %d is closing; closing knote for it...", fd);
		knote_fdclose(proc, fd);
	}

out:
	proc_fdunlock(proc);
	return 0;
};
