#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>
#include <kern/kalloc.h>
#include <string.h>
#include <mach/vm_statistics.h>
#include <duct/duct_post_xnu.h>

// <copied from="xnu://6153.61.1/libkern/c++/OSRuntime.cpp" modified="true">

/*********************************************************************
*********************************************************************/
void *
kern_os_malloc(size_t size)
{
	void *mem;
	if (size == 0) {
		return NULL;
	}

	mem = kallocp_tag_bt((vm_size_t *)&size, VM_KERN_MEMORY_LIBKERN);
	if (!mem) {
		return NULL;
	}

#ifndef __DARLING__
#if OSALLOCDEBUG
	OSAddAtomic(size, &debug_iomalloc_size);
#endif
#endif

	bzero(mem, size);

	return mem;
}

/*********************************************************************
*********************************************************************/
void
kern_os_free(void * addr)
{
#ifndef __DARLING__
	size_t size;
	size = kalloc_size(addr);
#if OSALLOCDEBUG
	OSAddAtomic(-size, &debug_iomalloc_size);
#endif
#endif

	kfree_addr(addr);
}

// </copied>
