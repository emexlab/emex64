/*
 * MIT License
 *
 * Copyright (c) 2026 cr4zyengineer
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

static bool la64_mmu_access_ctable(la64_core_t *core,
                                   uint64_t ptbase,
                                   uint16_t idx,
                                   uint64_t *oaddr)
{
    la64_mmu_entry_t *table = (la64_mmu_entry_t *)&core->machine->memory->memory[ptbase];
    la64_mmu_entry_t entry = table[idx];

    la64_mmu_flag_t flag = entry & LA64_MMU_MASK_FLAGS;

    if(!(flag & LA64_MMU_PT_PRESENT))
    {
        return false;
    }

    la64_mmu_pfn_t pfn = (entry & LA64_MMU_MASK_PFN) >> 8;

    *oaddr = pfn << 13;

    return true;
}

static bool la64_mmu_access_l1(la64_core_t *core,
                               uint64_t ptbase,
                               uint16_t idx,
                               uint8_t acc,
                               uint64_t *oaddr)
{
    la64_mmu_entry_t *table = (la64_mmu_entry_t *)&core->machine->memory->memory[ptbase];
    la64_mmu_entry_t entry = table[idx];

    la64_mmu_flag_t flag = entry & LA64_MMU_MASK_FLAGS;
    la64_mmu_flag_t checkflg = 0;

    /* permission switch */
    switch(acc)
    {
        case LA64_MMU_ACC_READ:
            checkflg = LA64_MMU_PT_READ;
            break;
        case LA64_MMU_ACC_WRITE:
            checkflg = LA64_MMU_PT_WRITE;
            break;
        case LA64_MMU_ACC_EXEC:
            checkflg = LA64_MMU_PT_EXEC;
            break;
        default:
            /* TODO: cause pagefault */
            return false;
    }

    checkflg |= LA64_MMU_PT_PRESENT;

    /* if CR0 is user then we need to add user check too */
    if(core->rl[LA64_REGISTER_CR0] < LA64_ELEVATION_KERNEL)
    {
        checkflg |= LA64_MMU_PT_USER;
    }

    /* action permission check */
    if((flag & checkflg) != checkflg)
    {
        /* TODO: cause page fault */
        return false;
    }

    la64_mmu_pfn_t pfn = (entry & LA64_MMU_MASK_PFN) >> 8;

    *oaddr = pfn << 13;

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
    la64_mmu_entry_t l5_entry = core->rl[LA64_REGISTER_CR4];

    /* gather its flag and check if paging is enabled */
    la64_mmu_flag_t l5_flag = l5_entry & LA64_MMU_MASK_FLAGS;

    if(!(l5_flag & LA64_MMU_PT_PRESENT) ||
       core->in_interrupt)
    {
        /* incase paging is disabled virtual addresses are physical ones */
        *paddr = vaddr;
        return true;
    }

    /* get pfn of control register */
    la64_mmu_pfn_t l5_pfn = (l5_entry & LA64_MMU_MASK_PFN) >> 8;

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
    uint64_t l4_addr = l5_pfn << 13;

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
