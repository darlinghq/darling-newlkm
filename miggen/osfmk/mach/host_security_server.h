#ifndef	_host_security_server_
#define	_host_security_server_

/* Module host_security */

#include <string.h>
#include <mach/ndr.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/notify.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/port.h>

#ifdef AUTOTEST
#ifndef FUNCTION_PTR_T
#define FUNCTION_PTR_T
typedef void (*function_ptr_t)(mach_port_t, char *, mach_msg_type_number_t);
typedef struct {
        char            *name;
        function_ptr_t  function;
} function_table_entry;
typedef function_table_entry   *function_table_t;
#endif /* FUNCTION_PTR_T */
#endif /* AUTOTEST */

#ifndef	host_security_MSG_COUNT
#define	host_security_MSG_COUNT	2
#endif	/* host_security_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mig.h>
#include <kern/ipc_kobject.h>
#include <kern/ipc_tt.h>
#include <kern/ipc_host.h>
#include <kern/ipc_sync.h>
#include <kern/ledger.h>
#include <kern/processor.h>
#include <kern/sync_lock.h>
#include <kern/sync_sema.h>
#include <vm/memory_object.h>
#include <vm/vm_map.h>
#include <kern/ipc_mig.h>
#include <mach/mig.h>
#include <mach/mach_types.h>

#ifdef __BeforeMigServerHeader
__BeforeMigServerHeader
#endif /* __BeforeMigServerHeader */


/* Routine host_security_create_task_token */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_security_create_task_token
(
	host_security_t host_security,
	task_t parent_task,
	security_token_t sec_token,
	audit_token_t audit_token,
	host_t host,
	ledger_array_t ledgers,
	mach_msg_type_number_t ledgersCnt,
	boolean_t inherit_memory,
	task_t *child_task
);

/* Routine host_security_set_task_token */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t host_security_set_task_token
(
	host_security_t host_security,
	task_t target_task,
	security_token_t sec_token,
	audit_token_t audit_token,
	host_t host
);

#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
boolean_t host_security_server(
		mach_msg_header_t *InHeadP,
		mach_msg_header_t *OutHeadP);

#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
mig_routine_t host_security_server_routine(
		mach_msg_header_t *InHeadP);


/* Description of this subsystem, for use in direct RPC */
extern const struct host_security_subsystem {
	mig_server_routine_t	server;	/* Server routine */
	mach_msg_id_t	start;	/* Min routine number */
	mach_msg_id_t	end;	/* Max routine number + 1 */
	unsigned int	maxsize;	/* Max msg size */
	vm_address_t	reserved;	/* Reserved */
	struct routine_descriptor	/*Array of routine descriptors */
		routine[2];
} host_security_subsystem;

/* typedefs for all requests */

#ifndef __Request__host_security_subsystem__defined
#define __Request__host_security_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		/* start of the kernel processed data */
		mach_msg_body_t msgh_body;
		mach_msg_port_descriptor_t parent_task;
		mach_msg_port_descriptor_t host;
		mach_msg_ool_ports_descriptor_t ledgers;
		/* end of the kernel processed data */
		NDR_record_t NDR;
		security_token_t sec_token;
		audit_token_t audit_token;
		mach_msg_type_number_t ledgersCnt;
		boolean_t inherit_memory;
	} __Request__host_security_create_task_token_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		/* start of the kernel processed data */
		mach_msg_body_t msgh_body;
		mach_msg_port_descriptor_t target_task;
		mach_msg_port_descriptor_t host;
		/* end of the kernel processed data */
		NDR_record_t NDR;
		security_token_t sec_token;
		audit_token_t audit_token;
	} __Request__host_security_set_task_token_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Request__host_security_subsystem__defined */


/* union of all requests */

#ifndef __RequestUnion__host_security_subsystem__defined
#define __RequestUnion__host_security_subsystem__defined
union __RequestUnion__host_security_subsystem {
	__Request__host_security_create_task_token_t Request_host_security_create_task_token;
	__Request__host_security_set_task_token_t Request_host_security_set_task_token;
};
#endif /* __RequestUnion__host_security_subsystem__defined */
/* typedefs for all replies */

#ifndef __Reply__host_security_subsystem__defined
#define __Reply__host_security_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		/* start of the kernel processed data */
		mach_msg_body_t msgh_body;
		mach_msg_port_descriptor_t child_task;
		/* end of the kernel processed data */
	} __Reply__host_security_create_task_token_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
	} __Reply__host_security_set_task_token_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Reply__host_security_subsystem__defined */


/* union of all replies */

#ifndef __ReplyUnion__host_security_subsystem__defined
#define __ReplyUnion__host_security_subsystem__defined
union __ReplyUnion__host_security_subsystem {
	__Reply__host_security_create_task_token_t Reply_host_security_create_task_token;
	__Reply__host_security_set_task_token_t Reply_host_security_set_task_token;
};
#endif /* __RequestUnion__host_security_subsystem__defined */

#ifndef subsystem_to_name_map_host_security
#define subsystem_to_name_map_host_security \
    { "host_security_create_task_token", 600 },\
    { "host_security_set_task_token", 601 }
#endif

#ifdef __AfterMigServerHeader
__AfterMigServerHeader
#endif /* __AfterMigServerHeader */

#endif	 /* _host_security_server_ */
