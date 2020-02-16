BUILD_ROOT := $(src)

ifndef MIGDIR
$(error Darling's kernel module is now built differently. \
	Please run 'make lkm' and 'make lkm_install' inside your CMake build directory)
endif


asflags-y := -D__DARLING__ -D__NO_UNDERSCORES__ \
	-I$(MIGDIR)/osfmk \
	-I$(BUILD_ROOT)/osfmk \
	-I$(BUILD_ROOT)/duct/defines

ccflags-y := -D__DARLING__ -DDARLING_DEBUG \
	-I$(BUILD_ROOT)/EXTERNAL_HEADERS \
	-I$(BUILD_ROOT)/EXTERNAL_HEADERS/bsd \
	-I$(BUILD_ROOT)/duct/defines \
	-DPAGE_SIZE_FIXED \
	-DCONFIG_SCHED_TRADITIONAL \
	-freorder-blocks \
	-fno-builtin \
	-fno-common \
	-fsigned-bitfields \
	-fno-strict-aliasing \
	-fno-keep-inline-functions \
	-Wno-unknown-pragmas \
	-DAPPLE \
	-DKERNEL \
	-DKERNEL_PRIVATE \
	-DXNU_KERNEL_PRIVATE \
	-DPRIVATE \
	-D__MACHO__=1 \
	-Dvolatile=__volatile \
	-DNEXT \
	-Wno-error=cast-align \
	-Wno-unused-parameter \
	-Wno-missing-prototypes \
	-Wno-unused-variable \
	-D__LITTLE_ENDIAN__=1 \
	-Wno-declaration-after-statement \
	-Wno-undef \
	-Wno-maybe-uninitialized \
	-D__private_extern__=extern \
	-D_MODE_T -D_NLINK_T -DVM32_SUPPORT=1 -DMACH_KERNEL_PRIVATE \
	-I$(MIGDIR)/bsd \
	-I$(BUILD_ROOT)/bsd \
	-I$(BUILD_ROOT)/osfmk \
	-I$(BUILD_ROOT)/iokit \
	-I$(BUILD_ROOT)/libkern \
	-I$(BUILD_ROOT)/libsa \
	-I$(BUILD_ROOT)/libsa \
	-I$(BUILD_ROOT)/pexpert \
	-I$(BUILD_ROOT)/security \
	-I$(BUILD_ROOT)/export-headers \
	-I$(BUILD_ROOT)/osfmk/libsa \
	-I$(BUILD_ROOT)/osfmk/mach_debug \
	-I$(BUILD_ROOT)/ \
	-I$(BUILD_ROOT)/darling \
	-I$(MIGDIR)/osfmk \
	-I$(MIGDIR)/../startup \
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
	-std=gnu11

miggen_cflags := -include $(BUILD_ROOT)/osfmk/duct/duct.h -include $(BUILD_ROOT)/osfmk/duct/duct_pre_xnu.h
atomic_cflags := -include $(BUILD_ROOT)/clang_to_gcc_atomic.h

# This takes effect on Linux <5.4
CFLAGS_task_server.o := $(miggen_cflags)
CFLAGS_clock_server.o := $(miggen_cflags)
CFLAGS_lock_set_server.o := $(miggen_cflags)
CFLAGS_clock_priv_server.o := $(miggen_cflags)
CFLAGS_processor_server.o := $(miggen_cflags)
CFLAGS_host_priv_server.o := $(miggen_cflags)
CFLAGS_host_security_server.o := $(miggen_cflags)
CFLAGS_UNDReply_server.o := $(miggen_cflags)
CFLAGS_mach_port_server.o := $(miggen_cflags)
#CFLAGS_default_pager_object_server.o := $(miggen_cflags)
CFLAGS_mach_vm_server.o := $(miggen_cflags)
CFLAGS_mach_host_server.o := $(miggen_cflags)
CFLAGS_thread_act_server.o := $(miggen_cflags)
CFLAGS_processor_set_server.o := $(miggen_cflags)
CFLAGS_vm32_map_server.o := $(miggen_cflags)
CFLAGS_device_server.o := $(miggen_cflags)
CFLAGS_clock_reply_user.o := $(miggen_cflags)
CFLAGS_notify_user.o := $(miggen_cflags)
CFLAGS_mach_voucher_server.o := $(miggen_cflags)
CFLAGS_mach_voucher_attr_control_server.o := $(miggen_cflags)
CFLAGS_OSAtomicOperations.o := $(atomic_cflags)

# This takes effect on Linux 5.4+
CFLAGS_$(MIGDIR_REL)/osfmk/mach/task_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/clock_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/lock_set_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/clock_priv_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/processor_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/host_priv_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/host_security_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/UserNotification/UNDReply_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/mach_port_server.o := $(miggen_cflags)
#CFLAGS_$(MIGDIR_REL)/osfmk/default_pager/default_pager_object_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/mach_vm_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/mach_host_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/mach_voucher_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/mach_voucher_attr_control_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/thread_act_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/processor_set_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/vm32_map_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/device/device_server.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/clock_reply_user.o := $(miggen_cflags)
CFLAGS_$(MIGDIR_REL)/osfmk/mach/notify_user.o := $(miggen_cflags)
CFLAGS_libkern/gen/OSAtomicOperations.o := $(atomic_cflags)

# KERNELVERSION is a dmks variable to specify the right version of the kernel.
# If this is not done like this, then when updating your kernel, you will
# build against the wrong kernel
KERNELVERSION = $(shell uname -r)
$(info Running kernel version is $(KERNELVERSION))
# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
$(info Invoked by kernel build system, building for $(KERNELRELEASE))
	obj-m := darling-mach.o
	darling-mach-objs := osfmk/ipc/ipc_entry.o \
		osfmk/ipc/ipc_hash.o \
		osfmk/ipc/ipc_space.o \
		osfmk/ipc/ipc_kmsg.o \
		osfmk/ipc/ipc_notify.o \
		osfmk/ipc/ipc_object.o \
		osfmk/ipc/ipc_pset.o \
		osfmk/ipc/ipc_table.o \
		osfmk/ipc/ipc_voucher.o \
		osfmk/ipc/mig_log.o \
		osfmk/ipc/mach_port.o \
		osfmk/ipc/mach_msg.o \
		osfmk/ipc/mach_debug.o \
		osfmk/ipc/mach_kernelrpc.o \
		osfmk/ipc/ipc_init.o \
		osfmk/ipc/ipc_right.o \
		osfmk/ipc/ipc_mqueue.o \
		osfmk/ipc/ipc_port.o \
		osfmk/kern/sync_sema.o \
		darling/down_interruptible.o \
		darling/traps.o \
		darling/task_registry.o \
		darling/module.o \
		darling/host_info.o \
		darling/evprocfd.o \
		darling/evpsetfd.o \
		darling/pthread_kill.o \
		darling/psynch_support.o \
		darling/foreign_mm.o \
		darling/continuation.o \
		darling/continuation-asm.o \
		osfmk/duct/darling_xnu_init.o \
		osfmk/duct/duct_atomic.o \
		osfmk/duct/duct_ipc_pset.o \
		osfmk/duct/duct_kern_clock.o \
		osfmk/duct/duct_kern_debug.o \
		osfmk/duct/duct_kern_kalloc.o \
		osfmk/duct/duct_kern_printf.o \
		osfmk/duct/duct_kern_startup.o \
		osfmk/duct/duct_kern_sysctl.o \
		osfmk/duct/duct_kern_task.o \
		osfmk/duct/duct_kern_thread_act.o \
		osfmk/duct/duct_kern_thread.o \
		osfmk/duct/duct_kern_thread_call.o \
		osfmk/duct/duct_kern_timer_call.o \
		osfmk/duct/duct_kern_zalloc.o \
		osfmk/duct/duct_ipc_importance.o \
		osfmk/duct/duct_libsa.o \
		osfmk/duct/duct_machine_routines.o \
		osfmk/duct/duct_machine_rtclock.o \
		osfmk/duct/duct_pcb.o \
		osfmk/duct/duct_vm_init.o \
		osfmk/duct/duct_vm_kern.o \
		osfmk/duct/duct_vm_map.o \
		osfmk/duct/duct_vm_user.o \
		osfmk/duct/duct_arm_locks_arm.o \
		osfmk/duct/duct_fileport.o \
		osfmk/kern/clock_oldops.o \
		osfmk/kern/ipc_clock.o \
		osfmk/kern/ipc_tt.o \
		osfmk/kern/ipc_sync.o \
		osfmk/kern/ipc_misc.o \
		osfmk/kern/host.o \
		osfmk/kern/ipc_host.o \
		osfmk/kern/ipc_kobject.o \
		osfmk/kern/mk_timer.o \
		osfmk/kern/ipc_mig.o \
		osfmk/kern/locks.o \
		osfmk/kern/ltable.o \
		osfmk/kern/waitq.o \
		duct/osfmk/dummy-locks.o \
		duct/osfmk/dummy-misc.o \
		duct/osfmk/dummy-kern.o \
		duct/osfmk/dummy-machine.o \
		duct/osfmk/dummy-vm-resident.o \
		duct/osfmk/dummy-kern-thread-call.o \
		duct/osfmk/dummy-vm-map.o \
		duct/osfmk/dummy-vm-user.o \
		duct/osfmk/dummy-kern-task.o \
		duct/osfmk/dummy-kern-thread.o \
		duct/osfmk/dummy-kern-audit-sessionport.o \
		duct/osfmk/dummy-kern-processor.o \
		duct/osfmk/dummy-kern-syscall-emulation.o \
		duct/osfmk/dummy-kern-zalloc.o \
		duct/osfmk/dummy-kern-sync-lock.o \
		duct/osfmk/dummy-kern-machine.o \
		duct/osfmk/dummy-kern-clock-oldops.o \
		duct/osfmk/dummy-kern-thread-policy.o \
		duct/osfmk/dummy-kern-task-policy.o \
		duct/osfmk/dummy-kern-mk-sp.o \
		duct/osfmk/dummy-vm-memory-object.o \
		duct/osfmk/dummy-kern-kmod.o \
		duct/osfmk/dummy-vm-kern.o \
		duct/osfmk/dummy-vm-debug.o \
		duct/osfmk/dummy-kern-thread-act.o \
		duct/osfmk/dummy-kern-host-notify.o \
		duct/bsd/dummy-kdebug.o \
		duct/bsd/dummy-init.o \
		libkern/gen/OSAtomicOperations.o \
		$(MIGDIR_REL)/osfmk/mach/task_server.o \
		$(MIGDIR_REL)/osfmk/mach/clock_server.o \
		$(MIGDIR_REL)/osfmk/mach/clock_priv_server.o \
		$(MIGDIR_REL)/osfmk/mach/processor_server.o \
		$(MIGDIR_REL)/osfmk/mach/host_priv_server.o \
		$(MIGDIR_REL)/osfmk/mach/host_security_server.o \
		$(MIGDIR_REL)/osfmk/mach/lock_set_server.o \
		$(MIGDIR_REL)/osfmk/mach/mach_port_server.o \
		$(MIGDIR_REL)/osfmk/mach/mach_vm_server.o \
		$(MIGDIR_REL)/osfmk/mach/mach_host_server.o \
		$(MIGDIR_REL)/osfmk/mach/mach_voucher_server.o \
		$(MIGDIR_REL)/osfmk/mach/mach_voucher_attr_control_server.o \
		$(MIGDIR_REL)/osfmk/mach/processor_set_server.o \
		$(MIGDIR_REL)/osfmk/mach/thread_act_server.o \
		$(MIGDIR_REL)/osfmk/mach/clock_reply_user.o \
		$(MIGDIR_REL)/osfmk/mach/notify_user.o \
		$(MIGDIR_REL)/osfmk/device/device_server.o \
		$(MIGDIR_REL)/osfmk/UserNotification/UNDReply_server.o \
		pexpert/duct/duct_gen_bootargs.o \
		pexpert/duct/duct_pe_kprintf.o \
		darling/binfmt.o \
		darling/commpage.o

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

