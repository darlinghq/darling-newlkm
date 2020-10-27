#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <sys/event.h>
#include <duct/duct_post_xnu.h>

#include <darling/kqueue.h>
#include <darling/debug_print.h>

extern void kmeminit(void);

#define bsd_init_kprintf(x, ...) debug_msg("bsd_init: " x, ## __VA_ARGS__)

void bsd_init(void) {
	bsd_init_kprintf("calling kmeminit\n");
	kmeminit();

	bsd_init_kprintf("calling knote_init\n");
	knote_init();

	bsd_init_kprintf("calling dkqueue_init\n");
	dkqueue_init();
};
