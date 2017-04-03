#ifndef	_security_server_
#define	_security_server_

/* Module security */

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

#ifndef	security_MSG_COUNT
#define	security_MSG_COUNT	10
#endif	/* security_MSG_COUNT */

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


/* Routine mach_get_task_label */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_get_task_label
(
	ipc_space_t task,
	mach_port_name_t *label
);

/* Routine mach_get_task_label_text */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_get_task_label_text
(
	ipc_space_t task,
	labelstr_t policies,
	labelstr_t label
);

/* Routine mach_get_label */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_get_label
(
	ipc_space_t task,
	mach_port_name_t port,
	mach_port_name_t *label
);

/* Routine mach_get_label_text */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_get_label_text
(
	ipc_space_t task,
	mach_port_name_t name,
	labelstr_t policies,
	labelstr_t label
);

/* Routine mach_set_port_label */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mach_set_port_label
(
	ipc_space_t task,
	mach_port_name_t name,
	labelstr_t label
);

/* Routine mac_check_service */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mac_check_service
(
	ipc_space_t task,
	labelstr_t subject,
	labelstr_t object,
	labelstr_t service,
	labelstr_t perm
);

/* Routine mac_port_check_service_obj */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mac_port_check_service_obj
(
	ipc_space_t task,
	labelstr_t subject,
	mach_port_name_t object,
	labelstr_t service,
	labelstr_t perm
);

/* Routine mac_port_check_access */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mac_port_check_access
(
	ipc_space_t task,
	mach_port_name_t subject,
	mach_port_name_t object,
	labelstr_t service,
	labelstr_t perm
);

/* Routine mac_label_new */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mac_label_new
(
	ipc_space_t task,
	mach_port_name_t *name,
	labelstr_t label
);

/* Routine mac_request_label */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t mac_request_label
(
	ipc_space_t task,
	mach_port_name_t subject,
	mach_port_name_t object,
	labelstr_t service,
	mach_port_name_t *newlabel
);

#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
boolean_t security_server(
		mach_msg_header_t *InHeadP,
		mach_msg_header_t *OutHeadP);

#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
mig_routine_t security_server_routine(
		mach_msg_header_t *InHeadP);


/* Description of this subsystem, for use in direct RPC */
extern const struct security_subsystem {
	mig_server_routine_t	server;	/* Server routine */
	mach_msg_id_t	start;	/* Min routine number */
	mach_msg_id_t	end;	/* Max routine number + 1 */
	unsigned int	maxsize;	/* Max msg size */
	vm_address_t	reserved;	/* Reserved */
	struct routine_descriptor	/*Array of routine descriptors */
		routine[10];
} security_subsystem;

/* typedefs for all requests */

#ifndef __Request__security_subsystem__defined
#define __Request__security_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
	} __Request__mach_get_task_label_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_msg_type_number_t policiesOffset; /* MiG doesn't use it */
		mach_msg_type_number_t policiesCnt;
		char policies[512];
	} __Request__mach_get_task_label_text_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_port_name_t port;
	} __Request__mach_get_label_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_port_name_t name;
		mach_msg_type_number_t policiesOffset; /* MiG doesn't use it */
		mach_msg_type_number_t policiesCnt;
		char policies[512];
	} __Request__mach_get_label_text_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_port_name_t name;
		mach_msg_type_number_t labelOffset; /* MiG doesn't use it */
		mach_msg_type_number_t labelCnt;
		char label[512];
	} __Request__mach_set_port_label_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_msg_type_number_t subjectOffset; /* MiG doesn't use it */
		mach_msg_type_number_t subjectCnt;
		char subject[512];
		mach_msg_type_number_t objectOffset; /* MiG doesn't use it */
		mach_msg_type_number_t objectCnt;
		char object[512];
		mach_msg_type_number_t serviceOffset; /* MiG doesn't use it */
		mach_msg_type_number_t serviceCnt;
		char service[512];
		mach_msg_type_number_t permOffset; /* MiG doesn't use it */
		mach_msg_type_number_t permCnt;
		char perm[512];
	} __Request__mac_check_service_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_msg_type_number_t subjectOffset; /* MiG doesn't use it */
		mach_msg_type_number_t subjectCnt;
		char subject[512];
		mach_port_name_t object;
		mach_msg_type_number_t serviceOffset; /* MiG doesn't use it */
		mach_msg_type_number_t serviceCnt;
		char service[512];
		mach_msg_type_number_t permOffset; /* MiG doesn't use it */
		mach_msg_type_number_t permCnt;
		char perm[512];
	} __Request__mac_port_check_service_obj_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_port_name_t subject;
		mach_port_name_t object;
		mach_msg_type_number_t serviceOffset; /* MiG doesn't use it */
		mach_msg_type_number_t serviceCnt;
		char service[512];
		mach_msg_type_number_t permOffset; /* MiG doesn't use it */
		mach_msg_type_number_t permCnt;
		char perm[512];
	} __Request__mac_port_check_access_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_msg_type_number_t labelOffset; /* MiG doesn't use it */
		mach_msg_type_number_t labelCnt;
		char label[512];
	} __Request__mac_label_new_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_port_name_t subject;
		mach_port_name_t object;
		mach_msg_type_number_t serviceOffset; /* MiG doesn't use it */
		mach_msg_type_number_t serviceCnt;
		char service[512];
	} __Request__mac_request_label_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Request__security_subsystem__defined */


/* union of all requests */

#ifndef __RequestUnion__security_subsystem__defined
#define __RequestUnion__security_subsystem__defined
union __RequestUnion__security_subsystem {
	__Request__mach_get_task_label_t Request_mach_get_task_label;
	__Request__mach_get_task_label_text_t Request_mach_get_task_label_text;
	__Request__mach_get_label_t Request_mach_get_label;
	__Request__mach_get_label_text_t Request_mach_get_label_text;
	__Request__mach_set_port_label_t Request_mach_set_port_label;
	__Request__mac_check_service_t Request_mac_check_service;
	__Request__mac_port_check_service_obj_t Request_mac_port_check_service_obj;
	__Request__mac_port_check_access_t Request_mac_port_check_access;
	__Request__mac_label_new_t Request_mac_label_new;
	__Request__mac_request_label_t Request_mac_request_label;
};
#endif /* __RequestUnion__security_subsystem__defined */
/* typedefs for all replies */

#ifndef __Reply__security_subsystem__defined
#define __Reply__security_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
		mach_port_name_t label;
	} __Reply__mach_get_task_label_t;
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
		mach_msg_type_number_t labelOffset; /* MiG doesn't use it */
		mach_msg_type_number_t labelCnt;
		char label[512];
	} __Reply__mach_get_task_label_text_t;
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
		mach_port_name_t label;
	} __Reply__mach_get_label_t;
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
		mach_msg_type_number_t labelOffset; /* MiG doesn't use it */
		mach_msg_type_number_t labelCnt;
		char label[512];
	} __Reply__mach_get_label_text_t;
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
	} __Reply__mach_set_port_label_t;
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
	} __Reply__mac_check_service_t;
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
	} __Reply__mac_port_check_service_obj_t;
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
	} __Reply__mac_port_check_access_t;
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
		mach_port_name_t name;
	} __Reply__mac_label_new_t;
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
		mach_port_name_t newlabel;
	} __Reply__mac_request_label_t;
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Reply__security_subsystem__defined */


/* union of all replies */

#ifndef __ReplyUnion__security_subsystem__defined
#define __ReplyUnion__security_subsystem__defined
union __ReplyUnion__security_subsystem {
	__Reply__mach_get_task_label_t Reply_mach_get_task_label;
	__Reply__mach_get_task_label_text_t Reply_mach_get_task_label_text;
	__Reply__mach_get_label_t Reply_mach_get_label;
	__Reply__mach_get_label_text_t Reply_mach_get_label_text;
	__Reply__mach_set_port_label_t Reply_mach_set_port_label;
	__Reply__mac_check_service_t Reply_mac_check_service;
	__Reply__mac_port_check_service_obj_t Reply_mac_port_check_service_obj;
	__Reply__mac_port_check_access_t Reply_mac_port_check_access;
	__Reply__mac_label_new_t Reply_mac_label_new;
	__Reply__mac_request_label_t Reply_mac_request_label;
};
#endif /* __RequestUnion__security_subsystem__defined */

#ifndef subsystem_to_name_map_security
#define subsystem_to_name_map_security \
    { "mach_get_task_label", 5200 },\
    { "mach_get_task_label_text", 5201 },\
    { "mach_get_label", 5202 },\
    { "mach_get_label_text", 5203 },\
    { "mach_set_port_label", 5204 },\
    { "mac_check_service", 5205 },\
    { "mac_port_check_service_obj", 5206 },\
    { "mac_port_check_access", 5207 },\
    { "mac_label_new", 5208 },\
    { "mac_request_label", 5209 }
#endif

#ifdef __AfterMigServerHeader
__AfterMigServerHeader
#endif /* __AfterMigServerHeader */

#endif	 /* _security_server_ */
