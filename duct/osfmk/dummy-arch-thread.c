#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/thread.h>
#include <duct/duct_post_xnu.h>

#include <darling/traps.h> // for kprintf

void machine_thread_reset_pc(thread_t thread, mach_vm_address_t pc) {
	kprintf("not implemented: machine_thread_reset_pc()\n");
};
