BUILD_ROOT ?= $(src)

ccflags-y := -D__DARLING__ \
	-I$(BUILD_ROOT)/EXTERNAL_HEADERS \
	-I$(BUILD_ROOT)/EXTERNAL_HEADERS/bsd \
	-DPAGE_SIZE_FIXED \
	-DCONFIG_SCHED_TRADITIONAL \
	-freorder-blocks \
	-fno-builtin \
	-fno-common \
	-fsigned-bitfields \
	-fno-strict-aliasing \
	-fno-keep-inline-functions \
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
	-D__STDC_VERSION__=199901L \
	-D__LITTLE_ENDIAN__=1 \
	-Wno-declaration-after-statement \
	-Wno-undef \
	-Wno-maybe-uninitialized \
	-D__private_extern__=extern \
	-D_MODE_T -D_NLINK_T -DVM32_SUPPORT=1 -DMACH_KERNEL_PRIVATE \
	-I$(BUILD_ROOT)/miggen/bsd \
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
	-I$(BUILD_ROOT)/linux \
	-I$(BUILD_ROOT)/miggen/osfmk \
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
	-DVM_PRESSURE_EVENTS \
	-DCONFIG_KERNEL_0DAY_SYSCALL_HANDLER \
	-DEVENTMETER \
	-DCONFIG_APP_PROFILE=0 \
	-DDEBUG

# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq ($(KERNELRELEASE),)
	obj-m := darling-mach.o
	darling-mach-objs := osfmk/ipc/ipc_entry.o \
		osfmk/ipc/ipc_hash.o \
		osfmk/ipc/ipc_space.o \
		osfmk/ipc/ipc_kmsg.o \
		osfmk/ipc/ipc_labelh.o \
		osfmk/ipc/ipc_notify.o \
		osfmk/ipc/ipc_object.o \
		osfmk/ipc/ipc_pset.o \
		osfmk/ipc/ipc_table.o \
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
		linux/down_interruptible.o \
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
		osfmk/duct/duct_kern_waitqueue.o \
		osfmk/duct/duct_kern_zalloc.o \
		osfmk/duct/duct_libsa.o \
		osfmk/duct/duct_machine_routines.o \
		osfmk/duct/duct_machine_rtclock.o \
		osfmk/duct/duct_pcb.o \
		osfmk/duct/duct_vm_init.o \
		osfmk/duct/duct_vm_kern.o \
		osfmk/duct/duct_vm_map.o \
		osfmk/duct/duct_vm_user.o \
		osfmk/kern/clock_oldops.o \
		osfmk/kern/ipc_tt.o \
		osfmk/kern/host.o \
		duct/osfmk/dummy-kern.o \
		duct/osfmk/dummy-vm-resident.o \
		duct/osfmk/dummy-kern-thread-call.o \
		duct/osfmk/dummy-vm-map.o

# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	rm -f *.o *.ko
	rm -f *.mod.c
	rm -rf modules.order Module.symvers

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

