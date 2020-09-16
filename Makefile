BUILD_ROOT := $(src)

ifndef MIGDIR
$(error Darling's kernel module is now built differently. \
	Please run 'make lkm' and 'make lkm_install' inside your CMake build directory)
endif

#
# ***
# general
# ***
#

asflags-y := -D__DARLING__ -D__NO_UNDERSCORES__ \
	-I$(MIGDIR)/osfmk \
	-I$(BUILD_ROOT)/osfmk \
	-I$(BUILD_ROOT)/duct/defines

WARNING_FLAGS := \
	-Wno-unknown-warning-option \
	-Wno-ignored-optimization-argument \
	-Wno-unknown-pragmas \
	-Wno-error=cast-align \
	-Wno-unused-parameter \
	-Wno-missing-prototypes \
	-Wno-unused-variable \
	-Wno-declaration-after-statement \
	-Wno-undef \
	-Wno-maybe-uninitialized \
	-Wno-gnu-variable-sized-type-not-at-end


#-freorder-blocks

FEATURE_FLAGS := \
	-fno-builtin \
	-fno-common \
	-fsigned-bitfields \
	-fno-strict-aliasing \
	-fno-keep-inline-functions \
	-fblocks \
	-mfentry

# NOTE: we should further sort these
# e.g. something like MACH_DEFINES for definitions that enable certain behaviors in the XNU code,
#      and DARLING_DEFINES for our own personal definitions, and maybe LINUX_DEFINES for definitions
#      that affect Linux headers, and then OTHER_DEFINES for everything else
DEFINES := \
	-D__DARLING__ \
	-DDARLING_DEBUG \
	-DPAGE_SIZE_FIXED \
	-DCONFIG_SCHED_TRADITIONAL \
	-DCONFIG_SCHED_TIMESHARE_CORE \
	-DAPPLE \
	-DKERNEL \
	-DKERNEL_PRIVATE \
	-DXNU_KERNEL_PRIVATE \
	-DPRIVATE \
	-D__MACHO__=1 \
	-Dvolatile=__volatile \
	-DNEXT \
	-D__LITTLE_ENDIAN__=1 \
	-D__private_extern__=extern \
	-D_MODE_T \
	-D_NLINK_T \
	-DVM32_SUPPORT=1 \
	-DMACH_KERNEL_PRIVATE \
	-DARCH_PRIVATE \
	-DDRIVER_PRIVATE \
	-D_KERNEL_BUILD \
	-DKERNEL_BUILD \
	-DMACH_KERNEL \
	-DBSD_BUILD \
	-DBSD_KERNEL_PRIVATE \
	-DLP64KERN=1 \
	-DLP64_DEBUG=0 \
	-DTIMEZONE=0 \
	-DPST=0 \
	-DQUOTA \
	-DABSOLUTETIME_SCALAR_TYPE \
	-DCONFIG_LCTX \
	-DMACH \
	-DCONFIG_ZLEAKS \
	-DNO_DIRECT_RPC \
	-DIPFIREWALL_FORWARD \
	-DIPFIREWALL_DEFAULT_TO_ACCEPT \
	-DTRAFFIC_MGT \
	-DRANDOM_IP_ID \
	-DTCP_DROP_SYNFIN \
	-DICMP_BANDLIM \
	-DIFNET_INPUT_SANITY_CHK \
	-DPSYNCH \
	-DSECURE_KERNEL \
	-DOLD_SEMWAIT_SIGNAL \
	-DCONFIG_MBUF_JUMBO \
	-DCONFIG_WORKQUEUE \
	-DCONFIG_HFS_STD \
	-DCONFIG_HFS_TRIM \
	-DCONFIG_TASK_MAX=512 \
	-DCONFIG_IPC_TABLE_ENTRIES_STEPS=256 \
	-DNAMEDSTREAMS \
	-DCONFIG_VOLFS \
	-DCONFIG_IMGSRC_ACCESS \
	-DCONFIG_TRIGGERS \
	-DCONFIG_VFS_FUNNEL \
	-DCONFIG_EXT_RESOLVER \
	-DCONFIG_SEARCHFS \
	-DIPSEC \
	-DIPSEC_ESP \
	-DIPV6FIREWALL_DEFAULT_TO_ACCEPT \
	-Dcrypto \
	-Drandomipid \
	-DCONFIG_KN_HASHSIZE=20 \
	-DCONFIG_VNODES=750 \
	-DCONFIG_VNODE_FREE_MIN=75 \
	-DCONFIG_NC_HASH=1024 \
	-DCONFIG_VFS_NAMES=2048 \
	-DCONFIG_MAX_CLUSTERS=4 \
	-DKAUTH_CRED_PRIMES_COUNT=3 \
	-DKAUTH_CRED_PRIMES="{5, 17, 97}" \
	-DCONFIG_MIN_NBUF=64 \
	-DCONFIG_MIN_NIOBUF=32 \
	-DCONFIG_NMBCLUSTERS="((1024 * 256) / MCLBYTES)" \
	-DCONFIG_TCBHASHSIZE=128 \
	-DCONFIG_ICMP_BANDLIM=50 \
	-DCONFIG_AIO_MAX=10 \
	-DCONFIG_AIO_PROCESS_MAX=4 \
	-DCONFIG_AIO_THREAD_COUNT=2 \
	-DCONFIG_THREAD_MAX=1024 \
	-DCONFIG_MAXVIFS=2 \
	-DCONFIG_MFCTBLSIZ=16 \
	-DCONFIG_MSG_BSIZE=4096 \
	-DCONFIG_ENFORCE_SIGNED_CODE \
	-DCONFIG_MEMORYSTATUS \
	-DCONFIG_JETSAM \
	-DCONFIG_FREEZE \
	-DCONFIG_ZLEAK_ALLOCATION_MAP_NUM=8192 \
	-DCONFIG_ZLEAK_TRACE_MAP_NUM=4096 \
	-DVM_PRESSURE_EVENTS \
	-DCONFIG_KERNEL_0DAY_SYSCALL_HANDLER \
	-DEVENTMETER \
	-DCONFIG_APP_PROFILE=0 \
	-DCC_USING_FENTRY=1 \
	-DIMPORTANCE_INHERITANCE=1 \
	-DMACH_BSD

OTHER_FLAGS := \
	-std=gnu11

ccflags-y := $(WARNING_FLAGS) $(FEATURE_FLAGS) $(DEFINES) $(OTHER_FLAGS)

#
# ***
# darling-mach
# ***
#

#
# object lists
# remember to reference subdirectories by including the subdirectory variables as part of the list
# e.g.
#     OBJS_osfmk = \
#     	$(OBJS_osfmk/cool-subdir) \
#     	some_random_object.o \
#     	this_other_dude.o \
#     	$(OBJS_osfmk/whatever) \
#     	coolness.o
# note that order doesn't matter, although you'll probably want to keep it sorted and have subdirs separate from objects
# remember to create these as lazily-evaluated variables
#

#
# osfmk/
#
OBJS_osfmk = \
	$(OBJS_osfmk/ipc) \
	$(OBJS_osfmk/kern) \
	$(OBJS_osfmk/prng) \
	$(OBJS_osfmk/duct)
OBJS_osfmk/ipc = \
	osfmk/ipc/ipc_entry.o \
	osfmk/ipc/ipc_hash.o \
	osfmk/ipc/ipc_importance.o \
	osfmk/ipc/ipc_init.o \
	osfmk/ipc/ipc_kmsg.o \
	osfmk/ipc/ipc_mqueue.o \
	osfmk/ipc/ipc_notify.o \
	osfmk/ipc/ipc_object.o \
	osfmk/ipc/ipc_port.o \
	osfmk/ipc/ipc_pset.o \
	osfmk/ipc/ipc_right.o \
	osfmk/ipc/ipc_space.o \
	osfmk/ipc/ipc_table.o \
	osfmk/ipc/ipc_voucher.o \
	osfmk/ipc/mach_debug.o \
	osfmk/ipc/mach_kernelrpc.o \
	osfmk/ipc/mach_msg.o \
	osfmk/ipc/mach_port.o \
	osfmk/ipc/mig_log.o
OBJS_osfmk/kern = \
	osfmk/kern/clock_oldops.o \
	osfmk/kern/exception.o \
	osfmk/kern/host.o \
	osfmk/kern/ipc_clock.o \
	osfmk/kern/ipc_host.o \
	osfmk/kern/ipc_kobject.o \
	osfmk/kern/ipc_mig.o \
	osfmk/kern/ipc_misc.o \
	osfmk/kern/ipc_sync.o \
	osfmk/kern/ipc_tt.o \
	osfmk/kern/locks.o \
	osfmk/kern/ltable.o \
	osfmk/kern/mk_timer.o \
	osfmk/kern/mpsc_queue.o \
	osfmk/kern/priority_queue.o \
	osfmk/kern/restartable.o \
	osfmk/kern/sync_sema.o \
	osfmk/kern/turnstile.o \
	osfmk/kern/ux_handler.o \
	osfmk/kern/waitq.o \
	osfmk/kern/work_interval.o
OBJS_osfmk/prng = \
	osfmk/prng/prng_random.o
OBJS_osfmk/duct = \
	osfmk/duct/darling_xnu_init.o \
	osfmk/duct/duct_arch_thread.o \
	osfmk/duct/duct_arm_locks_arm.o \
	osfmk/duct/duct_atomic.o \
	osfmk/duct/duct_ipc_pset.o \
	osfmk/duct/duct_kern_bsd_kern.o \
	osfmk/duct/duct_kern_clock.o \
	osfmk/duct/duct_kern_debug.o \
	osfmk/duct/duct_kern_kalloc.o \
	osfmk/duct/duct_kern_printf.o \
	osfmk/duct/duct_kern_sched_prim.o \
	osfmk/duct/duct_kern_startup.o \
	osfmk/duct/duct_kern_sysctl.o \
	osfmk/duct/duct_kern_task.o \
	osfmk/duct/duct_kern_task_policy.o \
	osfmk/duct/duct_kern_thread_act.o \
	osfmk/duct/duct_kern_thread_call.o \
	osfmk/duct/duct_kern_thread_policy.o \
	osfmk/duct/duct_kern_thread.o \
	osfmk/duct/duct_kern_timer_call.o \
	osfmk/duct/duct_kern_zalloc.o \
	osfmk/duct/duct_libsa.o \
	osfmk/duct/duct_machine_routines.o \
	osfmk/duct/duct_machine_rtclock.o \
	osfmk/duct/duct_pcb.o \
	osfmk/duct/duct_vm_init.o \
	osfmk/duct/duct_vm_kern.o \
	osfmk/duct/duct_vm_map.o \
	osfmk/duct/duct_vm_user.o

#
# bsd/
#
OBJS_bsd = \
	$(OBJS_bsd/kern) \
	$(OBJS_bsd/pthread) \
	$(OBJS_bsd/uxkern) \
	$(OBJS_bsd/duct)
OBJS_bsd/kern = \
	bsd/kern/qsort.o
OBJS_bsd/pthread = \
	bsd/pthread/pthread_priority.o \
	bsd/pthread/pthread_shims.o
OBJS_bsd/uxkern = \
	bsd/uxkern/ux_exception.o
OBJS_bsd/duct = \
	bsd/duct/duct_kern_kern_fork.o \
	bsd/duct/duct_kern_kern_proc.o \
	bsd/duct/duct_kern_kern_sig.o \
	bsd/duct/duct_uxkern_ux_exception.o

#
# duct/
#
OBJS_duct = \
	$(OBJS_duct/osfmk) \
	$(OBJS_duct/bsd)
OBJS_duct/osfmk = \
	duct/osfmk/dummy-arch-thread.o \
	duct/osfmk/dummy-bank-bank.o \
	duct/osfmk/dummy-kern-audit-sessionport.o \
	duct/osfmk/dummy-kern-clock-oldops.o \
	duct/osfmk/dummy-kern-host-notify.o \
	duct/osfmk/dummy-kern-kmod.o \
	duct/osfmk/dummy-kern-locks.o \
	duct/osfmk/dummy-kern-machine.o \
	duct/osfmk/dummy-kern-mk-sp.o \
	duct/osfmk/dummy-kern-priority.o \
	duct/osfmk/dummy-kern-processor.o \
	duct/osfmk/dummy-kern-sched-prim.o \
	duct/osfmk/dummy-kern-sync-lock.o \
	duct/osfmk/dummy-kern-syscall-emulation.o \
	duct/osfmk/dummy-kern-task-policy.o \
	duct/osfmk/dummy-kern-task.o \
	duct/osfmk/dummy-kern-thread-act.o \
	duct/osfmk/dummy-kern-thread-call.o \
	duct/osfmk/dummy-kern-thread-policy.o \
	duct/osfmk/dummy-kern-thread.o \
	duct/osfmk/dummy-kern-zalloc.o \
	duct/osfmk/dummy-kern.o \
	duct/osfmk/dummy-locks.o \
	duct/osfmk/dummy-machine.o \
	duct/osfmk/dummy-misc.o \
	duct/osfmk/dummy-vm-debug.o \
	duct/osfmk/dummy-vm-kern.o \
	duct/osfmk/dummy-vm-map.o \
	duct/osfmk/dummy-vm-memory-object.o \
	duct/osfmk/dummy-vm-resident.o \
	duct/osfmk/dummy-vm-user.o
OBJS_duct/bsd = \
	duct/bsd/dummy-init.o \
	duct/bsd/dummy-kdebug.o

#
# libkern/
#
OBJS_libkern = \
	$(OBJS_libkern/libclosure) \
	$(OBJS_libkern/gen) \
	$(OBJS_libkern/os) \
	$(OBJS_libkern/duct)
OBJS_libkern/libclosure = \
	libkern/libclosure/libclosuredata.o
OBJS_libkern/gen = \
	libkern/gen/OSAtomicOperations.o
OBJS_libkern/os = \
	libkern/os/refcnt.o
OBJS_libkern/duct = \
	libkern/duct/duct_cpp_OSRuntime.o \
	libkern/duct/duct_libclosure_runtime.o

#
# pexpert/
#
OBJS_pexpert = \
	$(OBJS_pexpert/duct)
OBJS_pexpert/duct = \
	pexpert/duct/duct_gen_bootargs.o \
	pexpert/duct/duct_pe_kprintf.o

#
# <migdir>/osfmk/
#
OBJS_$(MIGDIR_REL)/osfmk = \
	$(OBJS_$(MIGDIR_REL)/osfmk/mach) \
	$(OBJS_$(MIGDIR_REL)/osfmk/device) \
	$(OBJS_$(MIGDIR_REL)/osfmk/UserNotification)
OBJS_$(MIGDIR_REL)/osfmk/mach = \
	$(MIGDIR_REL)/osfmk/mach/clock_priv_server.o \
	$(MIGDIR_REL)/osfmk/mach/clock_reply_user.o \
	$(MIGDIR_REL)/osfmk/mach/clock_server.o \
	$(MIGDIR_REL)/osfmk/mach/exc_user.o \
	$(MIGDIR_REL)/osfmk/mach/exc_server.o \
	$(MIGDIR_REL)/osfmk/mach/host_priv_server.o \
	$(MIGDIR_REL)/osfmk/mach/host_security_server.o \
	$(MIGDIR_REL)/osfmk/mach/lock_set_server.o \
	$(MIGDIR_REL)/osfmk/mach/mach_exc_server.o \
	$(MIGDIR_REL)/osfmk/mach/mach_exc_user.o \
	$(MIGDIR_REL)/osfmk/mach/mach_host_server.o \
	$(MIGDIR_REL)/osfmk/mach/mach_port_server.o \
	$(MIGDIR_REL)/osfmk/mach/mach_vm_server.o \
	$(MIGDIR_REL)/osfmk/mach/mach_voucher_attr_control_server.o \
	$(MIGDIR_REL)/osfmk/mach/mach_voucher_server.o \
	$(MIGDIR_REL)/osfmk/mach/memory_entry_server.o \
	$(MIGDIR_REL)/osfmk/mach/notify_user.o \
	$(MIGDIR_REL)/osfmk/mach/processor_server.o \
	$(MIGDIR_REL)/osfmk/mach/processor_set_server.o \
	$(MIGDIR_REL)/osfmk/mach/restartable_server.o \
	$(MIGDIR_REL)/osfmk/mach/task_server.o \
	$(MIGDIR_REL)/osfmk/mach/thread_act_server.o
OBJS_$(MIGDIR_REL)/osfmk/device = \
	$(MIGDIR_REL)/osfmk/device/device_server.o
OBJS_$(MIGDIR_REL)/osfmk/UserNotification = \
	$(MIGDIR_REL)/osfmk/UserNotification/UNDReplyServer.o

#
# darling/
#
OBJS_darling = \
	darling/binfmt.o \
	darling/commpage.o \
	darling/continuation-asm.o \
	darling/continuation.o \
	darling/down_interruptible.o \
	darling/evprocfd.o \
	darling/evpsetfd.o \
	darling/foreign_mm.o \
	darling/host_info.o \
	darling/module.o \
	darling/procs.o \
	darling/psynch_support.o \
	darling/pthread_kext.o \
	darling/pthread_kill.o \
	darling/task_registry.o \
	darling/traps.o

#
# full list of all objects in the darling-mach kernel module
#
DARLING_MACH_ALL_OBJS = \
	$(OBJS_osfmk) \
	$(OBJS_bsd) \
	$(OBJS_duct) \
	$(OBJS_libkern) \
	$(OBJS_pexpert) \
	$(OBJS_$(MIGDIR_REL)/osfmk) \
	$(OBJS_darling)

#
# normal includes
# these come after any parent directory includes for each object
#
DARLING_MACH_NORMAL_INCLUDES := \
	-I$(BUILD_ROOT)/EXTERNAL_HEADERS \
	-I$(BUILD_ROOT)/duct/defines \
	-I$(BUILD_ROOT)/osfmk \
	-I$(MIGDIR)/bsd \
	-I$(BUILD_ROOT)/bsd \
	-I$(BUILD_ROOT)/iokit \
	-I$(BUILD_ROOT)/libkern \
	-I$(BUILD_ROOT)/libsa \
	-I$(BUILD_ROOT)/pexpert \
	-I$(BUILD_ROOT)/security \
	-I$(BUILD_ROOT)/export-headers \
	-I$(BUILD_ROOT)/osfmk/libsa \
	-I$(BUILD_ROOT)/osfmk/mach_debug \
	-I$(BUILD_ROOT)/ \
	-I$(BUILD_ROOT)/darling \
	-I$(MIGDIR)/osfmk \
	-I$(MIGDIR)/../../startup \
	-I$(BUILD_ROOT)/include

#
# special flags for the generated files in the MIG directory
#
CFLAGS_$(MIGDIR_REL) = \
	-include $(BUILD_ROOT)/osfmk/duct/duct.h \
	-include $(BUILD_ROOT)/osfmk/duct/duct_pre_xnu.h
INCLUDE_OVERRIDES_$(MIGDIR_REL) = \
	-I$(BUILD_ROOT)/osfmk
INCLUDE_OVERRIDES_$(MIGDIR_REL)/osfmk/mach = \
	-I$(BUILD_ROOT)/osfmk

#
# other special flags
#
CFLAGS_osfmk = -I$(BUILD_ROOT)/osfmk/kern

#
# stop KBuild from tripping up Clang
#
CFLAGS_darling-mach.mod.o = \
	-Wno-unknown-warning-option

#
# darling-mach-specific flags
#
DARLING_MACH_CFLAGS =

#
# ***
# darling-overlay
# ***
#

#
# full list of all objects in the darling-overlay kernel module
#
DARLING_OVERLAY_ALL_OBJS = \
	overlayfs/copy_up.o \
	overlayfs/dir.o \
	overlayfs/export.o \
	overlayfs/file.o \
	overlayfs/inode.o \
	overlayfs/namei.o \
	overlayfs/readdir.o \
	overlayfs/super.o \
	overlayfs/util.o

#
# normal includes
# these come after any parent directory includes for each object
#
DARLING_OVERLAY_NORMAL_INCLUDES := \
	-I$(BUILD_ROOT)/include

#
# darling-overlay-specific flags
#
DARLING_OVERLAY_CFLAGS =

#
# ***
# KBuild setup
# ***
#

# KERNELVERSION is a dmks variable to specify the right version of the kernel.
# If this is not done like this, then when updating your kernel, you will
# build against the wrong kernel
KERNELVERSION = $(shell uname -r)
$(info Running kernel version is $(KERNELVERSION))
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
  # can't indent with tabs here or else Make will complain

  $(info Invoked by kernel build system, building for $(KERNELRELEASE))

  # some Make wizardry to calculate the includes to each object
  # we do this to ensure that the parent directories of each object come first in the search paths
  # otherwise, for e.g., when compiling an object in `osfmk`, `kern/ast.h` in bsd might be used instead of `kern/ast.h` in osfmk
  # this also contains logic to allow per-directory CFLAGS to be added and cascade down
  # (subdirectory flags are added after parent directory flags, so they can override parent directory flags)
  do_includes = \
    $(if $(1), \
      $(if $(shell basename '$(1)' | sed 's/^.*\.o$$//'), \
        $(INCLUDE_OVERRIDES_$(1)) \
        -I$(BUILD_ROOT)/$(1) \
      ) \
      $(if $(subst $(BUILD_ROOT)/$(MIGDIR_REL),,$(BUILD_ROOT)/$(1)), \
        $(call do_includes,$(shell dirname '$(1)' | sed -e 's/^.$$//')) \
      ) \
      $(if $(shell basename '$(1)' | sed 's/^.*\.o$$//'), \
        $(CFLAGS_$(1)) \
      ) \
    )

  # for the first foreach:
  #   the first eval is for Linux < 5.4
  #   the second eval is for Linux >= 5.4 and is the "correct" one
  # for the second foreach:
  #   note that here we only have to add to the CFLAGS for the full path (the one used on Linux >= 5.4)
  #   the first loop already created lazily-evaluated CFLAGS variables for Linux < 5.4
  #   when those variables are used, they will automatically refer to the correct CFLAGS for the full path
  iterate_objs = \
    $(foreach OBJ,$($(1)_ALL_OBJS), \
      $(eval CFLAGS_$(notdir $(OBJ)) = $$(CFLAGS_$(OBJ))) \
      $(if $(strip $(CFLAGS_$(OBJ))), \
        $(eval CFLAGS_$(OBJ) := $(call do_includes,$(OBJ))) \
      , \
        $(eval CFLAGS_$(OBJ) += $(call do_includes,$(OBJ))) \
      ) \
    ) \
    $(foreach OBJ,$($(1)_ALL_OBJS), \
      $(eval CFLAGS_$(OBJ) += $$($(1)_NORMAL_INCLUDES) $$($(1)_CFLAGS)) \
    )

  # do the CFLAGS magic for all darling-mach objects
  $(call iterate_objs,DARLING_MACH)

  # add MIG flags for the MIG objects
  $(foreach MIG_OBJ,$(MIG_OBJS), \
    $(eval CFLAGS_$(MIG_OBJ) += $$(miggen_cflags)) \
  )

  # same as before for darling-mach, except this one is for darling-overlay
  $(call iterate_objs,DARLING_OVERLAY)

  obj-m := darling-mach.o darling-overlay.o
  darling-mach-objs := $(DARLING_MACH_ALL_OBJS)
  darling-overlay-objs := $(DARLING_OVERLAY_ALL_OBJS)

# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
	KERNELDIR ?= /lib/modules/$(KERNELVERSION)/build
	PWD := $(shell pwd)
default:
	rm -f darling-mach.mod.o
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

all:
	rm -f darling-mach.mod.o
	$(MAKE) -C /lib/modules/$(KERNELVERSION)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(KERNELVERSION)/build M=$(PWD) clean
	find . \( -name '*.o' -or -name '*.ko' \) -delete
	rm -f *.mod.c
	rm -rf modules.order Module.symvers

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

