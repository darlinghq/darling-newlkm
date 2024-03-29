project(lkm)

add_definitions(
	-DKERNEL
	-DKERNEL_PRIVATE
	-DKERNEL_SERVER=1
	-DKERNEL_USER=1
	-DMACH_KERNEL_PRIVATE
	-DMACH_NOTIFY_SEND_POSSIBLE_EXPECTED
)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/osfmk)

set(MIG_USER_SOURCE_SUFFIX "User.c")
set(MIG_SERVER_SOURCE_SUFFIX "Server.c")
set(MIG_SERVER_HEADER_SUFFIX "Server.h")
set(MIG_NO_XTRACE 1)

mig(osfmk/UserNotification/UNDReply.defs)

set(MIG_USER_SOURCE_SUFFIX "_user.c")
set(MIG_SERVER_SOURCE_SUFFIX "_server.c")
set(MIG_SERVER_HEADER_SUFFIX "_server.h")

mig(osfmk/atm/atm_notification.defs)
mig(osfmk/atm/atm_types.defs)
mig(osfmk/device/device.defs)
mig(osfmk/device/device_types.defs)
mig(osfmk/gssd/gssd_mach.defs)
mig(osfmk/kextd/kextd_mach.defs)
mig(osfmk/lockd/lockd_mach.defs)
mig(osfmk/mach/arcade_register.defs)
mig(osfmk/mach/arcade_upcall.defs)
mig(osfmk/mach/audit_triggers.defs)
mig(osfmk/mach/clock.defs)
mig(osfmk/mach/clock_priv.defs)
mig(osfmk/mach/clock_reply.defs)
mig(osfmk/mach/clock_types.defs)
mig(osfmk/mach/coalition_notification.defs)
mig(osfmk/mach_debug/mach_debug_types.defs)
mig(osfmk/mach/exc.defs)
mig(osfmk/mach/fairplayd_notification.defs)
mig(osfmk/mach/host_notify_reply.defs)
mig(osfmk/mach/host_priv.defs)
mig(osfmk/mach/host_security.defs)
mig(osfmk/mach/iocompressionstats_notification.defs)
mig(osfmk/mach/ktrace_background.defs)
mig(osfmk/mach/ledger.defs)
mig(osfmk/mach/lock_set.defs)
mig(osfmk/mach/mach_eventlink.defs)
mig(osfmk/mach/mach_exc.defs)
mig(osfmk/mach/mach_host.defs)
mig(osfmk/mach/machine/machine_types.defs)
mig(osfmk/mach/mach_notify.defs)
mig(osfmk/mach/mach_port.defs)
mig(osfmk/mach/mach_types.defs)
mig(osfmk/mach/mach_vm.defs)
mig(osfmk/mach/mach_voucher_attr_control.defs)
mig(osfmk/mach/mach_voucher.defs)
mig(osfmk/mach/memory_entry.defs)
mig(osfmk/mach/memory_object_control.defs)
mig(osfmk/mach/memory_object_default.defs)
mig(osfmk/mach/memory_object.defs)
mig(osfmk/mach/notify.defs)
mig(osfmk/mach/processor.defs)
mig(osfmk/mach/processor_set.defs)
mig(osfmk/mach/prof.defs)
mig(osfmk/mach/resource_notify.defs)
mig(osfmk/mach/restartable.defs)
mig(osfmk/mach/semaphore.defs)
mig(osfmk/mach/std_types.defs)
mig(osfmk/mach/sync.defs)
mig(osfmk/mach/sysdiagnose_notification.defs)
mig(osfmk/mach/task_access.defs)
mig(osfmk/mach/task.defs)
mig(osfmk/mach/telemetry_notification.defs)
mig(osfmk/mach/thread_act.defs)
mig(osfmk/mach/upl.defs)
mig(osfmk/mach/vfs_nspace.defs)
mig(osfmk/mach/vm32_map.defs)
mig(osfmk/mach/vm_map.defs)
mig(osfmk/UserNotification/UNDReply.defs)
mig(osfmk/UserNotification/UNDRequest.defs)
mig(osfmk/UserNotification/UNDTypes.defs)

file(RELATIVE_PATH MIGDIR_REL "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")

add_custom_target(lkm_generate
	DEPENDS
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/upl.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/host_notify_reply.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/vm32_map_server.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/mach_notify.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/mach_voucher_attr_control.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/memory_entry_server.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/memory_object_control.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/memory_object_server.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/memory_object_default_server.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/mach_voucher_server.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/vm_map.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/resource_notify.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/restartable_server.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/exc_server.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/mach_exc_server.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/notify_server.h
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/task_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/clock_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/clock_priv_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/processor_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/host_priv_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/host_security_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/lock_set_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/mach_port_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/mach_vm_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/mach_host_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/processor_set_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/thread_act_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/mach/clock_reply_user.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/device/device_server.c
		${CMAKE_CURRENT_BINARY_DIR}/osfmk/UserNotification/UNDReplyServer.c
)

if(BUILD_LKM_WITH_CLANG)
	# because KBuild doesn't include the module's Makefile when doing the final complation for a module (i.e. for `<module-name>.mod.c`), apparently
	# we then use it with `EXTRA_CFLAGS=${LKM_CLANG_COMPAT_FLAGS}`, but we really only need it to apply to `*.mod.c`
	# unforuntately, KBuild doesn't look at any specific variables for those files, only generic global CFLAGS variables like EXTRA_CFLAGS
	set(LKM_CLANG_COMPAT_FLAGS
		-Wno-error=unknown-warning-option
		"-include ${CMAKE_CURRENT_SOURCE_DIR}/include/duct/compiler/clang/asm-inline.h"
	)
	list(JOIN LKM_CLANG_COMPAT_FLAGS " " LKM_CLANG_COMPAT_FLAGS)
else()
	set(LKM_CLANG_COMPAT_FLAGS "")
endif()

add_custom_target(lkm
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/lkm-make
	COMMENT "Darling no longer requires building the kernel module. There is no need to run `make lkm` anymore"
)

add_custom_target(lkm_install
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/lkm-make
	COMMENT "Darling no longer requires building the kernel module. There is no need to run `make lkm_install` anymore"
)

add_custom_target(lkm_clean
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/lkm-make
	COMMENT "Darling no longer requires building the kernel module. There is no need to run `make lkm_clean` anymore")
