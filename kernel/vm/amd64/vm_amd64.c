#include "kdk/amd64.h"
#include "kdk/amd64/regs.h"
#include "kdk/executive.h"
#include "kdk/libkern.h"
#include "kdk/vm.h"
#include "vm/amd64/vmp_amd64.h"
#include "vm/vmp.h"

typedef uint64_t pml4e_t, pdpte_t, pde_t;

extern vm_procstate_t kernel_procstate;

void
vmp_md_ps_init(eprocess_t *ps)
{
	vm_procstate_t *vmps = ps->vm;
	vm_page_t *table_page;
	paddr_t table_addr;
	pte_t *table_ptebase;

	vm_page_alloc(&table_page, 0, kPageUsePML4, true);
	table_page->process = ps;
	table_addr = vm_page_paddr(table_page);
	vmps->md.table = table_addr;

	if (vmps == &kernel_procstate) {
		memcpy((void *)P2V(table_addr), (void *)P2V(read_cr3()),
		    PGSIZE);

#if 1
		/* preallocate higher half entries */
		table_ptebase = (pte_t *)P2V(table_addr);
		for (int i = 256; i < 512; i++) {
			pte_t *pte = &table_ptebase[i];
			if (pte->value == 0) {
				vm_page_t *pml3_page;

				vm_page_alloc(&pml3_page, 0, kPageUsePML3,
				    true);
				pml3_page->process = ps;
				pml3_page->nonzero_ptes = 10000;
				pml3_page->noswap_ptes = 10000;
				pml3_page->refcnt = 10000;
				vmp_md_pte_create_hw(pte,
				    vm_page_pfn(pml3_page), true, true, true,
				    false);
			}
		}
#endif

		write_cr3(table_addr);
	} else {
		memcpy((void *)P2V(table_addr + (PGSIZE / 2)),
		    (void *)P2V(kernel_procstate.md.table + (PGSIZE / 2)),
		    (PGSIZE / 2));
	}
}

paddr_t
vmp_md_translate(vaddr_t addr)
{
	pte_t *pte;
	paddr_t paddr;

	if (addr >= HHDM_BASE && addr <= HHDM_BASE + HHDM_SIZE)
		paddr = V2P(addr);
	else {
		int r;

		r = vmp_fetch_pte(kernel_process->vm, PGROUNDDOWN(addr), &pte);
		kassert(r == 0);
		paddr = vmp_pte_hw_paddr(pte, 1);
		paddr += addr % PGSIZE;
	}

	return paddr;
}

void vmp_md_setup_table_pointers(vm_procstate_t *ps, vm_page_t *dirpage,
    vm_page_t *tablepage, pte_t *dirpte, enum vmp_table_old_state old_state)
{
	pte_t pte;
	pte.value = 0x0;
	pte.hw.valid = 1;
	pte.hw.writeable = 1;
	pte.hw.user = 1;
	pte.hw.pfn = vm_page_pfn(tablepage);
	dirpte->value = pte.value;

	if (old_state != kWasTrans)
		vmp_pagetable_page_noswap_pte_created(ps, dirpage, old_state == kWasZero ? true : false);
}

void vmp_md_busy_table_pointers(vm_procstate_t *ps, vm_page_t *dirpage,
    pte_t *dirpte, vm_page_t *tablepage)
{
	vmp_md_pte_create_busy(dirpte, vm_page_pfn(tablepage));
	vmp_pagetable_page_noswap_pte_created(ps, dirpage, true);
}

void
vmp_md_trans_table_pointers(vm_procstate_t *ps, vm_page_t *dirpage,
    pte_t *dirpte, vm_page_t *tablepage)
{
	vmp_md_pte_create_trans(dirpte, vm_page_pfn(tablepage));
}

void
vmp_md_swap_table_pointers(vm_procstate_t *ps, vm_page_t *dirpage,
    pte_t *dirpte, uintptr_t drumslot)
{
	vmp_md_pte_create_swap(dirpte, drumslot);
	vmp_pagetable_page_pte_became_swap(ps, dirpage);
}


void
vmp_md_delete_table_pointers(vm_procstate_t *ps, vm_page_t *dirpage, pte_t *dirpte)
{
	dirpte->hw.value = 0x0;
	vmp_pagetable_page_pte_deleted(ps, dirpage, false);
}
