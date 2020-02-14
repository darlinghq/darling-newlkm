/* 
 * duct/osfmk/dummy-kern.c
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <kern/locks.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>
#include <kern/clock.h>
#include <kern/zalloc.h>
#include <kern/thread_call.h>
#include <kern/cpu_data.h>
#include <kern/simple_lock.h>
#include <kern/kalloc.h>
#include <kern/task.h>
#include <kern/waitq.h>
#include <kern/counters.h>
#include <mach_debug/vm_info.h>
#include <mach_debug/page_info.h>

//#undef thread_block
#undef thread_block_parameter
//#undef thread_wakeup_prim
#undef assert_wait_deadline
// osfmk/kern/sync_lock.c

#if 0
// osfmk/kern/sched_prim.c
wait_result_t thread_block (thread_continue_t continuation)
{
        kprintf("not implemented: thread_block()\n");
        return 0;
}
#endif

wait_result_t
thread_block_parameter(
    thread_continue_t       continuation,
    void        * parameter)
{
        kprintf("not implemented: thread_block_parameter()\n");
        return 0;
}

kern_return_t thread_wakeup_prim( event_t event,
                                  boolean_t one_thread,
                                  wait_result_t result )
{
        kprintf("not implemented: thread_wakeup_prim()\n");
        return 0;
}

wait_result_t
assert_wait_deadline(
        event_t                         event,
        wait_interrupt_t        interruptible,
        uint64_t                        deadline)
{
        kprintf("not implemented: assert_wait_deadline()\n");
        return 0;
}






// osfmk/kern/clock.c

void
clock_interval_to_deadline (
        uint32_t interval,
        uint32_t scale_factor,
        uint64_t *result)
{
        kprintf("not implemented: clock_interval_to_deadline()\n");
}





// osfmk/i386/cpu_data.h, line 324

int
get_cpu_number(void)
{
        kprintf("not implemented: get_cpu_number()\n");
        return 0;
}

void
_disable_preemption(void)
{
        kprintf("not implemented: _disable_preemption()\n");
}

void
_enable_preemption(void)
{
        kprintf("not implemented: _enable_preemption()\n");
}

// osfmk/kern/simple_lock.h, line 97



// osfmk/kern/kalloc.c

vm_map_t  kalloc_map;
vm_size_t kalloc_max_prerounded;




// osfmk/x86_64/loose_ends.c
// osfmk/i386/loose_ends.c

void
machine_callstack(natural_t *buf,
                  vm_size_t callstack_max)
{
        kprintf("not implemented: machine_callstack()\n");
}

// osfmk/kern/task_policy.c

int
proc_pid(void * proc)
{
        kprintf("not implemented: proc_pid()\n");
        return 0;
}

// osfmk/kern/sync_sema.c

// void
// semaphore_init(void)
// {
//     ;
// }

// osfmk/kern/syscall_subr.c

void
thread_poll_yield(thread_t self)
{
        // kprintf("not implemented: thread_poll_yield()\n");
}

// osfmk/i386/trap.c

#if 0
void
thread_syscall_return(kern_return_t ret)
{
        kprintf("not implemented: thread_syscall_return()\n");
}
#endif

// osfmk/kern/startup.c

vm_offset_t vm_kernel_addrperm;

// osfmk/i386/i386_vm_init.c

vm_offset_t vm_kernel_base;
vm_offset_t vm_kernel_slide;
vm_offset_t vm_kernel_top;

// osfmk/kern/wait_queue.c



// boolean_t
// wait_queue_member(
//         wait_queue_t wq,
//         wait_queue_set_t wq_set)
// {
//         kprintf("not implemented: wait_queue_member()\n");
//         return 0;
// }

#if 0
kern_return_t
wait_queue_set_unlink_all_nofree(
        wait_queue_set_t wq_set,
        queue_t         links)
{
        kprintf("not implemented: wait_queue_set_unlink_all_nofree()\n");
        return 0;
}
#endif

#if 0
kern_return_t
wait_queue_unlink_all_nofree(
        wait_queue_t wq,
        queue_t links)
{
        kprintf("not implemented: wait_queue_unlink_all_nofree()\n");
        return 0;
}
#endif

#if 0
void
mach_notify_dead_name(
        ipc_port_t    port,
        mach_port_name_t  name)
{
        kprintf("not implemented: mach_notify_dead_name()\n");
}

void
mach_notify_no_senders(
        ipc_port_t    port,
        mach_port_name_t  name)
{
        kprintf("not implemented: mach_notify_no_senders()\n");
}

void
mach_notify_port_deleted(
        ipc_port_t    port,
        mach_port_name_t  name)
{
        kprintf("not implemented: mach_notify_port_deleted()\n");
}

void
mach_notify_port_destroyed(
        ipc_port_t  port,
        ipc_port_t  right)
{
        kprintf("not implemented: mach_notify_port_destroyed()\n");
}

void
mach_notify_send_once(
        ipc_port_t  port)
{
        kprintf("not implemented: mach_notify_send_once()\n");
}

void
mach_notify_send_possible(
  ipc_port_t    port,
  mach_port_name_t  name)
{
        kprintf("not implemented: mach_notify_send_possible()\n");
}
#endif

// osfmk/kern/counters.c

mach_counter_t c_ipc_mqueue_receive_block_kernel = 0;
mach_counter_t c_ipc_mqueue_receive_block_user   = 0;
mach_counter_t c_ipc_mqueue_send_block           = 0;

// osfmk/kern/locks.h

// boolean_t
// lck_mtx_try_lock(lck_mtx_t *lck)
// {
//         kprintf("not implemented: lck_mtx_try_lock()\n");
//         return 0;
// }



// osfmk/kern/timer_call.c

boolean_t
timer_call_enter(
        timer_call_t            call,
        uint64_t                deadline,
        uint32_t                flags)
{
        kprintf("not implemented: timer_call_enter()\n");
        return 0;
}


// osfmk/kern/clock.c

/*
 *      clock_get_calendar_nanotime:
 *
 *      Returns the current calendar value,
 *      nanoseconds as the fraction.
 *
 *      Since we do not have an interface to
 *      set the calendar with resolution greater
 *      than a microsecond, we honor that here.
 */
void
clock_get_calendar_nanotime(
        clock_sec_t             *secs,
        clock_nsec_t            *nanosecs)
{
        kprintf("not implemented: clock_get_calendar_nanotime()\n");
}

// osfmk/kern/timer_call.c

#if 0
void
timer_call_setup(
        timer_call_t                    call,
        timer_call_func_t               func,
        timer_call_param_t              param0)
{
        kprintf("not implemented: timer_call_setup()\n");
}
#endif

// osfmk/kern/sched_prim.c
#undef assert_wait
/*
 *      assert_wait:
 *
 *      Assert that the current thread is about to go to
 *      sleep until the specified event occurs.
 */
wait_result_t
assert_wait(
        event_t                         event,
        wait_interrupt_t        interruptible)
{
        kprintf("not implemented: assert_wait()\n");
        return 0;
}

// osfmk/kern/stack.c

/*
* Return info on stack usage for threads in a specific processor set
*/
kern_return_t
processor_set_stack_usage(
        processor_set_t pset,
        unsigned int    *totalp,
        vm_size_t       *spacep,
        vm_size_t       *residentp,
        vm_size_t       *maxusagep,
        vm_offset_t     *maxstackp)
{
        kprintf("not implemented: processor_set_stack_usage()\n");
        return 0;
}

// osfmk/mach/host_priv_server.h

/* Routine kext_request */
kern_return_t
kext_request(
        host_priv_t host_priv,
        uint32_t user_log_flags,
        vm_offset_t request_data,
        mach_msg_type_number_t request_dataCnt,
        vm_offset_t *response_data,
        mach_msg_type_number_t *response_dataCnt,
        vm_offset_t *log_data,
        mach_msg_type_number_t *log_dataCnt,
        kern_return_t *op_result)
{
        kprintf("not implemented: kext_request()\n");
        return 0;
}

// osfmk/mach/vm_map_server.h

/* Routine mach_vm_region_info */
kern_return_t
mach_vm_region_info(
        vm_map_t task,
        vm_address_t address,
        vm_info_region_t *region,
        vm_info_object_array_t *objects,
        mach_msg_type_number_t *objectsCnt)
{
        kprintf("not implemented: mach_vm_region_info()\n");
        return 0;
}

/* Routine mach_vm_region_info_64 */
kern_return_t
mach_vm_region_info_64(
        vm_map_t task,
        vm_address_t address,
        vm_info_region_64_t *region,
        vm_info_object_array_t *objects,
        mach_msg_type_number_t *objectsCnt)
{
        kprintf("not implemented: mach_vm_region_info_64()\n");
        return 0;
}

/* Routine vm_mapped_pages_info */
kern_return_t
vm_mapped_pages_info(
        vm_map_t task,
        page_address_array_t *pages,
        mach_msg_type_number_t *pagesCnt)
{
        kprintf("not implemented: vm_mapped_pages_info()\n");
        return 0;
}

// osfmk/kern/printf.c

void
bsd_log_lock(void)
{
        kprintf("not implemented: bsd_log_lock()\n");
}

void
bsd_log_unlock(void)
{
        kprintf("not implemented: bsd_log_unlock()\n");
}

int disableConsoleOutput = 0;

// osfmk/kern/clock.c



/*
 *  clock_initialize_calendar:
 *
 *  Set the calendar and related clocks
 *  from the platform clock at boot or
 *  wake event.
 *
 *  Also sends host notifications.
 */
void
clock_initialize_calendar(void)
{
        kprintf("not implemented: clock_initialize_calendar()\n");
}

/*
 *  clock_wakeup_calendar:
 *
 *  Interface to power management, used
 *  to initiate the reset of the calendar
 *  on wake from sleep event.
 */
void
clock_wakeup_calendar(void)
{
        kprintf("not implemented: clock_wakeup_calendar()\n");
}

void
delay_for_interval(
    uint32_t        interval,
    uint32_t        scale_factor)
{
        kprintf("not implemented: delay_for_interval()\n");
}

void
cons_putc_locked(
    char c)
{
        kprintf("not implemented: cons_putc_locked()\n");
}

// osfmk/kern/debug.c

unsigned int    debug_mode = 0;

// osfmk/kern/bsd_kern.c

// osfmk/kern/kext_alloc.c

kern_return_t
kext_alloc(vm_offset_t *_addr, vm_size_t size, boolean_t fixed)
{
        kprintf("not implemented: kext_alloc()\n");
        return 0;
}

void
kext_free(vm_offset_t addr, vm_size_t size)
{
        kprintf("not implemented: kext_free()\n");
}

// osfmk/device/device_init.c

ipc_port_t  master_device_port = 0;

// osfmk/arm/pmap.c

/**
 * pmap_clear_noencrypt
 */
void pmap_clear_noencrypt(ppnum_t pn)
{
        kprintf("not implemented: pmap_clear_noencrypt()\n");
}

/**
 * pmap_set_noencrypt
 */
void pmap_set_noencrypt(ppnum_t pn)
{
        kprintf("not implemented: pmap_set_noencrypt()\n");
}
