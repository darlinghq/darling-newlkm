/*
 * Copyright (c) 2000-2011 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */
/*
 *  File:   kern/zalloc.c
 *  Author: Avadis Tevanian, Jr.
 *
 *  Zone-based memory allocator.  A zone is a collection of fixed size
 *  data blocks for which quick allocation/deallocation is possible.
 */

#include <duct/duct.h>
#include <duct/duct_pre_xnu.h>

#include <zone_debug.h>
#include <zone_alias_addr.h>

#include <mach/mach_types.h>
#include <mach/vm_param.h>
#include <mach/kern_return.h>
#include <mach/mach_host_server.h>
#include <mach/task_server.h>
#include <mach/machine/vm_types.h>
#include <mach_debug/zone_info.h>
#include <mach/vm_map.h>

#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/host.h>
#include <kern/macro_help.h>
#include <kern/sched.h>
#include <kern/locks.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>
#include <kern/thread_call.h>
#include <kern/zalloc.h>
#include <kern/kalloc.h>

#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <pexpert/pexpert.h>

#include <machine/machparam.h>

#include <libkern/OSDebug.h>
#include <libkern/OSAtomic.h>
#include <sys/kdebug.h>

/*
 * Zone Corruption Debugging
 *
 * We perform three methods to detect use of a zone element after it's been freed. These
 * checks are enabled for every N'th element (counted per-zone) by specifying
 * "zp-factor=N" as a boot-arg. To turn this feature off, set "zp-factor=0" or "-no-zp".
 *
 * (1) Range-check the free-list "next" pointer for sanity.
 * (2) Store the pointer in two different words, one at the beginning of the freed element
 *     and one at the end, and compare them against each other when re-using the element,
 *     to detect modifications.
 * (3) Poison the freed memory by overwriting it with 0xdeadbeef, and check it when the
 *     memory is being reused to make sure it is still poisoned.
 *
 * As a result, each element (that is large enough to hold this data inside) must be marked
 * as either "ZP_POISONED" or "ZP_NOT_POISONED" in the first integer within the would-be
 * poisoned segment after the first free-list pointer.
 *
 * Performance slowdown is inversely proportional to the frequency with which you check
 * (as would be expected), with a 4-5% hit around N=1, down to ~0.3% at N=16 and just
 * "noise" at N=32 and higher. You can expect to find a 100% reproducible
 * bug in an average of N tries, with a standard deviation of about N, but you will probably
 * want to set "zp-factor=1" or "-zp" if you are attempting to reproduce a known bug.
 *
 *
 * Zone corruption logging
 *
 * You can also track where corruptions come from by using the boot-arguments:
 * "zlog=<zone name to log> -zc". Search for "Zone corruption logging" later in this
 * document for more implementation and usage information.
 */

#define ZP_POISON       0xdeadbeef
#define ZP_POISONED     0xfeedface
#define ZP_NOT_POISONED 0xbaddecaf

#if CONFIG_EMBEDDED
    #define ZP_DEFAULT_SAMPLING_FACTOR 0
#else /* CONFIG_EMBEDDED */
    #define ZP_DEFAULT_SAMPLING_FACTOR 16
#endif /* CONFIG_EMBEDDED */

uint32_t    free_check_sample_factor = 0;       /* set by zp-factor=N boot arg */
boolean_t   corruption_debug_flag    = FALSE;   /* enabled by "-zc" boot-arg */

/*
 * Zone checking helper macro.
 */
#define is_kernel_data_addr(a)  (!(a) || ((a) >= vm_min_kernel_address && !((a) & 0x3)))

/*
 * Frees the specified element, which is within the specified zone. If this
 * element should be poisoned and its free list checker should be set, both are
 * done here. These checks will only be enabled if the element size is at least
 * large enough to hold two vm_offset_t's and one uint32_t (to enable both types
 * of checks).
 */
// static inline void
// free_to_zone(zone_t zone, void *elem)
// {
//     ;
// }

/*
 * Allocates an element from the specifed zone, storing its address in the
 * return arg. This function will look for corruptions revealed through zone
 * poisoning and free list checks.
 */
// static inline void
// alloc_from_zone(zone_t zone, void **ret)
// {
//     ;
// }


/*
 * Fake zones for things that want to report via zprint but are not actually zones.
 */
struct fake_zone_info {
    const char* name;
    void (*init)(int);
    void (*query)(int *,
             vm_size_t *, vm_size_t *, vm_size_t *, vm_size_t *,
              uint64_t *, int *, int *, int *);
};

static const struct fake_zone_info fake_zones[] = {
    {
        .name = "kernel_stacks",
        .init = stack_fake_zone_init,
        .query = stack_fake_zone_info,
    },
    {
        .name = "page_tables",
        .init = pt_fake_zone_init,
        .query = pt_fake_zone_info,
    },
    {
        .name = "kalloc.large",
        .init = kalloc_fake_zone_init,
        .query = kalloc_fake_zone_info,
    },
};
static const unsigned int num_fake_zones =
    sizeof (fake_zones) / sizeof (fake_zones[0]);

/*
 * Zone info options
 */
boolean_t zinfo_per_task = FALSE;       /* enabled by -zinfop in boot-args */
#define ZINFO_SLOTS 200             /* for now */
#define ZONES_MAX (ZINFO_SLOTS - num_fake_zones - 1)

/*
 * Support for garbage collection of unused zone pages
 *
 * The kernel virtually allocates the "zone map" submap of the kernel
 * map. When an individual zone needs more storage, memory is allocated
 * out of the zone map, and the two-level "zone_page_table" is
 * on-demand expanded so that it has entries for those pages.
 * zone_page_init()/zone_page_alloc() initialize "alloc_count"
 * to the number of zone elements that occupy the zone page (which may
 * be a minimum of 1, including if a zone element spans multiple
 * pages).
 *
 * Asynchronously, the zone_gc() logic attempts to walk zone free
 * lists to see if all the elements on a zone page are free. If
 * "collect_count" (which it increments during the scan) matches
 * "alloc_count", the zone page is a candidate for collection and the
 * physical page is returned to the VM system. During this process, the
 * first word of the zone page is re-used to maintain a linked list of
 * to-be-collected zone pages.
 */
typedef uint32_t zone_page_index_t;
#define ZONE_PAGE_INDEX_INVALID ((zone_page_index_t)0xFFFFFFFFU)

struct zone_page_table_entry {
    volatile    uint16_t    alloc_count;
    volatile    uint16_t    collect_count;
};

#define ZONE_PAGE_USED  0
#define ZONE_PAGE_UNUSED 0xffff

/* Forwards */
void        zone_page_init(
                vm_offset_t addr,
                vm_size_t   size);

void        zone_page_alloc(
                vm_offset_t addr,
                vm_size_t   size);

void        zone_page_free_element(
                zone_page_index_t   *free_page_head,
                zone_page_index_t   *free_page_tail,
                vm_offset_t addr,
                vm_size_t   size);

void        zone_page_collect(
                vm_offset_t addr,
                vm_size_t   size);

boolean_t   zone_page_collectable(
                vm_offset_t addr,
                vm_size_t   size);

void        zone_page_keep(
                vm_offset_t addr,
                vm_size_t   size);

void        zalloc_async(
                thread_call_param_t p0,
                thread_call_param_t p1);

void        zone_display_zprint( void );

vm_map_t    zone_map = VM_MAP_NULL;

zone_t      zone_zone = ZONE_NULL;  /* the zone containing other zones */

zone_t      zinfo_zone = ZONE_NULL; /* zone of per-task zone info */

/*
 *  The VM system gives us an initial chunk of memory.
 *  It has to be big enough to allocate the zone_zone
 *  all the way through the pmap zone.
 */

vm_offset_t zdata;
vm_size_t   zdata_size;

#define zone_wakeup(zone) thread_wakeup((event_t)(zone))
#define zone_sleep(zone)                \
    (void) lck_mtx_sleep(&(zone)->lock, LCK_SLEEP_SPIN, (event_t)(zone), THREAD_UNINT);


#define lock_zone_init(zone)                \
MACRO_BEGIN                     \
    char _name[32];                 \
    (void) snprintf(_name, sizeof (_name), "zone.%s", (zone)->zone_name); \
    lck_grp_attr_setdefault(&(zone)->lock_grp_attr);        \
    lck_grp_init(&(zone)->lock_grp, _name, &(zone)->lock_grp_attr); \
    lck_attr_setdefault(&(zone)->lock_attr);            \
    lck_mtx_init_ext(&(zone)->lock, &(zone)->lock_ext,      \
        &(zone)->lock_grp, &(zone)->lock_attr);         \
MACRO_END

#define lock_try_zone(zone) lck_mtx_try_lock_spin(&zone->lock)

/*
 *  Garbage collection map information
 */
#define ZONE_PAGE_TABLE_FIRST_LEVEL_SIZE (32)
struct zone_page_table_entry * volatile zone_page_table[ZONE_PAGE_TABLE_FIRST_LEVEL_SIZE];
vm_size_t           zone_page_table_used_size;
vm_offset_t         zone_map_min_address;
vm_offset_t         zone_map_max_address;
unsigned int            zone_pages;
unsigned int                   zone_page_table_second_level_size;                      /* power of 2 */
unsigned int                   zone_page_table_second_level_shift_amount;

#define zone_page_table_first_level_slot(x)  ((x) >> zone_page_table_second_level_shift_amount)
#define zone_page_table_second_level_slot(x) ((x) & (zone_page_table_second_level_size - 1))

void   zone_page_table_expand(zone_page_index_t pindex);
struct zone_page_table_entry *zone_page_table_lookup(zone_page_index_t pindex);

/*
 *  Exclude more than one concurrent garbage collection
 */
decl_lck_mtx_data(,     zone_gc_lock);

lck_attr_t      zone_lck_attr;
lck_grp_t       zone_lck_grp;
lck_grp_attr_t  zone_lck_grp_attr;
lck_mtx_ext_t   zone_lck_ext;

#if !ZONE_ALIAS_ADDR
#define from_zone_map(addr, size) \
    ((vm_offset_t)(addr) >= zone_map_min_address && \
     ((vm_offset_t)(addr) + size -1) <  zone_map_max_address)
#else
#define from_zone_map(addr, size) \
    ((vm_offset_t)(zone_virtual_addr((vm_map_address_t)(uintptr_t)addr)) >= zone_map_min_address && \
     ((vm_offset_t)(zone_virtual_addr((vm_map_address_t)(uintptr_t)addr)) + size -1) <  zone_map_max_address)
#endif

/*
 *  Protects first_zone, last_zone, num_zones,
 *  and the next_zone field of zones.
 */
decl_simple_lock_data(, all_zones_lock);
zone_t          first_zone;
zone_t          *last_zone;
unsigned int        num_zones;

boolean_t zone_gc_allowed = TRUE;
boolean_t zone_gc_forced = FALSE;
boolean_t panic_include_zprint = FALSE;
boolean_t zone_gc_allowed_by_time_throttle = TRUE;

/*
 * Zone leak debugging code
 *
 * When enabled, this code keeps a log to track allocations to a particular zone that have not
 * yet been freed.  Examining this log will reveal the source of a zone leak.  The log is allocated
 * only when logging is enabled, so there is no effect on the system when it's turned off.  Logging is
 * off by default.
 *
 * Enable the logging via the boot-args. Add the parameter "zlog=<zone>" to boot-args where <zone>
 * is the name of the zone you wish to log.
 *
 * This code only tracks one zone, so you need to identify which one is leaking first.
 * Generally, you'll know you have a leak when you get a "zalloc retry failed 3" panic from the zone
 * garbage collector.  Note that the zone name printed in the panic message is not necessarily the one
 * containing the leak.  So do a zprint from gdb and locate the zone with the bloated size.  This
 * is most likely the problem zone, so set zlog in boot-args to this zone name, reboot and re-run the test.  The
 * next time it panics with this message, examine the log using the kgmacros zstack, findoldest and countpcs.
 * See the help in the kgmacros for usage info.
 *
 *
 * Zone corruption logging
 *
 * Logging can also be used to help identify the source of a zone corruption.  First, identify the zone
 * that is being corrupted, then add "-zc zlog=<zone name>" to the boot-args.  When -zc is used in conjunction
 * with zlog, it changes the logging style to track both allocations and frees to the zone.  So when the
 * corruption is detected, examining the log will show you the stack traces of the callers who last allocated
 * and freed any particular element in the zone.  Use the findelem kgmacro with the address of the element that's been
 * corrupted to examine its history.  This should lead to the source of the corruption.
 */

static int log_records; /* size of the log, expressed in number of records */

#define MAX_ZONE_NAME   32  /* max length of a zone name we can take from the boot-args */

static char zone_name_to_log[MAX_ZONE_NAME] = "";   /* the zone name we're logging, if any */

/*
 * The number of records in the log is configurable via the zrecs parameter in boot-args.  Set this to
 * the number of records you want in the log.  For example, "zrecs=1000" sets it to 1000 records.  Note
 * that the larger the size of the log, the slower the system will run due to linear searching in the log,
 * but one doesn't generally care about performance when tracking down a leak.  The log is capped at 8000
 * records since going much larger than this tends to make the system unresponsive and unbootable on small
 * memory configurations.  The default value is 4000 records.
 */

#if defined(__LP64__)
#define ZRECORDS_MAX        128000      /* Max records allowed in the log */
#else
#define ZRECORDS_MAX        8000        /* Max records allowed in the log */
#endif
#define ZRECORDS_DEFAULT    4000        /* default records in log if zrecs is not specificed in boot-args */

/*
 * Each record in the log contains a pointer to the zone element it refers to, a "time" number that allows
 * the records to be ordered chronologically, and a small array to hold the pc's from the stack trace.  A
 * record is added to the log each time a zalloc() is done in the zone_of_interest.  For leak debugging,
 * the record is cleared when a zfree() is done.  For corruption debugging, the log tracks both allocs and frees.
 * If the log fills, old records are replaced as if it were a circular buffer.
 */

struct zrecord {
        void        *z_element;     /* the element that was zalloc'ed of zfree'ed */
        uint32_t    z_opcode:1,     /* whether it was a zalloc or zfree */
            z_time:31;      /* time index when operation was done */
        void        *z_pc[MAX_ZTRACE_DEPTH];    /* stack trace of caller */
};

/*
 * Opcodes for the z_opcode field:
 */

#define ZOP_ALLOC   1
#define ZOP_FREE    0

/*
 * The allocation log and all the related variables are protected by the zone lock for the zone_of_interest
 */

static struct zrecord *zrecords;        /* the log itself, dynamically allocated when logging is enabled  */
static int zcurrent  = 0;           /* index of the next slot in the log to use */
static int zrecorded = 0;           /* number of allocations recorded in the log */
static unsigned int ztime = 0;          /* a timestamp of sorts */
static zone_t  zone_of_interest = NULL;     /* the zone being watched; corresponds to zone_name_to_log */

/*
 * Decide if we want to log this zone by doing a string compare between a zone name and the name
 * of the zone to log. Return true if the strings are equal, false otherwise.  Because it's not
 * possible to include spaces in strings passed in via the boot-args, a period in the logname will
 * match a space in the zone name.
 */

// static int
// log_this_zone(const char *zonename, const char *logname)
// {
//     return 0;
// }


/*
 * Test if we want to log this zalloc/zfree event.  We log if this is the zone we're interested in and
 * the buffer for the records has been allocated.
 */

#define DO_LOGGING(z)       (zrecords && (z) == zone_of_interest)

extern boolean_t zlog_ready;

#if CONFIG_ZLEAKS
// #pragma mark -
// #pragma mark Zone Leak Detection

/*
 * The zone leak detector, abbreviated 'zleak', keeps track of a subset of the currently outstanding
 * allocations made by the zone allocator.  Every zleak_sample_factor allocations in each zone, we capture a
 * backtrace.  Every free, we examine the table and determine if the allocation was being tracked,
 * and stop tracking it if it was being tracked.
 *
 * We track the allocations in the zallocations hash table, which stores the address that was returned from
 * the zone allocator.  Each stored entry in the zallocations table points to an entry in the ztraces table, which
 * stores the backtrace associated with that allocation.  This provides uniquing for the relatively large
 * backtraces - we don't store them more than once.
 *
 * Data collection begins when the zone map is 50% full, and only occurs for zones that are taking up
 * a large amount of virtual space.
 */
#define ZLEAK_STATE_ENABLED     0x01    /* Zone leak monitoring should be turned on if zone_map fills up. */
#define ZLEAK_STATE_ACTIVE      0x02    /* We are actively collecting traces. */
#define ZLEAK_STATE_ACTIVATING      0x04    /* Some thread is doing setup; others should move along. */
#define ZLEAK_STATE_FAILED      0x08    /* Attempt to allocate tables failed.  We will not try again. */
uint32_t    zleak_state = 0;        /* State of collection, as above */

boolean_t   panic_include_ztrace    = FALSE;    /* Enable zleak logging on panic */
vm_size_t   zleak_global_tracking_threshold;    /* Size of zone map at which to start collecting data */
vm_size_t   zleak_per_zone_tracking_threshold;  /* Size a zone will have before we will collect data on it */
unsigned int    zleak_sample_factor = 1000;     /* Allocations per sample attempt */

/*
 * Counters for allocation statistics.
 */

/* Times two active records want to occupy the same spot */
unsigned int z_alloc_collisions = 0;
unsigned int z_trace_collisions = 0;

/* Times a new record lands on a spot previously occupied by a freed allocation */
unsigned int z_alloc_overwrites = 0;
unsigned int z_trace_overwrites = 0;

/* Times a new alloc or trace is put into the hash table */
unsigned int z_alloc_recorded   = 0;
unsigned int z_trace_recorded   = 0;

/* Times zleak_log returned false due to not being able to acquire the lock */
unsigned int z_total_conflicts  = 0;


// #pragma mark struct zallocation
/*
 * Structure for keeping track of an allocation
 * An allocation bucket is in use if its element is not NULL
 */
struct zallocation {
    uintptr_t       za_element;     /* the element that was zalloc'ed or zfree'ed, NULL if bucket unused */
    vm_size_t       za_size;            /* how much memory did this allocation take up? */
    uint32_t        za_trace_index; /* index into ztraces for backtrace associated with allocation */
    /* TODO: #if this out */
    uint32_t        za_hit_count;       /* for determining effectiveness of hash function */
};

/* Size must be a power of two for the zhash to be able to just mask off bits instead of mod */
uint32_t zleak_alloc_buckets = CONFIG_ZLEAK_ALLOCATION_MAP_NUM;
uint32_t zleak_trace_buckets = CONFIG_ZLEAK_TRACE_MAP_NUM;

vm_size_t zleak_max_zonemap_size;

/* Hashmaps of allocations and their corresponding traces */
static struct zallocation*  zallocations;
static struct ztrace*       ztraces;

/* not static so that panic can see this, see kern/debug.c */
struct ztrace*              top_ztrace;

/* Lock to protect zallocations, ztraces, and top_ztrace from concurrent modification. */
static lck_spin_t           zleak_lock;
static lck_attr_t           zleak_lock_attr;
static lck_grp_t            zleak_lock_grp;
static lck_grp_attr_t           zleak_lock_grp_attr;

/*
 * Initializes the zone leak monitor.  Called from zone_init()
 */
// static void
// {\n    return 0;\n}\n(vm_size_t max_zonemap_size)
// {
//     ;
// }

#if CONFIG_ZLEAKS

/*
 * Support for kern.zleak.active sysctl - a simplified
 * version of the zleak_state variable.
 */
int
get_zleak_state(void)
{
        kprintf("not implemented: get_zleak_state()\n");
        return 0;
}

#endif


kern_return_t
zleak_activate(void)
{
        kprintf("not implemented: zleak_activate()\n");
        return 0;
}

/*
 * TODO: What about allocations that never get deallocated,
 * especially ones with unique backtraces? Should we wait to record
 * until after boot has completed?
 * (How many persistent zallocs are there?)
 */

/*
 * This function records the allocation in the allocations table,
 * and stores the associated backtrace in the traces table
 * (or just increments the refcount if the trace is already recorded)
 * If the allocation slot is in use, the old allocation is replaced with the new allocation, and
 * the associated trace's refcount is decremented.
 * If the trace slot is in use, it returns.
 * The refcount is incremented by the amount of memory the allocation consumes.
 * The return value indicates whether to try again next time.
 */
// static boolean_t
// zleak_log(uintptr_t* bt,
//           uintptr_t addr,
//           uint32_t depth,
//           vm_size_t allocation_size)
// {
//     return 0;
// }

/*
 * Free the allocation record and release the stacktrace.
 * This should be as fast as possible because it will be called for every free.
 */
// static void
// zleak_free(uintptr_t addr,
//            vm_size_t allocation_size)
// {
//     ;
// }

#endif /* CONFIG_ZLEAKS */

/*  These functions outside of CONFIG_ZLEAKS because they are also used in
 *  mbuf.c for mbuf leak-detection.  This is why they lack the z_ prefix.
 */

/*
 * This function captures a backtrace from the current stack and
 * returns the number of frames captured, limited by max_frames.
 * It's fast because it does no checking to make sure there isn't bad data.
 * Since it's only called from threads that we're going to keep executing,
 * if there's bad data we were going to die eventually.
 * If this function is inlined, it doesn't record the frame of the function it's inside.
 * (because there's no stack frame!)
 */

uint32_t
fastbacktrace(uintptr_t* bt, uint32_t max_frames)
{
        kprintf("not implemented: fastbacktrace()\n");
        return 0;
}

/* "Thomas Wang's 32/64 bit mix functions."  http://www.concentric.net/~Ttwang/tech/inthash.htm */
uintptr_t
hash_mix(uintptr_t x)
{
        kprintf("not implemented: hash_mix()\n");
        return 0;
}

uint32_t
hashbacktrace(uintptr_t* bt, uint32_t depth, uint32_t max_size)
{
        kprintf("not implemented: hashbacktrace()\n");
        return 0;
}

/*
 *  TODO: Determine how well distributed this is
 *      max_size must be a power of 2. i.e 0x10000 because 0x10000-1 is 0x0FFFF which is a great bitmask
 */
uint32_t
hashaddr(uintptr_t pt, uint32_t max_size)
{
        kprintf("not implemented: hashaddr()\n");
        return 0;
}

/* End of all leak-detection code */
// #pragma mark -

/*
 *  zinit initializes a new zone.  The zone data structures themselves
 *  are stored in a zone, which is initially a static structure that
 *  is initialized by zone_init.
 */
// zone_t
// zinit(
//    vm_size_t   size,       /* the size of an element */
//    vm_size_t   max,        /* maximum memory to use */
//    vm_size_t   alloc,      /* allocation size */
//    const char  *name)      /* a name for the zone */
// {
//     return 0;
// }
unsigned    zone_replenish_loops, zone_replenish_wakeups, zone_replenish_wakeups_initiated;

// static void zone_replenish_thread(zone_t);

/* High priority VM privileged thread used to asynchronously refill a designated
 * zone, such as the reserved VM map entry zone.
 */
// static void zone_replenish_thread(zone_t z)
// {
//     ;
// }

void
zone_prio_refill_configure(zone_t z, vm_size_t low_water_mark)
{
        kprintf("not implemented: zone_prio_refill_configure()\n");
}

/*
 *  Cram the given memory into the specified zone.
 */
void
zcram(
    zone_t      zone,
    vm_offset_t         newmem,
    vm_size_t       size)
{
        kprintf("not implemented: zcram()\n");
}


/*
 *  Steal memory for the zone package.  Called from
 *  vm_page_bootstrap().
 */
void
zone_steal_memory(void)
{
        kprintf("not implemented: zone_steal_memory()\n");
}


/*
 * Fill a zone with enough memory to contain at least nelem elements.
 * Memory is obtained with kmem_alloc_kobject from the kernel_map.
 * Return the number of elements actually put into the zone, which may
 * be more than the caller asked for since the memory allocation is
 * rounded up to a full page.
 */
int
zfill(
    zone_t  zone,
    int nelem)
{
        kprintf("not implemented: zfill()\n");
        return 0;
}

/*
 *  Initialize the "zone of zones" which uses fixed memory allocated
 *  earlier in memory initialization.  zone_bootstrap is called
 *  before zone_init.
 */
// void
// zone_bootstrap(void)
// {
//         // kprintf("not implemented: zone_bootstrap()\n");
// }

void
zinfo_task_init(task_t task)
{
        // kprintf("not implemented: zinfo_task_init()\n");
}

void
zinfo_task_free(task_t task)
{
        // kprintf("not implemented: zinfo_task_free()\n");
}

void
zone_init(
    vm_size_t max_zonemap_size)
{
        kprintf("not implemented: zone_init()\n");
}

void
zone_page_table_expand(zone_page_index_t pindex)
{
        kprintf("not implemented: zone_page_table_expand()\n");
}

struct zone_page_table_entry *
zone_page_table_lookup(zone_page_index_t pindex)
{
        kprintf("not implemented: zone_page_table_lookup()\n");
        return 0;
}

extern volatile SInt32 kfree_nop_count;

// #pragma mark -
// #pragma mark zalloc_canblock

/*
 *  zalloc returns an element from the specified zone.
 */
void *
zalloc_canblock(
    register zone_t zone,
    boolean_t canblock)
{
        kprintf("not implemented: zalloc_canblock()\n");
        return 0;
}


// void *
// zalloc(
//        register zone_t zone)
// {
//     return 0;
// }

void *
zalloc_noblock(
           register zone_t zone)
{
        kprintf("not implemented: zalloc_noblock()\n");
        return 0;
}

void
zalloc_async(
    thread_call_param_t          p0,
    __unused thread_call_param_t p1)
{
        kprintf("not implemented: zalloc_async()\n");
}

/*
 *  zget returns an element from the specified zone
 *  and immediately returns nothing if there is nothing there.
 *
 *  This form should be used when you can not block (like when
 *  processing an interrupt).
 *
 *  XXX: It seems like only vm_page_grab_fictitious_common uses this, and its
 *  friend vm_page_more_fictitious can block, so it doesn't seem like
 *  this is used for interrupts any more....
 */
void *
zget(
    register zone_t zone)
{
        kprintf("not implemented: zget()\n");
        return 0;
}

/* Keep this FALSE by default.  Large memory machine run orders of magnitude
   slower in debug mode when true.  Use debugger to enable if needed */
/* static */ boolean_t zone_check = FALSE;

static zone_t zone_last_bogus_zone = ZONE_NULL;
static vm_offset_t zone_last_bogus_elem = 0;

// void
// zfree(
//     register zone_t zone,
//     void        *addr)
// {
//     ;
// }


/*  Change a zone's flags.
 *  This routine must be called immediately after zinit.
 */
// void
// zone_change(
//     zone_t      zone,
//     unsigned int    item,
//     boolean_t   value)
// {
//     ;
// }

/*
 * Return the expected number of free elements in the zone.
 * This calculation will be incorrect if items are zfree'd that
 * were never zalloc'd/zget'd. The correct way to stuff memory
 * into a zone is by zcram.
 */

integer_t
zone_free_count(zone_t zone)
{
        kprintf("not implemented: zone_free_count()\n");
        return 0;
}

/*
 *  Zone garbage collection subroutines
 */

boolean_t
zone_page_collectable(
    vm_offset_t addr,
    vm_size_t   size)
{
        kprintf("not implemented: zone_page_collectable()\n");
        return 0;
}

void
zone_page_keep(
    vm_offset_t addr,
    vm_size_t   size)
{
        kprintf("not implemented: zone_page_keep()\n");
}

void
zone_page_collect(
    vm_offset_t addr,
    vm_size_t   size)
{
        kprintf("not implemented: zone_page_collect()\n");
}

void
zone_page_init(
    vm_offset_t addr,
    vm_size_t   size)
{
        kprintf("not implemented: zone_page_init()\n");
}

void
zone_page_alloc(
    vm_offset_t addr,
    vm_size_t   size)
{
        kprintf("not implemented: zone_page_alloc()\n");
}

void
zone_page_free_element(
    zone_page_index_t   *free_page_head,
    zone_page_index_t   *free_page_tail,
    vm_offset_t addr,
    vm_size_t   size)
{
        kprintf("not implemented: zone_page_free_element()\n");
}


/* This is used for walking through a zone's free element list.
 */
struct zone_free_element {
    struct zone_free_element * next;
};

/*
 * Add a linked list of pages starting at base back into the zone
 * free list. Tail points to the last element on the list.
 */
#define ADD_LIST_TO_ZONE(zone, base, tail)              \
MACRO_BEGIN                             \
    (tail)->next = (void *)((zone)->free_elements);         \
    if ((zone)->elem_size >= (2 * sizeof(vm_offset_t) + sizeof(uint32_t))) {    \
        ((vm_offset_t *)(tail))[((zone)->elem_size/sizeof(vm_offset_t))-1] =    \
            (zone)->free_elements;              \
    }                               \
    (zone)->free_elements = (unsigned long)(base);          \
MACRO_END

/*
 * Add an element to the chain pointed to by prev.
 */
#define ADD_ELEMENT(zone, prev, elem)                   \
MACRO_BEGIN                             \
    (prev)->next = (elem);                      \
    if ((zone)->elem_size >= (2 * sizeof(vm_offset_t) + sizeof(uint32_t))) {    \
        ((vm_offset_t *)(prev))[((zone)->elem_size/sizeof(vm_offset_t))-1] =    \
            (vm_offset_t)(elem);                \
    }                               \
MACRO_END

struct {
    uint32_t    pgs_freed;

    uint32_t    elems_collected,
                elems_freed,
                elems_kept;
} zgc_stats;

/*  Zone garbage collection
 *
 *  zone_gc will walk through all the free elements in all the
 *  zones that are marked collectable looking for reclaimable
 *  pages.  zone_gc is called by consider_zone_gc when the system
 *  begins to run out of memory.
 *
 *  We should ensure that zone_gc never blocks.
 */
void
zone_gc(boolean_t consider_jetsams)
{
        kprintf("not implemented: zone_gc()\n");
}

/*
 *  By default, don't attempt zone GC more frequently
 *  than once / 1 minutes.
 */
void
compute_zone_gc_throttle(void *arg __unused)
{
        kprintf("not implemented: compute_zone_gc_throttle()\n");
}


#if CONFIG_TASK_ZONE_INFO

kern_return_t
task_zone_info(
    task_t          task,
    mach_zone_name_array_t  *namesp,
    mach_msg_type_number_t  *namesCntp,
    task_zone_info_array_t  *infop,
    mach_msg_type_number_t  *infoCntp)
{
        kprintf("not implemented: task_zone_info()\n");
        return 0;
}

#else   /* CONFIG_TASK_ZONE_INFO */

kern_return_t
task_zone_info(
    __unused task_t     task,
    __unused mach_zone_name_array_t *namesp,
    __unused mach_msg_type_number_t *namesCntp,
    __unused task_zone_info_array_t *infop,
    __unused mach_msg_type_number_t *infoCntp)
{
        kprintf("not implemented: task_zone_info()\n");
        return 0;
}

#endif  /* CONFIG_TASK_ZONE_INFO */

kern_return_t
mach_zone_info(
    host_priv_t     host,
    mach_zone_name_array_t  *namesp,
    mach_msg_type_number_t  *namesCntp,
    mach_zone_info_array_t  *infop,
    mach_msg_type_number_t  *infoCntp)
{
        kprintf("not implemented: mach_zone_info()\n");
        return 0;
}

/*
 * host_zone_info - LEGACY user interface for Mach zone information
 *          Should use mach_zone_info() instead!
 */
kern_return_t
host_zone_info(
    host_priv_t     host,
    zone_name_array_t   *namesp,
    mach_msg_type_number_t  *namesCntp,
    zone_info_array_t   *infop,
    mach_msg_type_number_t  *infoCntp)
{
        kprintf("not implemented: host_zone_info()\n");
        return 0;
}

kern_return_t
mach_zone_force_gc(
    host_t host)
{
        kprintf("not implemented: mach_zone_force_gc()\n");
        return 0;
}

extern unsigned int stack_total;
extern unsigned long long stack_allocs;

#if defined(__i386__) || defined (__x86_64__)
extern unsigned int inuse_ptepages_count;
extern long long alloc_ptepages_count;
#endif

void zone_display_zprint()
{
        kprintf("not implemented: zone_display_zprint()\n");
}

#if ZONE_DEBUG

/* should we care about locks here ? */

#define zone_in_use(z)  ( z->count || z->free_elements )

void
zone_debug_enable(
    zone_t      z)
{
        kprintf("not implemented: zone_debug_enable()\n");
}

void
zone_debug_disable(
    zone_t      z)
{
        kprintf("not implemented: zone_debug_disable()\n");
}


#endif  /* ZONE_DEBUG */

kern_return_t mach_zone_info_for_zone(host_priv_t host, mach_zone_name_t name, mach_zone_info_t* infop) {
	kprintf("not implemented: mach_zone_info_for_zone()\n");
	return 0;
};

void zone_require(void* addr, zone_t expected_zone) {
	// do nothing. this is just a validation function anyways
};
