#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <bank/bank_internal.h>
#include <sys/persona.h>
#include <duct/duct_post_xnu.h>

kern_return_t bank_get_bank_ledger_thread_group_and_persona(ipc_voucher_t voucher, ledger_t* bankledger, struct thread_group** banktg, uint32_t* persona_id) {
	kprintf("not implemented: bank_get_bank_ledger_thread_group_and_persona()\n");

	// ...but pretend we did a thing, just to please `ipc_kmsg.c`

	if (bankledger) {
		*bankledger = LEDGER_NULL;
	}

	if (banktg) {
		*banktg = NULL;
	}

	if (persona_id) {
		*persona_id = PERSONA_ID_NONE;
	}

	return KERN_SUCCESS;
};
