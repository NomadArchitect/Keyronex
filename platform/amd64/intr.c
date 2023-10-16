#include "asmintr.h"
#include "intr.h"
#include "kdk/amd64.h"
#include "kdk/amd64/regs.h"
#include "kdk/nanokern.h"
#include "kdk/vm.h"
#include "nanokern/ki.h"

typedef struct {
	uint16_t isr_low;
	uint16_t selector;
	uint8_t ist;
	uint8_t type;
	uint16_t isr_mid;
	uint32_t isr_high;
	uint32_t zero;
} __attribute__((packed)) idt_entry_t;

static TAILQ_HEAD(intr_entries, intr_entry) intr_entries[256];
static idt_entry_t idt[256] = { 0 };

ipl_t
splraise(ipl_t ipl)
{
	ipl_t oldipl = read_cr8();
	kassert(oldipl <= ipl);
	write_cr8(ipl);
	return oldipl;
}

void
splx(ipl_t ipl)
{
	kassert(ipl <= splget());
	write_cr8(ipl);
}

ipl_t
splget(void)
{
	return read_cr8();
}

static void
idt_set(uint8_t index, vaddr_t isr, uint8_t type, uint8_t ist)
{
	idt[index].isr_low = (uint64_t)isr & 0xFFFF;
	idt[index].isr_mid = ((uint64_t)isr >> 16) & 0xFFFF;
	idt[index].isr_high = (uint64_t)isr >> 32;
	idt[index].selector = 0x28; /* sixth */
	idt[index].type = type;
	idt[index].ist = ist;
	idt[index].zero = 0x0;
}

void
idt_load(void)
{
	struct {
		uint16_t limit;
		vaddr_t addr;
	} __attribute__((packed)) idtr = { sizeof(idt) - 1, (vaddr_t)idt };

	asm volatile("lidt %0" : : "m"(idtr));
}

/* setup and load initial IDT */
void
intr_init(void)
{
#define IDT_SET(VAL) idt_set(VAL, (vaddr_t)&isr_thunk_##VAL, INT, 0);
	NORMAL_INTS(IDT_SET);
#undef IDT_SET

#define IDT_SET(VAL, GATE) idt_set(VAL, (vaddr_t)&isr_thunk_##VAL, GATE, 0);
	SPECIAL_INTS(IDT_SET);
#undef IDT_SET

	idt_load();

	for (int i = 0; i < elementsof(intr_entries); i++) {
		TAILQ_INIT(&intr_entries[i]);
	}
}

void
handle_int(md_intr_frame_t *frame, uintptr_t num)
{
	ipl_t ipl = splget(), new_ipl = num >> 4;
	struct intr_entries *entries;
	struct intr_entry *entry;

	if (splget() >new_ipl)
		kfatal("IPL not less or equal\n");
	write_cr8(new_ipl);
	asm("sti");

	switch (num) {
	case 14:
		kfatal("Unhandled page fault at IP 0x%lx: address 0x%lx\n",
		    frame->rip, read_cr2());
		break;

	case kIntVecDPC:
		lapic_eoi();
		ki_dispatch_dpcs(curcpu());
		break;

	case kIntVecLAPICTimer:
		ki_cpu_hardclock(frame, curcpu());
		lapic_eoi();
		break;

	default: {
		entries = &intr_entries[num];

		if (TAILQ_EMPTY(entries)) {
			kprintf("Unhandled interrupt %lu. CR2: 0x%lx\n", num,
			    read_cr2());
		}

		TAILQ_FOREACH (entry, entries, queue_entry) {
			new_ipl = MAX2(new_ipl, entry->ipl);
		}

		if (new_ipl < splget()) {
			kprintf(
			    "In handling interrupt %lu (cr2: 0x%lx):\n"
			    "IPL not less or equal (running at %d, interrupt priority %d)\n",
			    num, read_cr2(), splget(), new_ipl);
		}

		splraise(new_ipl);

		TAILQ_FOREACH (entry, entries, queue_entry) {
			bool r = entry->handler(frame, entry->arg);
			(void)r;
		}

		if (num >= 32)
			lapic_eoi();
	}
	}

	asm("cli");
	write_cr8(ipl);
}

int
md_intr_alloc(const char *name, ipl_t prio, intr_handler_t handler, void *arg,
    bool shareable, uint8_t *vector, struct intr_entry *entry)
{
	/* first vector appropriate to the priority */
	uint8_t starting = MAX2(prio << 4, 32);

	for (int i = starting; i < starting + 16; i++) {
		struct intr_entry *slot = TAILQ_FIRST(&intr_entries[i]);
		if (slot == NULL || (slot->shareable && shareable)) {
			md_intr_register(name, i, prio, handler, arg, shareable,
			    entry);
			if (vector)
				*vector = i;
			return 0;
		}
	}

	return -1;
}

void
md_intr_register(const char *name, uint8_t vec, ipl_t prio,
    intr_handler_t handler, void *arg, bool shareable, struct intr_entry *entry)
{
	entry->name = name;
	entry->ipl = prio;
	entry->handler = handler;
	entry->arg = arg;
	entry->shareable = shareable;
	TAILQ_INSERT_TAIL(&intr_entries[vec], entry, queue_entry);
}