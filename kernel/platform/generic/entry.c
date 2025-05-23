#include <limine.h>
#include <stddef.h>
#include <stdint.h>

#include "executive/exp.h"
#include "kdk/executive.h"
#include "kdk/kern.h"
#include "kdk/kmem.h"
#include "kdk/libkern.h"
#include "kdk/object.h"
#include "kdk/vm.h"
#include "kern/ki.h"
#include "vm/vmp.h"

void ddk_init(void);

void plat_first_init(void);
void plat_pre_smp_init(void);
void plat_ap_early_init(kcpu_t *cpu, struct limine_smp_info *smpi);
void plat_common_core_early_init(kcpu_t *cpu, kthread_t *idle_thread,
    struct limine_smp_info *smpi);
void plat_common_core_late_init(kcpu_t *cpu, kthread_t *idle_thread,
    struct limine_smp_info *smpi);

extern uintptr_t idle_mask;
struct kcpu bootstrap_cpu;
struct kthread thread0;
kcpu_local_data_t bootstrap_cpu_local_data = {
	.cpu = &bootstrap_cpu,
	.cpu_num = 0,
	.curthread = &thread0
};
kspinlock_t pac_console_lock = KSPINLOCK_INITIALISER;
static int cpus_up = 0;
/*
 * can't rely on mutexes until scheduling is up (and in any case not in idle
 * thread), so this must be used instead
 */
static kspinlock_t early_lock = KSPINLOCK_INITIALISER;

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent.
LIMINE_BASE_REVISION(2);

volatile struct limine_framebuffer_request fb_request __attribute__((
    aligned(8))) = { .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0 };

static volatile struct limine_hhdm_request hhdm_request
    __attribute__((aligned(8))) = { .id = LIMINE_HHDM_REQUEST, .revision = 0 };

volatile struct limine_kernel_address_request kernel_address_request
    __attribute__((aligned(8))) = { .id = LIMINE_KERNEL_ADDRESS_REQUEST,
	    .revision = 0 };

static volatile struct limine_kernel_file_request kernel_file_request
    __attribute__((aligned(8))) = { .id = LIMINE_KERNEL_FILE_REQUEST,
	    .revision = 0 };

volatile struct limine_memmap_request memmap_request __attribute__((
    aligned(8))) = { .id = LIMINE_MEMMAP_REQUEST, .revision = 0 };

volatile struct limine_module_request module_request __attribute__((
    aligned(8))) = { .id = LIMINE_MODULE_REQUEST, .revision = 0 };

volatile struct limine_rsdp_request rsdp_request
    __attribute__((aligned(8))) = { .id = LIMINE_RSDP_REQUEST, .revision = 0 };

volatile struct limine_dtb_request dtb_request
__attribute__((aligned(8))) = { .id = LIMINE_DTB_REQUEST, .revision = 0 };

static volatile struct limine_smp_request smp_request
    __attribute__((aligned(8))) = { .id = LIMINE_SMP_REQUEST, .revision = 0 };

struct ex_boot_config boot_config = {
	.root = "9p:trans=virtio,server=sysroot,aname=/"
};

static void
common_core_init(kcpu_t *cpu, kthread_t *thread, struct limine_smp_info *smpi)
{
	char *name;

	plat_common_core_early_init(cpu, thread, smpi);
	ki_cpu_init(cpu, thread);

	/* guard allocations */
	ke_spinlock_acquire_nospl(&early_lock);
	kmem_asprintf(&name, "idle thread *cpu %d)", cpu->num);
	ki_thread_common_init(thread, cpu, &kernel_process->kprocess, name);
	ke_spinlock_release_nospl(&early_lock);
	thread->state = kThreadStateRunning;

	plat_common_core_late_init(cpu, thread, smpi);

	__atomic_add_fetch(&cpus_up, 1, __ATOMIC_RELAXED);
}

#if SMP
static void
ap_init(struct limine_smp_info *smpi)
{
	kcpu_t *cpu = (kcpu_t *)smpi->extra_argument;
	plat_ap_early_init(cpu, smpi);
	common_core_init(cpu, cpu->curthread, smpi);
	/* this is now that CPU's idle thread loop */
	hcf();
}
#endif

#if defined(__amd64__)
#define SMPR_BSP_ID bsp_lapic_id
#define SMPI_ID lapic_id
#define KCPU_ID lapic_id
#elif defined(__aarch64__)
#define SMPR_BSP_ID bsp_mpidr
#define SMPI_ID mpidr
#elif defined (__riscv)
#define SMPR_BSP_ID bsp_hartid
#define SMPI_ID hartid
#endif

static void
smp_allocate(void)
{
	struct limine_smp_response *smpr = smp_request.response;
	ncpus = smpr->cpu_count;

	cpus = kmem_alloc(sizeof(kcpu_t *) * ncpus);

	kprintf("smp_allocate: %zu cpus\n", ncpus);
	kassert(ncpus <= 64);

#if !defined(__m68k__)
	for (size_t i = 0; i < ncpus; i++) {
		struct limine_smp_info *smpi = smpr->cpus[i];

/* uncomment to trace SMP ID, whatever that is */
#if 0
		kprintf("%zu: SMPI ID %zx\n", i, (uintptr_t)smpi->SMPI_ID);
#endif

		if (smpi->SMPI_ID == smpr->SMPR_BSP_ID) {
			smpi->extra_argument = (uint64_t)&bootstrap_cpu;
			bootstrap_cpu.num = i;
			cpus[i] = &bootstrap_cpu;
			bootstrap_cpu.cpucb.SMPI_ID = smpi->SMPI_ID;
		} else {
			kcpu_t *cpu = kmem_alloc(sizeof(kcpu_t));
			kthread_t *thread = kmem_alloc(sizeof(kthread_t));

			cpu->num = i;
			cpu->curthread = thread;
			cpu->cpucb.SMPI_ID = smpi->SMPI_ID;
			cpu->local_data = kmem_alloc(sizeof(kcpu_local_data_t));
			cpu->local_data->curthread = thread;
			cpu->local_data->cpu = cpu;
			cpu->local_data->cpu_num = i;
			cpus[i] = cpu;
			thread->last_cpu = cpu;
		}
	}
#endif
}

static void
smp_start()
{
#if !defined(__m68k__)
	struct limine_smp_response *smpr = smp_request.response;

	kprintf("smp_start: starting %zu APs\n", ncpus - 1);

	for (size_t i = 0; i < ncpus; i++) {
		struct limine_smp_info *smpi = smpr->cpus[i];

		if (smpi->SMPI_ID == smpr->SMPR_BSP_ID) {
			smpi->extra_argument = (uint64_t)&bootstrap_cpu;
			common_core_init(&bootstrap_cpu, &thread0, smpi);
		} else {
			kcpu_t *cpu = cpus[i];
			smpi->extra_argument = (uint64_t)cpu;
			smpi->goto_address = ap_init;
		}
	}
#else
	bootstrap_cpu.num = 0;
	cpus[0] = &bootstrap_cpu;
	common_core_init(&bootstrap_cpu, &thread0, NULL);
#endif

	while (__atomic_load_n(&cpus_up, __ATOMIC_RELAXED) != ncpus)
		;

	kprintf("smp_start: all cores up\n");

	idle_mask = (1 << ncpus) - 1;
}

#define MSG                                                                    \
	"\n"                                                                   \
	"------------------------------------------------------------------\n" \
	"                      Keyronex Operating System                   \n" \
	"------------------------------------------------------------------\n\n"

#if defined(__m68k__)
#define ARCH_NAME "m68k"
#elif defined(__amd64__)
#define ARCH_NAME "amd64"
#elif defined(__aarch64__)
#define ARCH_NAME "aarch64"
#elif defined(__riscv)
#define ARCH_NAME "riscv64"
#endif

// The following will be our kernel's entry point.
// If renaming _start() to something else, make sure to change the
// linker script accordingly.

void
_start(void)
{
	plat_first_init();
	bootstrap_cpu.local_data = &bootstrap_cpu_local_data;

	npf_pprintf(pac_putc, NULL, MSG);

	npf_pprintf(pac_putc, NULL,
	    "Keyronex/" ARCH_NAME " (" __DATE__ " " __TIME__ ")\r\n");

	/* set up initial threading structures */

	ki_cpu_init(&bootstrap_cpu, &thread0);
	thread0.last_cpu = &bootstrap_cpu;
	thread0.state = kThreadStateRunning;
	thread0.timeslice = 5;
	ki_thread_common_init(&thread0, &bootstrap_cpu,
	    &kernel_process->kprocess, "idle0");

	vmp_pmm_init();
	vmp_kernel_init();
	kmem_init();
	obj_init();
	ps_early_init(&thread0);
	smp_allocate();
	kmem_postsmp_init();
	ddk_init();
	plat_pre_smp_init();
	smp_start();
#if 0
	ntcompat_init();
#endif

	ps_create_kernel_thread(&ex_init_thread, "ex_init", ex_init, NULL);
	ke_thread_resume(ex_init_thread);

	/* idle loop */
#if defined (__m68k__)
	for (;;) {
		asm("stop #0x2000");
	}
#else
	hcf();
#endif
}
