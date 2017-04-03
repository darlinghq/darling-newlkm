

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <zone_debug.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/mig_errors.h>
#include <mach/port.h>
#include <mach/vm_param.h>
#include <mach/notify.h>
//#include <mach/mach_host_server.h>
#include <mach/mach_types.h>

#include <machine/machparam.h>		/* spl definitions */

#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>

#include <kern/clock.h>
#include <kern/spl.h>
#include <kern/counters.h>
#include <kern/queue.h>
#include <kern/zalloc.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>

#include <kern/assert.h>

#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>

#include <device/device_types.h>
#include <device/device_port.h>
#include <device/device_server.h>

#include <machine/machparam.h>


// void iokit_add_reference( io_object_t obj )
// {
//         kprintf("not implemented: iokit_add_reference()\n");
// }

// void
// iokit_remove_reference( io_object_t obj )
// {
//         kprintf("not implemented: iokit_remove_reference()\n");
// }

// ipc_port_t iokit_port_for_object( io_object_t obj,
// 			ipc_kobject_type_t type )
// {
//         kprintf("not implemented: iokit_port_for_object()\n");
//         return 0;
// }

// kern_return_t iokit_client_died( io_object_t obj,
//                         ipc_port_t port, ipc_kobject_type_t type, mach_port_mscount_t * mscount )
// {
//         kprintf("not implemented: iokit_client_died()\n");
//         return 0;
// }
