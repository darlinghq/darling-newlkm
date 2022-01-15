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

#include "duct.h"
#include "duct_pre_xnu.h"
#include "duct_vm_init.h"
#include "duct_vm_map.h"

void duct_vm_mem_bootstrap (void)
{
#if defined (__DARLING__)
#else
    // vm_offset_t    start, end;
    // vm_size_t zsizearg;
    // mach_vm_size_t zsize;
#endif
	/*
	 *	Initializes resident memory structures.
	 *	From here on, all physical memory is accounted for,
	 *	and we use only virtual addresses.
	 */
#define vm_mem_bootstrap_kprintf(x) /* kprintf(x) */

#if defined (__DARLING__)
#else
    // vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling vm_page_bootstrap\n"));
    // vm_page_bootstrap(&start, &end);
    // 
    // /*
    //  *    Initialize other VM packages
    //  */
    // 
    // vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling zone_bootstrap\n"));
    // zone_bootstrap();
    //     
    // vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling vm_object_bootstrap\n"));
    // vm_object_bootstrap();
    // 
    // vm_kernel_ready = TRUE;
#endif

	vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling vm_map_init\n"));
	duct_vm_map_init();

#if defined (__DARLING__)
#else
//     vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling kmem_init\n"));
//     kmem_init(start, end);
//     kmem_ready = TRUE;
//     /*
//      * Eat a random amount of kernel_map to fuzz subsequent heap, zone and
//      * stack addresses. (With a 4K page and 9 bits of randomness, this
//      * eats at most 2M of VA from the map.)
//      */
//     kmapoff_pgcnt = 0;
//     
//     vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling pmap_init\n"));
//     pmap_init();
//     
//     zlog_ready = TRUE;
// 
//     if (PE_parse_boot_argn("zsize", &zsizearg, sizeof (zsizearg)))
//         zsize = zsizearg * 1024ULL * 1024ULL;
//     else {
//         zsize = sane_size >> 2;                /* Get target zone size as 1/4 of physical memory */
//     }
// 
//     if (zsize < ZONE_MAP_MIN)
//         zsize = ZONE_MAP_MIN;    /* Clamp to min */
// #if defined(__LP64__)
//     zsize += zsize >> 1;
// #endif  /* __LP64__ */
//     if (zsize > sane_size >> 1)
//         zsize = sane_size >> 1;    /* Clamp to half of RAM max */
// #if !__LP64__
//     if (zsize > ZONE_MAP_MAX)
//         zsize = ZONE_MAP_MAX;    /* Clamp to 1.5GB max for K32 */
// #endif /* !__LP64__ */
// 
//     vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling kext_alloc_init\n"));
//     kext_alloc_init();
// 
//     vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling zone_init\n"));
//     assert((vm_size_t) zsize == zsize);
//     zone_init((vm_size_t) zsize);    /* Allocate address space for zones */
// 
//     /* The vm_page_zone must be created prior to kalloc_init; that
//      * routine can trigger zalloc()s (for e.g. mutex statistic structure
//      * initialization). The vm_page_zone must exist to saisfy fictitious
//      * page allocations (which are used for guard pages by the guard
//      * mode zone allocator).
//      */
//     vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling vm_page_module_init\n"));
//     vm_page_module_init();
// 
//     vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling kalloc_init\n"));
//     kalloc_init();
// 
//     vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling vm_fault_init\n"));
//     vm_fault_init();
// 
//     vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling memory_manager_default_init\n"));
//     memory_manager_default_init();
// 
//     vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling memory_object_control_bootstrap\n"));
//     memory_object_control_bootstrap();
// 
//     vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: calling device_pager_bootstrap\n"));
//     device_pager_bootstrap();
// 
//     vm_paging_map_init();
#endif

	vm_mem_bootstrap_kprintf(("vm_mem_bootstrap: done\n"));
}
