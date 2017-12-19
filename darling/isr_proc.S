.globl isr_proc

#ifdef __x86_64__
#include <asm/errno.h>
#include <asm/segment.h>
#include <asm/percpu.h>
#include <asm/thread_info.h>

isr_proc:
	swapgs

#if 0
	movq	%rsp, PER_CPU_VAR(rsp_scratch)
	movq	PER_CPU_VAR(cpu_current_top_of_stack), %rsp

	pushq	$__USER_DS			/* pt_regs->ss */
	pushq	PER_CPU_VAR(rsp_scratch)	/* pt_regs->sp */
	pushq	%r11				/* pt_regs->flags */
	pushq	$__USER_CS			/* pt_regs->cs */
	pushq	%rcx				/* pt_regs->ip */
#endif



	pushq	%rax				/* pt_regs->orig_ax */
	pushq	%rdi				/* pt_regs->di */
	pushq	%rsi				/* pt_regs->si */
	pushq	%rdx				/* pt_regs->dx */
	pushq	%rcx				/* pt_regs->cx */
	pushq	$-ENOSYS			/* pt_regs->ax */
	pushq	%r8				/* pt_regs->r8 */
	pushq	%r9				/* pt_regs->r9 */
	pushq	%r10				/* pt_regs->r10 */
	pushq	%r11				/* pt_regs->r11 */
	sub	$(6*8), %rsp			/* pt_regs->bp, bx, r12-15 not saved */
	cld

	movq %rsp, %rdi
	call isr_proc_impl

	swapgs

	movq 6*8(%rsp), %r11
	movq 7*8(%rsp), %r10
	movq 8*8(%rsp), %r9
	movq 9*8(%rsp), %r8
	movq 10*8(%rsp), %rax
	movq 11*8(%rsp), %rcx
	movq 12*8(%rsp), %rdx
	movq 13*8(%rsp), %rsi
	movq 14*8(%rsp), %rdi

	add $(16*8), %rsp
	// movq	PER_CPU_VAR(rsp_scratch), %rsp
	iretq

#else
#	warning Missing isr_proc implementation for this architecture!
#endif

