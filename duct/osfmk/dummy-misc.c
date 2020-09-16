
#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <machine/types.h>
#include <mach/message.h>
#include <device/device_types.h>
#include <kern/clock.h>
#include <kern/ipc_kobject.h>
#include <UserNotification/UNDTypes.h>
#include <mach_debug/zone_info.h>



// osfmk/i386/rtclock.c


// osfmk/UserNotification/KUNCUserNotifications.c

/*
 *      User interface for setting the host UserNotification Daemon port.
 */

kern_return_t
host_set_UNDServer(
        host_priv_t     host_priv,
        UNDServerRef    server)
{
        kprintf("not implemented: host_set_UNDServer()");
        return 0;
}

/*
 *      User interface for retrieving the UserNotification Daemon port.
 */

kern_return_t
host_get_UNDServer(
        host_priv_t     host_priv,
        UNDServerRef    *serverp)
{
        kprintf("not implemented: host_get_UNDServer()");
        return 0;
}

/*
 *      Routine: convert_port_to_UNDReply
 *
 *              MIG helper routine to convert from a mach port to a
 *              UNDReply object.
 *
 *      Assumptions:
 *              Nothing locked.
 */
UNDReplyRef
convert_port_to_UNDReply(
        ipc_port_t port)
{
        kprintf("not implemented: convert_port_to_UNDReply()");
        return 0;
}

/*
 *      Routine: UNDNotificationCreated_rpc
 *
 *              Intermediate routine.  Allows the kernel mechanism
 *              to be informed that the notification request IS
 *              being processed by the user-level daemon, and how
 *              to identify that request.
 */
kern_return_t
UNDNotificationCreated_rpc (
        UNDReplyRef     reply,
        int             userLandNotificationKey)
{
        kprintf("not implemented: UNDNotificationCreated_rpc()");
        return 0;
}


/*
 * UND Mig Callbacks
 */

kern_return_t
UNDAlertCompletedWithResult_rpc (
        UNDReplyRef             reply,
        int                     result,
        xmlData_t               keyRef,         /* raw XML bytes */
#ifdef KERNEL_CF
        mach_msg_type_number_t  keyLen)
#else
        __unused mach_msg_type_number_t keyLen)
#endif
{
        kprintf("not implemented: UNDAlertCompletedWithResult_rpc()");
        return 0;
}

void set_sched_pri(thread_t thread, int priority, set_sched_pri_options_t options)
{
    kprintf("not implemented: set_sched_pri\n");
}

unsigned long pthread_priority_canonicalize(unsigned long priority, boolean_t for_propagation)
{
    kprintf("not implemented: pthread_priority_canonicalize()\n");
    return priority;
}


#if 0

lck_mtx_t iokit_obj_to_port_binding_lock;

// osfmk/i386/locks_i386.c

void
lock_done(
        register lock_t * l)
{
        kprintf("not implemented: lock_done()");
}

void
lock_write(
        register lock_t * l)
{
        kprintf("not implemented: lock_write()");
}

// osfmk/i386/pmap_common.c

unsigned int
pmap_cache_attributes(ppnum_t pn)
{
        kprintf("not implemented: pmap_cache_attributes()");
        return 0;
}

void
pmap_set_cache_attributes(ppnum_t pn, unsigned int cacheattr)
{
        kprintf("not implemented: pmap_set_cache_attributes()");
}

// osfmk/i386/pmap_x86_common.c

void
pmap_enter(
        register pmap_t         pmap,
        vm_map_offset_t         vaddr,
        ppnum_t                 pn,
        vm_prot_t               prot,
        vm_prot_t               fault_type,
        unsigned int            flags,
        boolean_t               wired)
{
        kprintf("not implemented: pmap_enter()");
}

/*
 * pmap_find_phys returns the (4K) physical page number containing a
 * given virtual address in a given pmap.
 * Note that pmap_pte may return a pde if this virtual address is
 * mapped by a large page and this is taken into account in order
 * to return the correct page number in this case.
 */
ppnum_t
pmap_find_phys(pmap_t pmap, addr64_t va)
{
        kprintf("not implemented: pmap_find_phys()");
        return 0;
}

// osfmk/i386/pmap.c

/* Map a (possibly) autogenned block */
void
pmap_map_block(
        pmap_t          pmap,
        addr64_t        va,
        ppnum_t         pa,
        uint32_t        size,
        vm_prot_t       prot,
        int             attr,
        __unused unsigned int   flags)
{
        kprintf("not implemented: pmap_map_block()");
}

void
flush_dcache(__unused vm_offset_t   addr,
         __unused unsigned      count,
         __unused int       phys)
{
        kprintf("not implemented: flush_dcache()");
}

void
invalidate_icache(__unused vm_offset_t  addr,
          __unused unsigned cnt,
          __unused int      phys)
{
        kprintf("not implemented: invalidate_icache()");
}

pmap_t      kernel_pmap;

#endif

// osfmk/i386/rtclock.c

void
nanotime_to_absolutetime(
        clock_sec_t             secs,
        clock_nsec_t            nanosecs,
        uint64_t                *result)
{
        kprintf("not implemented: nanotime_to_absolutetime()");
}

kern_return_t
mach_memory_info(
	host_priv_t		host,
	mach_zone_name_array_t	*namesp,
	mach_msg_type_number_t  *namesCntp,
	mach_zone_info_array_t	*infop,
	mach_msg_type_number_t  *infoCntp,
	mach_memory_info_array_t *memoryInfop,
	mach_msg_type_number_t   *memoryInfoCntp)
{
       kprintf("not implemented: mach_memory_info()");
       // NOTE(@facekapow): i'm adding `return 0` because that's what all the neighboring functions are doing,
       //                   but maybe we (and maybe all the other functions) should be returning `KERN_FAILURE` instead
       return 0;
}

#if 0

/*
* Create an external object.
*/
kern_return_t
default_pager_object_create(
        default_pager_t default_pager,
        vm_size_t       size,
        memory_object_t *mem_objp)
{
        kprintf("not implemented: default_pager_object_create()");
        return 0;
}

// osfmk/default_pager/default_pager.c

kern_return_t
default_pager_info_64(
        memory_object_default_t pager,
        default_pager_info_64_t *infop)
{
        kprintf("not implemented: default_pager_info_64()");
        return 0;
}

kern_return_t
default_pager_info(
        memory_object_default_t pager,
        default_pager_info_t    *infop)
{
        kprintf("not implemented: default_pager_info()");
        return 0;
}

// osfmk/default_pager/dp_backing_store.c

kern_return_t
default_pager_triggers( __unused MACH_PORT_FACE default_pager,
        int             hi_wat,
        int             lo_wat,
        int             flags,
        MACH_PORT_FACE  trigger_port)
{
        kprintf("not implemented: default_pager_triggers()");
        return 0;
}

kern_return_t
default_pager_add_file(
        MACH_PORT_FACE  backing_store,
        vnode_ptr_t     vp,
        int             record_size,
        vm_size_t       size)
{
        kprintf("not implemented: default_pager_add_file()");
        return 0;
}

kern_return_t
default_pager_backing_store_info(
        MACH_PORT_FACE          backing_store,
        backing_store_flavor_t  flavour,
        backing_store_info_t    info,
        mach_msg_type_number_t  *size)
{
        kprintf("not implemented: default_pager_backing_store_info()");
        return 0;
}

kern_return_t
default_pager_backing_store_delete(
        MACH_PORT_FACE backing_store)
{
        kprintf("not implemented: default_pager_backing_store_delete()");
        return 0;
}

kern_return_t
default_pager_backing_store_delete_internal(
        MACH_PORT_FACE backing_store)
{
        kprintf("not implemented: default_pager_backing_store_delete_internal()");
        return 0;
}

kern_return_t
default_pager_backing_store_create(
        memory_object_default_t pager,
        int                     priority,
        int                     clsize,         /* in bytes */
        MACH_PORT_FACE          *backing_store)
{
        kprintf("not implemented: default_pager_backing_store_create()");
        return 0;
}

// osfmk/default_pager/dp_memory_object.c

kern_return_t
default_pager_object_pages(
        default_pager_t         default_pager,
        mach_port_t                     memory_object,
        default_pager_page_array_t      *pagesp,
        mach_msg_type_number_t          *countp)
{
        kprintf("not implemented: default_pager_object_pages()");
        return 0;
}

kern_return_t
default_pager_objects(
        default_pager_t                 default_pager,
        default_pager_object_array_t    *objectsp,
        mach_msg_type_number_t          *ocountp,
        mach_port_array_t               *portsp,
        mach_msg_type_number_t          *pcountp)
{
        kprintf("not implemented: default_pager_objects()");
        return 0;
}


// osfmk/vm/pmap.h

/* LP64todo - switch to vm_map_offset_t when it grows */
void
pmap_remove(    /* Remove mappings. */
        pmap_t          map,
        vm_map_offset_t s,
        vm_map_offset_t e)
{
        kprintf("not implemented: pmap_remove()");
}

// osfmk/vm/vm_shared_region.c
void
post_sys_powersource(int i)
{
        kprintf("not implemented: post_sys_powersource()");
}

// osfmk/vm/vm_fault.c

kern_return_t
vm_fault(
    vm_map_t    map,
    vm_map_offset_t vaddr,
    vm_prot_t   fault_type,
    boolean_t   change_wiring,
    int     interruptible,
    pmap_t      caller_pmap,
    vm_map_offset_t caller_pmap_addr)
{
        kprintf("not implemented: vm_fault()");
        return 0;
}


// osfmk/arm/rtclock.c

// void
// clock_interval_to_absolutetime_interval(uint32_t interval, uint32_t scale_factor, uint64_t* result)
// {
//         kprintf("not implemented: clock_interval_to_absolutetime_interval()");
// }
//
// void
// absolutetime_to_nanoseconds(uint64_t abstime, uint64_t* result)
// {
//         kprintf("not implemented: absolutetime_to_nanoseconds()");
// }


// osfmk/arm/loose_ends.c

/**
 * copypv
 *
 * Copy a memory address across a virtual/physical memory barrier.
 */
kern_return_t
copypv(addr64_t src64, addr64_t snk64, unsigned int size, int which)
{
        kprintf("not implemented: copypv()");
        return 0;
}

// osfmk/i386/loose_ends.c

void
dcache_incoherent_io_flush64(addr64_t pa, unsigned int count)
{
        kprintf("not implemented: dcache_incoherent_io_flush64()");
}

void
dcache_incoherent_io_store64(addr64_t pa, unsigned int count)
{
        kprintf("not implemented: dcache_incoherent_io_store64()");
}

// osfmk/i386/loose_ends.c

void
flush_dcache64(addr64_t addr, unsigned count, int phys)
{
        kprintf("not implemented: flush_dcache64()");
}

#endif

// i'm pretty sure this is used to drive macOS 10.15's new DriverKit extensions
kern_return_t uext_server(ipc_kmsg_t requestkmsg, ipc_kmsg_t* pReply) {
	kprintf("not implemented: uext_server()");
	return KERN_NOT_SUPPORTED;
};
