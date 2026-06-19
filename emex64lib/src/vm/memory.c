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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include <emex64lib/support/diagnostic/legacy.h>
#include <emex64lib/support/likely.h>
#include <emex64lib/support/bitwalker.h>

#include <emex64lib/vm/memory.h>
#include <emex64lib/vm/core.h>
#include <emex64lib/vm/machine.h>
#include <emex64lib/vm/mmio.h>

typedef struct emex64_mmu_entry_lookup {
    bool fail;
    uint64_t *pte;
} emex64_mmu_entry_lookup_t;

static inline emex64_mmu_entry_lookup_t emex64_mmu_lookup_pte(emex64_core_t *core,
                                                              uint64_t pt_addr,
                                                              uint16_t idx)
{
    /*
     * bounds check pt_addr and check if it
     * can be even a table.
     */
    pt_addr = EMEX64_PAGE_ROUND_DOWN(pt_addr);
    if(unlikely(!EMEX64_IN_PHYS_MEMORY(pt_addr, EMEX64_PAGE_SIZE, core->machine->memory->memory, core->machine->memory->memory_size)))
    {
        return (emex64_mmu_entry_lookup_t){ .fail = true, .pte = NULL };
    }

    /* now access the table and check its entry too */
    uint64_t *pt = (uint64_t*)&core->machine->memory->memory[pt_addr];
    uint64_t *pte = &pt[idx];

    if(unlikely(!((*pte & EMEX64_MEMORY_MMU_MASK_FLAGS) & kEmex64MMUPTPresent)))
    {
        return (emex64_mmu_entry_lookup_t){ .fail = true, .pte = NULL };
    }

    return (emex64_mmu_entry_lookup_t){ .fail = false, .pte = pte };
}

static inline bool emex64_mmu_access_pxd(emex64_core_t *core,
                                         uint64_t pt_addr,
                                         uint16_t pxd_idx,
                                         kEmex64MemoryAction acc,
                                         uint64_t *oaddr)
{
    emex64_mmu_entry_lookup_t lookup = emex64_mmu_lookup_pte(core, pt_addr, pxd_idx);
    if(unlikely(lookup.fail))
    {
        return false;
    }

    uint64_t mmu_flags = 0;
    if(acc != kEmex64MemoryActionPageDirectory)
    {
        uint8_t checkflg = acc;

        /*
         * if CR0 is user then we need to add user
         * check too, otherwise the user program will
         * be able to access kernel memory.
         */
        if(core->cr_state.crel.level < kEmex64ElevationLevelKernel)
        {
            checkflg |= kEmex64MMUPTUser;
        }

        /* initial flag check */
        mmu_flags = (*(lookup.pte) & EMEX64_MEMORY_MMU_MASK_FLAGS);
        if(unlikely((mmu_flags & checkflg) != checkflg))
        {
            return false;
        }
    }

    uint64_t pfn = (*(lookup.pte) & EMEX64_MEMORY_MMU_MASK_PFN) >> 8;
    uint64_t physaddr = EMEX64_PAGE_ROUND_DOWN(pfn << 13);
    if(unlikely(!EMEX64_IN_PHYS_MEMORY(physaddr, EMEX64_PAGE_SIZE, core->machine->memory->memory, core->machine->memory->memory_size)))
    {
        return false;
    }

    switch(acc)
    {
        case kEmex64MemoryActionPageDirectory:
            /* not a normal page access */
            break;
        case kEmex64MemoryActionWrite:
            mmu_flags |= kEmex64MMUPTDirty;
            /* fallthrough */
        case kEmex64MemoryActionRead:
        case kEmex64MemoryActionExecute:
            mmu_flags |= kEmex64MMUPTAccessed;
            *(lookup.pte) = (*(lookup.pte) & ~EMEX64_MEMORY_MMU_MASK_FLAGS) | mmu_flags;
    }

    *oaddr = physaddr;

    return true;
}

static inline bool emex64_mmu_translate(emex64_core_t *core,
                                        uint64_t vaddr,
                                        kEmex64MemoryAction action,
                                        uint64_t *paddr)
{
    /*
     * getting page global directory from physical frame number
     * stored in the 5th level (yk the control register x3).
     */
    uint64_t pgd_addr = core->cr_state.crptb.pgd_addr;

    /* still unknown page directory addresses */
    uint64_t pud_addr, pmd_addr, pte_addr, phys_page_base_addr;

    /* now access each table */
    if(!emex64_mmu_access_pxd(core, pgd_addr, ((vaddr >> 43) & 0x3FF), kEmex64MemoryActionPageDirectory, &pud_addr) ||   /* 10 bits for each level index  */
       !emex64_mmu_access_pxd(core, pud_addr, ((vaddr >> 33) & 0x3FF), kEmex64MemoryActionPageDirectory, &pmd_addr) ||
       !emex64_mmu_access_pxd(core, pmd_addr, ((vaddr >> 23) & 0x3FF), kEmex64MemoryActionPageDirectory, &pte_addr) ||
       !emex64_mmu_access_pxd(core, pte_addr, ((vaddr >> 13) & 0x3FF), action, &phys_page_base_addr))
    {
        return false;
    }

    *paddr = phys_page_base_addr + (vaddr & 0x1FFF); /* 13bit offset (addressing within a page) */

    return true;
}

emex64_memory_t *emex64_memory_alloc(uint64_t size)
{
    /*
     * allocating random access memory, which
     * must be aligned to page size for the
     * sake of god. And because it makes sense
     * lol.
     */
    emex64_memory_t *memory = malloc(sizeof(emex64_memory_t));
    if(memory == NULL)
    {
        return NULL;
    }

    /* allocate raw memory (using mmap for larger sizes, better than malloc in this case) */
    memory->memory_size = EMEX64_PAGE_ROUND_UP(size);
    memory->memory = mmap(NULL, memory->memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(memory->memory == MAP_FAILED)
    {
        free(memory);
        return NULL;
    }

    return memory;
}

void emex64_memory_dealloc(emex64_memory_t *memory)
{
    munmap(memory->memory, memory->memory_size);
    free(memory);
}

bool emex64_memory_load_image(emex64_memory_t *memory,
                              emex_file_t *file)
{
    vfd_t *d = emex_file_dup_fd(file);
    if(d == NULL)
    {
        diag_fatal(NULL, "failed to dup virtual file descriptor from file\n");
        return false;
    }

    struct stat image_stat;
    if(vfd_stat(d, &image_stat) != 0)
    {
        vfd_close(d);
        diag_fatal(NULL, "failed to gather size of file at path '%s'\n", file->path);
        return false;
    }

    size_t image_size = image_stat.st_size;
    vfd_close(d);
    if(image_size > memory->memory_size)
    {
        diag_error(NULL, "firmware image is too large\n");
        return false;
    }

    /*
     * overmap the memory with the file in a dirty way tehe ^^
     * meaning that when ever the vm writes to this memory
     * it will become writable as the OS then copies the memory
     * to a writable page, this is much faster than copying it
     * our selves.
     */
    if(!emex_file_map(file))
    {
        diag_error(NULL, "mapping firmware image failed\n");
        return false;
    }

    ssize_t s = vpage_read(file->vo->root, 0, memory->memory, image_size);

    if((size_t)s < image_size)
    {
        diag_error(NULL, "mapping boot image failed\n");
        return false;
    }

    return true;
}

void emex64_memory_action(emex64_core_t *core,
                          uint64_t addr,
                          size_t size,
                          uint64_t *value,
                          kEmex64MemoryAction action)
{
    if(unlikely((core->cr_state.crexc.exception == kEmex64ExceptionBadAccess || core->cr_state.crexc.exception == kEmex64ExceptionKTRRViolation) && !core->in_interrupt))
    {
        return;
    }

    /*
     * MMIO starts at 0x0020000000000000 while the physical
     * maximum memory size is 0x001FFFFFFFFFFFFF, that is so
     * MMIO doesnt sit in middle of the memory, which is better
     * for page memory management on the OS side and faster cuz
     * we don't have to look it up in MMIO on every memory
     * access.
     */
    if(addr >> 53)
    {
        /*
         * MMIO shall never be accessed when paging is enabled or when
         * the access is not aligned to a quad word boundary, as
         * the decoder later in actual emex64 hardware would explode
         * in size otherwise. MMIO busses are complex!
         */
        if(unlikely((core->cr_state.crptb.enabled && !core->in_interrupt) || (!EMEX64_IS_ALIGNED_64(addr) && addr < EMEX64_FB_BASE)))
        {
            core->cr_state.crexc.exception = kEmex64ExceptionBadAccess;
            return;
        }

        emex64_mmio_region_t *mmio_region = emex64_mmio_find(core->machine->mmio_bus, addr);
        if(likely(mmio_region != NULL))
        {
            uint64_t offset = addr - mmio_region->base_addr;
            switch(action)
            {
                case kEmex64MemoryActionRead:
                    *value = mmio_region->read(core, mmio_region->device, offset, (int)size);
                    return;
                case kEmex64MemoryActionWrite:
                    mmio_region->write(core, mmio_region->device, offset, *value, (int)size);
                    return;
                default:
                    core->cr_state.crexc.exception = kEmex64ExceptionBadAccess;
                    return;
            }
        }
    }

    /*
     * find out if paging is enabled, if not write vaddr to paddr,
     * because that means paddr is vaddr because virtual addressing
     * is already off.
     *
     * we read it as if it was a 5th level entry, but its just a
     * control register.. for simplicity we do that hahaha.
     */
    if(core->cr_state.crptb.enabled && !core->in_interrupt)
    {
        if(!emex64_mmu_translate(core, addr, action, &addr))
        {
            core->cr_state.crexc.exception = kEmex64ExceptionPageFault;
            return;
        }
    }
    else
    {
        goto rw_fastpath;
    }

    uint64_t page_end = (addr & ~EMEX64_PAGE_MASK) + EMEX64_PAGE_SIZE;
    size_t lo_size = (size_t)(page_end - addr);

    if(lo_size < size)
    {
        size_t hi_size = size - lo_size;
        uint64_t hi_shift = lo_size * 8;
        uint64_t lo_val, hi_val, lo_mask;

        switch(action)
        {
            case kEmex64MemoryActionPageDirectory:
            case kEmex64MemoryActionExecute:
            case kEmex64MemoryActionRead:
                emex64_memory_action(core, addr, lo_size, &lo_val, action);
                emex64_memory_action(core, page_end, hi_size, &hi_val, action);
                *value = lo_val | (hi_val << hi_shift);
                return;
            case kEmex64MemoryActionWrite:
                lo_mask = (lo_size == 8) ? ~0ULL : (1ULL << hi_shift) - 1;
                lo_val = *value & lo_mask;
                hi_val = *value >> hi_shift;
                emex64_memory_action(core, addr, lo_size, &lo_val, action);
                emex64_memory_action(core, page_end, hi_size, &hi_val, action);
                return;
        }
        return;
    }
    else
rw_fastpath:
    {
        if(likely(!emex64_memory_access(core, addr, size)))
        {
            core->cr_state.crexc.exception = kEmex64ExceptionBadAccess;
            return;
        }

        uint8_t *mem_ptr = core->machine->memory->memory + addr;

        switch(action)
        {
            case kEmex64MemoryActionPageDirectory:
            case kEmex64MemoryActionExecute:
            case kEmex64MemoryActionRead:
                switch(size)
                {
                    case 1:
                        *value = *(uint8_t *)mem_ptr;
                        break;
                    case 2:
                    {
                        uint16_t tmp;
                        memcpy(&tmp, mem_ptr, 2);
                        *value = TO_HOST16(tmp);
                        break;
                    }
                    case 4:
                    {
                        uint32_t tmp;
                        memcpy(&tmp, mem_ptr, 4);
                        *value = TO_HOST32(tmp);
                        break;
                    }
                    case 8:
                    {
                        uint64_t tmp;
                        memcpy(&tmp, mem_ptr, 8);
                        *value = TO_HOST64(tmp);
                        break;
                    }
                    default:
                        core->cr_state.crexc.exception = kEmex64ExceptionBadAccess; 
                        return;
                }
                return;
            case kEmex64MemoryActionWrite:
                if(unlikely(core->machine->memory->ktrr_size > addr))
                {
                    core->cr_state.crexc.exception = kEmex64ExceptionKTRRViolation;
                    return;
                }
                switch(size)
                {
                    case 1:
                        *(uint8_t *)mem_ptr = (uint8_t)*value;
                        break;
                    case 2:
                    {
                        uint16_t tmp = (uint16_t)*value;
                        tmp = TO_HOST16(tmp);
                        memcpy(mem_ptr, &tmp, 2);
                        break;
                    }
                    case 4:
                    {
                        uint32_t tmp = (uint32_t)*value;
                        tmp = TO_HOST32(tmp);
                        memcpy(mem_ptr, &tmp, 4);
                        break;
                    }
                    case 8:
                    {
                        uint64_t tmp = (uint64_t)*value;
                        tmp = TO_HOST64(tmp);
                        memcpy(mem_ptr, &tmp, 8);
                        break;
                    }
                    default:
                        core->cr_state.crexc.exception = kEmex64ExceptionBadAccess; 
                        return;
                }
                return;
        }
    }
}

bool emex64_memory_cpy(emex64_core_t *core,
                       uint8_t *dst,
                       uint64_t addr,
                       size_t len,
                       kEmex64MemoryAction action)
{
    /* do not allow other actions than rx */
    assert(action != kEmex64MemoryActionWrite);

    if(unlikely((core->cr_state.crexc.exception == kEmex64ExceptionBadAccess || core->cr_state.crexc.exception == kEmex64ExceptionKTRRViolation) && !core->in_interrupt))
    {
        return false;
    }

    /* there will never be a buffer copy out on the MMIO regions */
    if(unlikely((addr >> 53) || ((addr + len - 1) >> 53)))
    {
        core->cr_state.crexc.exception = kEmex64ExceptionBadAccess;
        return false;
    }

    bool paging = core->cr_state.crptb.enabled && !core->in_interrupt;

    /* walking the MMU once per page */
    while(len > 0)
    {
        uint64_t paddr = addr;
        size_t chunk = len;

        if(paging)
        {
            if(unlikely(!emex64_mmu_translate(core, addr, action, &paddr)))
            {
                core->cr_state.crexc.exception = kEmex64ExceptionPageFault;
                return false;
            }

            size_t page_left = (size_t)(EMEX64_PAGE_SIZE - (addr & EMEX64_PAGE_MASK));
            if(chunk > page_left)
            {
                chunk = page_left;
            }
        }

        if(unlikely(!emex64_memory_access(core, paddr, chunk)))
        {
            core->cr_state.crexc.exception = kEmex64ExceptionBadAccess;
            return false;
        }

        /*
         * only a kernel level core may execute kernel space
         * code when KTRR is locked.
         *
         * fixme: causes SIGBUS
         */
        /*if(action == kEmex64MemoryActionExecute)
        {
            if(unlikely(core->rl[kEmex64RegisterCR0] >= kEmex64ElevationLevelKernel && core->machine->memory->ktrr_locked && core->machine->memory->ktrr_size < (addr + len)))
            {
                chunk = (size_t)(core->machine->memory->ktrr_size - addr);
            }
        }*/

        memcpy(dst, &core->machine->memory->memory[paddr], chunk);

        dst += chunk;
        addr += chunk;
        len -= chunk;
    }

    return true;
}
