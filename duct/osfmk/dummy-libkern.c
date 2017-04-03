/* 
 * duct/dummy-libkern.c
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <libkern/libkern/OSTypes.h>
#include <osfmk/arm/lock.h>
#include <pexpert/pexpert/pexpert.h>



// libkern/libkern/OSAtomic.h

Boolean
OSCompareAndSwapPtr(
void            * oldValue,
void            * newValue,
void * volatile * address)
{
        kprintf("not implemented: OSCompareAndSwapPtr()\n");
        return 0;
}

#define OSCompareAndSwapPtr(a, b, c) \
       (OSCompareAndSwapPtr(a, b, __SAFE_CAST_PTR(void * volatile *,c)))
