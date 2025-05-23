/*
 * Copyright (c) 2024 NetaScale Object Solutions.
 * Created on Mon Aug 05 2024.
 */

#include "kdk/executive.h"
#include "kdk/libkern.h"
#include "kdk/riscv64.h"
#include "kern/ki.h"
#include "vm/vmp.h"

/* trap.S */
void asm_thread_trampoline(void);
void asm_swtch(riscv64_context_t **pold_sp, riscv64_context_t *psp);
/* vm_riscv64.c */
void write_satp(paddr_t paddr);

static inline uint64_t
read_sstatus(void)
{
	uint64_t value;
	asm volatile("csrr %0, sstatus" : "=r"(value));
	return value;
}

static inline void
write_sstatus(uint64_t value)
{
	asm volatile("csrw sstatus, %0" : : "r"(value) : "memory");
}

int
ki_disable_interrupts(void)
{
	uint64_t sstatus = read_sstatus();
	uint64_t old_sstatus = sstatus;
	sstatus &= ~0x2; /* sie */
	write_sstatus(sstatus);
	return (old_sstatus & 0x2) != 0;
}

void
ki_set_interrupts(int enabled)
{
	uint64_t sstatus = read_sstatus();
	if (enabled)
		sstatus |= 0x2;
	else
		sstatus &= ~0x2;
	write_sstatus(sstatus);
}

void
c_thread_trampoline(void (*func)(void *), void *arg)
{
	ke_spinlock_release_nospl(&curcpu()->old_thread->lock);
	splx(kIPL0);
	func(arg);
}

void
ki_thread_copy_fpu_state(kthread_t *to)
{
	/* TODO */
}

void
ke_thread_init_context(kthread_t *thread, md_intr_frame_t *fork_frame,
    void (*func)(void *), void *arg)
{
	riscv64_context_t *ctx;

	if (fork_frame == NULL) {
		ctx = thread->kstack_base + KSTACK_SIZE -
		    sizeof(riscv64_context_t);

	} else {
		md_intr_frame_t *frame;

		/* keep 38 * 8 in sync with trap.S! */
		frame = thread->kstack_base + KSTACK_SIZE - (38 * 8);
		ctx = thread->kstack_base + KSTACK_SIZE - (38 * 8) -
		    sizeof(riscv64_context_t);

		memcpy(frame, fork_frame, sizeof(*fork_frame));
		frame->a0 = 0;
		frame->sepc += 4;
	}

	ctx->ra = (uintptr_t)asm_thread_trampoline;
	ctx->s0 = (uintptr_t)func;
	ctx->s1 = (uintptr_t)arg;

	thread->pcb.sp = ctx;
	thread->pcb.supervisor_sp = (uintptr_t)thread->kstack_base + KSTACK_SIZE;
}

void
ki_tlb_flush_vaddr_locally(vaddr_t addr)
{
	asm volatile("sfence.vma %0" : : "r"(addr) : "memory");
}

void ki_tlb_flush_locally(void)
{
	asm volatile("sfence.vma zero" ::: "memory");
}

void
ki_tlb_flush_vaddr_globally(vaddr_t addr)
{
	ki_tlb_flush_vaddr_locally(addr);
}

void
ki_tlb_flush_globally(void)
{
	ki_tlb_flush_locally();
}

void
ki_enter_user_mode(uintptr_t ip, uintptr_t sp)
{
	ki_disable_interrupts();

	uintptr_t sstatus = 0;

	asm volatile("csrr %0, sstatus" : "=r"(sstatus));
	sstatus &= ~(1 << 8); /* clear SPP */
	sstatus |= (1 << 5);  /* set SPIE */

	asm volatile("csrw sstatus, %0\n\t"
		     "csrw sepc, %1\n\t"
		     "mv sp, %2\n\t"
		     "csrw sscratch, tp\n\t"
		     "mv tp, zero\n\t"
		     "sret"
		     :
		     : "r"(sstatus), "r"(ip), "r"(sp)
		     : "memory");
}

void
ke_set_tcb(uintptr_t tcb)
{
	kfatal("Unimplemented\n");
}

void
md_intr_frame_trace(md_intr_frame_t *frame)
{
	splraise(kIPLHigh);
	kprintf("Thread %p; CPU %p:\n", curthread(), curcpu());
	kfatal("Unimplemented\n");
}

void md_current_trace(void)
{
	splraise(kIPLHigh);
	kprintf("Thread %p; CPU %p:\n", curthread(), curcpu());
	for (;;) ;
}

void
md_switch(kthread_t *old_thread, kthread_t *new_thread)
{
	bool intr;

	intr = ki_disable_interrupts();

	old_thread->pcb.user_sp = KCPU_LOCAL_LOAD(md.saved_user_sp);
	old_thread->pcb.supervisor_sp = KCPU_LOCAL_LOAD(md.saved_supervisor_sp);
	KCPU_LOCAL_STORE(md.saved_user_sp, new_thread->pcb.user_sp);
	KCPU_LOCAL_STORE(md.saved_supervisor_sp, new_thread->pcb.supervisor_sp);

	write_satp(ex_curproc()->vm->md.table);

#if 0
	if (old_thread->user)
		save_fp(old_thread->pcb.fp);
	if (new_thread->user)
		restore_fp(new_thread->pcb.fp);
#endif

	asm_swtch(&old_thread->pcb.sp, new_thread->pcb.sp);
	ki_set_interrupts(intr);
}

void
md_send_dpc_ipi(kcpu_t *cpu)
{
	for (;;)
		;
}

void
ki_trap_recover(md_intr_frame_t *frame)
{
	ktrap_recovery_frame_t *recovery = &(curthread()->trap_recovery);

	frame->sepc = recovery->ra;
	frame->a0 = 1;
	frame->s0 = recovery->s0;
	frame->s1 = recovery->s1;
	frame->s2 = recovery->s2;
	frame->s3 = recovery->s3;
	frame->s4 = recovery->s4;
	frame->s5 = recovery->s5;
	frame->s6 = recovery->s6;
	frame->s7 = recovery->s7;
	frame->s8 = recovery->s8;
	frame->s9 = recovery->s9;
	frame->s10 = recovery->s10;
	frame->s11 = recovery->s11;
	frame->sp = recovery->sp;
}
