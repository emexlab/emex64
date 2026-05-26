/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <emex64vm/mmu.h>
#include <emex64vm/core.h>
#include <emex64vm/machine.h>
#include <stdio.h>

typedef struct la64_mmu_entry_lookup {
    bool fail;
    uint64_t pte;
} la64_mmu_entry_lookup_t;

static la64_mmu_entry_lookup_t la64_mmu_lookup_pte(la64_core_t *core,
                                                   uint64_t pt_addr,
                                                   uint16_t idx)
{
    la64_mmu_entry_lookup_t lookup = {};

    /*
     * bounds check ptbase and check if it
     * can be even a table.
     */
    pt_addr = LA64_PAGE_ROUND_DOWN(pt_addr);
    if(!LA64_IN_PHYS_MEMORY(pt_addr, LA64_PAGE_SIZE, core->machine->memory->memory, core->machine->memory->memory_size))
    {
        lookup.fail = true;
        return lookup;
    }

    /* now access the table and check its entry too */
    uint64_t *pt = (uint64_t*)&core->machine->memory->memory[pt_addr];
    lookup.pte = pt[idx];

    if(!((lookup.pte & LA64_MMU_MASK_FLAGS) & LA64_MMU_PT_PRESENT))
    {
        lookup.fail = true;
        return lookup;
    }

    return lookup;
}

static bool la64_mmu_access_ctable(la64_core_t *core,
                                   uint64_t ptbase,
                                   uint16_t idx,
                                   uint64_t *oaddr)
{
    la64_mmu_entry_lookup_t lookup = la64_mmu_lookup_pte(core, ptbase, idx);
    if(lookup.fail)
    {
        return false;
    }

    uint64_t pfn = (lookup.pte & LA64_MMU_MASK_PFN) >> 8;
    uint64_t physaddr = LA64_PAGE_ROUND_DOWN(pfn << 13);
    if(!LA64_IN_PHYS_MEMORY(physaddr, LA64_PAGE_SIZE, core->machine->memory->memory, core->machine->memory->memory_size))
    {
        return false;
    }

    *oaddr = physaddr;

    return true;
}

static bool la64_mmu_access_l1(la64_core_t *core,
                               uint64_t ptbase,
                               uint16_t idx,
                               uint8_t acc,
                               uint64_t *oaddr)
{
    la64_mmu_entry_lookup_t lookup = la64_mmu_lookup_pte(core, ptbase, idx);
    if(lookup.fail)
    {
        return false;
    }

    uint8_t checkflg = acc;

    /*
     * if CR0 is user then we need to add user
     * check too, otherwise the user program will
     * be able to access kernel memory.
     */
    if(core->rl[LA64_REGISTER_CR0] < LA64_ELEVATION_KERNEL)
    {
        checkflg |= LA64_MMU_PT_USER;
    }

    /* initial flag check */
    if(((lookup.pte & LA64_MMU_MASK_FLAGS) & checkflg) != checkflg)
    {
        /* TODO: cause page fault */
        return false;
    }

    uint64_t pfn = (lookup.pte & LA64_MMU_MASK_PFN) >> 8;
    uint64_t physaddr = LA64_PAGE_ROUND_DOWN(pfn << 13);
    if(!LA64_IN_PHYS_MEMORY(physaddr, LA64_PAGE_SIZE, core->machine->memory->memory, core->machine->memory->memory_size))
    {
        return false;
    }

    *oaddr = physaddr;

    return true;
}

bool la64_mmu_access(la64_core_t *core,
                     uint64_t vaddr,
                     uint8_t acc,
                     uint64_t *paddr)
{
    /* vaddr cannot be bigger than 53bits */
    if(vaddr >> 53)
    {
        return false;
    }

    /*
     * find out if paging is enabled, if not write vaddr to paddr,
     * because that means paddr is vaddr because virtual addressing
     * is already off.
     *
     * we read it as if it was a 5th level entry, but its just a
     * control register.. for simplicity we do that hahaha.
     */
    uint64_t cr_pte = core->rl[LA64_REGISTER_CR4];
    if(!((cr_pte & LA64_MMU_MASK_FLAGS) & LA64_MMU_PT_PRESENT) || core->in_interrupt)
    {
        /* incase paging is disabled virtual addresses are physical ones */
        *paddr = vaddr;
        return true;
    }

    /* get pfn of control register */
    uint64_t cr_pfn = (cr_pte & LA64_MMU_MASK_PFN) >> 8;

    /* precalculating all indexes */
    uint16_t offset =  vaddr        & 0x1FFF;      /* 13bit offset (addressing within a page) */
    uint16_t l1_idx = (vaddr >> 13) & 0x3FF;       /* 10 bits for each index  */
    uint16_t l2_idx = (vaddr >> 23) & 0x3FF;
    uint16_t l3_idx = (vaddr >> 33) & 0x3FF;
    uint16_t l4_idx = (vaddr >> 43) & 0x3FF;

    /* now we calculate the address where the physical frame is */
    uint64_t l1_addr = 0;
    uint64_t l2_addr = 0;
    uint64_t l3_addr = 0;
    uint64_t l4_addr = cr_pfn << 13;

    /* now access each table */
    if(!la64_mmu_access_ctable(core, l4_addr, l4_idx, &l3_addr) ||
       !la64_mmu_access_ctable(core, l3_addr, l3_idx, &l2_addr) ||
       !la64_mmu_access_ctable(core, l2_addr, l2_idx, &l1_addr))
    {
        return false;
    }

    /*
     * now were at the l1 table, now things get very interesting
     * we need to extract the flags now and such..
     */
    uint64_t paddr_raw = 0;
    if(!la64_mmu_access_l1(core, l1_addr, l1_idx, acc, &paddr_raw))
    {
        return false;
    }

    *paddr = paddr_raw + offset;

    return true;
}
