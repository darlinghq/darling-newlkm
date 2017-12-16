.globl isr_proc

#ifdef __x86_64__

isr_proc:
	swapgs
	call isr_proc_impl
	swapgs
	iretq

#else
#	warning Missing isr_proc implementation for this architecture!
#endif

