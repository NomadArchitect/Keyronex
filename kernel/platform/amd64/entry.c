/*
 * Copyright (c) 2025 NetaScale Object Solutions.
 * Created on Wed Sep 6 2023.
 */

#include <kdk/amd64.h>
#include <kdk/amd64/gdt.h>
#include <kdk/amd64/portio.h>
#include <kdk/amd64/regs.h>
#include <kdk/executive.h>
#include <kdk/kern.h>
#include <limine.h>

#include "platform/amd64/intr.h"
#include "vm/vmp.h"

enum { kPortCOM1 = 0x3f8 };

/* intr.c */
void intr_init(void);
/* misc.c */
void setup_cpu_gdt(kcpu_t *cpu);
/* tc_pvclock.c */
void pvclock_init(void);
void pvclock_cpu_init(void);

extern kthread_t thread0;

static void
serial_init()
{
	outb(kPortCOM1 + 1, 0x00);
	outb(kPortCOM1 + 3, 0x80);
	outb(kPortCOM1 + 0, 0x03);
	outb(kPortCOM1 + 1, 0x00);
	outb(kPortCOM1 + 3, 0x03);
	outb(kPortCOM1 + 2, 0xC7);
	outb(kPortCOM1 + 4, 0x0B);
}

void
pac_putc(int ch, void *ctx)
{
	if (ch == '\n')
		pac_putc('\r', NULL);
	/* put to com1 */
	while (!(inb(kPortCOM1 + 5) & 0x20))
		;
	outb(kPortCOM1, ch);
}

void plat_first_init(void)
{
	wrmsr(kAMD64MSRGSBase, (uint64_t)&bootstrap_cpu_local_data);
	serial_init();
	intr_init();
	bootstrap_cpu_local_data.md.self = &bootstrap_cpu_local_data;
	bootstrap_cpu_local_data.md.hard_ipl = 0;
	bootstrap_cpu_local_data.md.soft_ipl = 0;
}

void plat_pre_smp_init(void)
{
	pvclock_init();
}

void plat_ap_early_init(kcpu_t *cpu, struct limine_smp_info *smpi)
{
	write_cr3(kernel_process->vm->md.table);
}

static kspinlock_t early_map_lock = KSPINLOCK_INITIALISER;

void plat_common_core_early_init(kcpu_t *cpu, kthread_t *idle_thread, struct limine_smp_info *smpi)
{
	paddr_t lapic_base;
	int r;

	wrmsr(kAMD64MSRGSBase, (uintptr_t)cpu->local_data);
	idt_load();
	lapic_base = rdmsr(kAMD64MSRAPICBase) & 0xfffff000;
	ke_spinlock_acquire_nospl(&early_map_lock);
	r = vm_ps_map_physical_view(kernel_process->vm, &cpu->cpucb.lapic_base,
	    PGSIZE, lapic_base, kVMAll, kVMAll, false);
	kassert(r == 0);
	ke_spinlock_release_nospl(&early_map_lock);
	lapic_enable(0xff);
	setup_cpu_gdt(cpu);
	intr_init();

	/* measure thrice and average it */
	cpu->cpucb.lapic_tps = 0;
	cpu->cpucb.lapic_id = smpi->lapic_id;
	for (int i = 0; i < 3; i++)
		cpu->cpucb.lapic_tps += lapic_timer_calibrate() / 3;


	pvclock_cpu_init();
}

void plat_common_core_late_init(kcpu_t *cpu, kthread_t *idle_thread, struct limine_smp_info *smpi)
{
	/* enable SSE and SSE2 */
	uint64_t cr0 = read_cr0();
	cr0 &= ~((uint64_t)1 << 2);
	cr0 |= (uint64_t)1 << 1;
	write_cr0(cr0);

	uint64_t cr4 = read_cr4();
	cr4 |= (uint64_t)3 << 9;
	write_cr4(cr4);

	lapic_timer_start();
	asm("sti");
}
