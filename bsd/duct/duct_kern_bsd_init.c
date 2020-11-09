#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <sys/event.h>
#include <kern/locks.h>
#include <duct/duct_post_xnu.h>

#include <darling/kqueue.h>
#include <darling/debug_print.h>

extern void kmeminit(void);

// XNU allocates these with zalloc/kalloc
// we can just define them as variables
static lck_grp_attr_t the_real_proc_lck_grp_attr;
static lck_grp_t the_real_proc_mlock_grp;
static lck_attr_t the_real_proc_lck_attr;
static lck_mtx_t the_real_proc_klist_mlock;

lck_grp_attr_t* proc_lck_grp_attr = &the_real_proc_lck_grp_attr;
lck_grp_t* proc_mlock_grp = &the_real_proc_mlock_grp;
lck_attr_t* proc_lck_attr = &the_real_proc_lck_attr;
lck_mtx_t* proc_klist_mlock = &the_real_proc_klist_mlock;

#define bsd_init_kprintf(x, ...) debug_msg("bsd_init: " x, ## __VA_ARGS__)

void bsd_init(void) {
	bsd_init_kprintf("calling kmeminit\n");
	kmeminit();

	lck_grp_attr_setdefault(proc_lck_grp_attr);
	lck_grp_init(proc_mlock_grp, "proc-mlock", proc_lck_grp_attr);

	lck_attr_setdefault(proc_lck_attr);
	lck_mtx_init(proc_klist_mlock, proc_mlock_grp, proc_lck_attr);

	bsd_init_kprintf("calling knote_init\n");
	knote_init();

	bsd_init_kprintf("calling dkqueue_init\n");
	dkqueue_init();
};
