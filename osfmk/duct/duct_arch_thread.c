/**
 * Put architecture-specific thread functions in this file
 */

#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/thread.h>
#include <duct/duct_post_xnu.h>

#include <duct/compiler/clang/asm-inline.h>
#include <asm/processor.h>

// <copied from="xnu://6153.61.1/osfmk/i386/pcb.c" modified="true">

/**
 * Darling comment:
 * we copied this function from the i386 implementation, but it's the same for ARM and ARM64
 * when pointer authentication is disabled (which is the case for Darling)
 * 
 * this function is found in `pcb.c` on i386 and `status.c` on ARM{,64}
 */

kern_return_t
machine_thread_function_pointers_convert_from_user(
	__unused thread_t thread,
	__unused user_addr_t *fptrs,
	__unused uint32_t count)
{
	// No conversion from userspace representation on this platform
	return KERN_SUCCESS;
}

// </copied>

mach_vm_address_t machine_thread_pc(thread_t thread) {
	return KSTK_EIP(thread->linux_task);
};
