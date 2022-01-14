#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/locks.h>
#include <duct/duct_post_xnu.h>

void kprintf(const char* fmt, ...);

void lck_mtx_lock_wait(lck_mtx_t* lck, thread_t holder, struct turnstile** ts) {
	kprintf("lck_mtx_lock_wait() not yet implemented\n");
};

int lck_mtx_lock_acquire(lck_mtx_t* lck, struct turnstile* ts) {
	kprintf("lck_mtx_lock_wait() not yet implemented\n");
	return 0;
};

boolean_t lck_mtx_unlock_wakeup(lck_mtx_t* lck, thread_t holder) {
	kprintf("lck_mtx_unlock_wakeup() not yet implemented\n");
	return false;
};

void lck_mtx_unlockspin_wakeup(lck_mtx_t* lck) {
	kprintf("lck_mtx_unlockspin_wakeup() not yet implemented\n");
};

void lck_mtx_yield(lck_mtx_t* lck) {
	kprintf("lck_mtx_yield() not yet implemented\n");
};
