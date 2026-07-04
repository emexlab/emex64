/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <EmexToolchain/support/diagnostic/log.h>
#include <EmexToolchain/support/likely.h>
#include <EmexToolchain/vm/E64Memory.h>
#include <EmexToolchain/vm/core.h>
#include <EmexToolchain/vm/machine.h>
#include <EmexToolchain/vm/E64MMIO.h>

typedef struct E64Memory {
    EFObject header;
    uint8_t *memory;
    uint64_t memory_size;
    uint64_t ktrr_size;
    bool ktrr_locked;
} *E64Memory;

static void __E64MemoryDeinit(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64Memory)memoryRef;
    if(memory->memory != MAP_FAILED)
    {
        munmap(memory->memory, memory->memory_size);
    }
}

static EFClass E64MemoryClass = {
    .name = "E64Memory",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __E64MemoryDeinit,
    .equal = NULL,
    .copyDescription = NULL,
};

typedef struct emex64_mmu_entry_lookup {
    bool fail;
    uint64_t *pte;
} emex64_mmu_entry_lookup_t;

static void E64MemoryRegisterClass(void)
{
    EFClassRegister(&E64MemoryClass);
}

EFTypeID E64MemoryGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, E64MemoryRegisterClass);
    return E64MemoryClass.typeID;
}

E64MemoryRef E64MemoryCreate(EFAllocatorRef allocatorRef,
                                   uint64_t size)
{
    E64Memory memory = EFObjectAlloc(allocatorRef, E64MemoryGetTypeID(), sizeof(struct E64Memory));
    if(memory == NULL)
    {
        return NULL;
    }

    /* allocate raw memory (using mmap for larger sizes, better than malloc in this case) */
    memory->memory_size = EMEX64_PAGE_ROUND_UP(size);
    memory->memory = mmap(NULL, memory->memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(memory->memory == MAP_FAILED)
    {
        EFRelease(memory);
        return NULL;
    }

    return (E64MemoryRef)memory;
}

void E64MemoryLockKTRR(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return;
    }

    memory->ktrr_locked = true;
}

bool E64MemoryIsKTRRLocked(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return false;
    }

    return memory->ktrr_locked;
}

uint64_t E64MemoryGetKTRRSize(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return 0;
    }

    return memory->ktrr_size;
}

bool E64MemorySetKTRRSize(E64MemoryRef memoryRef,
                             uint64_t size)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL || memory->ktrr_locked)
    {
        return false;
    }

    memory->ktrr_size = size;
    return true;
}

uint64_t E64MemoryGetSize(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return 0;
    }

    return memory->memory_size;
}

bool E64MemoryAccessIsWithinBounds(E64MemoryRef memoryRef,
                                      uint64_t address,
                                      uint64_t size)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return false;
    }

    uint64_t addr_end = address + size;
    if(address > addr_end || memory->memory_size < addr_end)
    {
        return false;
    }
    return true;
}

static inline emex64_mmu_entry_lookup_t emex64_mmu_lookup_pte(E64Memory memory,
                                                              emex64_core_t *core,
                                                              uint64_t pt_addr,
                                                              uint16_t idx)
{
    /*
     * bounds check pt_addr and check if it
     * can be even a table.
     */
    pt_addr = EMEX64_PAGE_ROUND_DOWN(pt_addr);
    if(unlikely(!EMEX64_IN_PHYS_MEMORY(pt_addr, EMEX64_PAGE_SIZE, memory->memory, memory->memory_size)))
    {
        return (emex64_mmu_entry_lookup_t){ .fail = true, .pte = NULL };
    }

    /* now access the table and check its entry too */
    uint64_t *pt = (uint64_t*)&memory->memory[pt_addr];
    uint64_t *pte = &pt[idx];

    if(unlikely(!((*pte & EMEX64_MEMORY_MMU_MASK_FLAGS) & kE64MMUPTPresent)))
    {
        return (emex64_mmu_entry_lookup_t){ .fail = true, .pte = NULL };
    }

    return (emex64_mmu_entry_lookup_t){ .fail = false, .pte = pte };
}

static inline bool emex64_mmu_access_pxd(E64Memory memory,
                                         emex64_core_t *core,
                                         uint64_t pt_addr,
                                         uint16_t pxd_idx,
                                         kE64MemoryAction acc,
                                         uint64_t *oaddr)
{
    emex64_mmu_entry_lookup_t lookup = emex64_mmu_lookup_pte(memory, core, pt_addr, pxd_idx);
    if(unlikely(lookup.fail))
    {
        return false;
    }

    uint64_t mmu_flags = 0;
    if(acc != kE64MemoryActionPageDirectory)
    {
        uint8_t checkflg = acc;

        /*
         * if CR0 is user then we need to add user
         * check too, otherwise the user program will
         * be able to access kernel memory.
         */
        if(core->cr_state.crel.level < kE64ElevationLevelKernel)
        {
            checkflg |= kE64MMUPTUser;
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
    if(unlikely(!EMEX64_IN_PHYS_MEMORY(physaddr, EMEX64_PAGE_SIZE, memory->memory, memory->memory_size)))
    {
        return false;
    }

    switch(acc)
    {
        case kE64MemoryActionPageDirectory:
            /* not a normal page access */
            break;
        case kE64MemoryActionWrite:
            mmu_flags |= kE64MMUPTDirty;
            /* fallthrough */
        case kE64MemoryActionRead:
        case kE64MemoryActionExecute:
            mmu_flags |= kE64MMUPTAccessed;
            *(lookup.pte) = (*(lookup.pte) & ~EMEX64_MEMORY_MMU_MASK_FLAGS) | mmu_flags;
    }

    *oaddr = physaddr;

    return true;
}

static inline bool emex64_mmu_translate(E64Memory memory,
                                        emex64_core_t *core,
                                        uint64_t vaddr,
                                        kE64MemoryAction action,
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
    if(!emex64_mmu_access_pxd(memory, core, pgd_addr, ((vaddr >> 43) & 0x3FF), kE64MemoryActionPageDirectory, &pud_addr) ||   /* 10 bits for each level index  */
       !emex64_mmu_access_pxd(memory, core, pud_addr, ((vaddr >> 33) & 0x3FF), kE64MemoryActionPageDirectory, &pmd_addr) ||
       !emex64_mmu_access_pxd(memory, core, pmd_addr, ((vaddr >> 23) & 0x3FF), kE64MemoryActionPageDirectory, &pte_addr) ||
       !emex64_mmu_access_pxd(memory, core, pte_addr, ((vaddr >> 13) & 0x3FF), action, &phys_page_base_addr))
    {
        return false;
    }

    *paddr = phys_page_base_addr + (vaddr & 0x1FFF); /* 13bit offset (addressing within a page) */

    return true;
}

bool E64MemoryLoadImage(E64MemoryRef memoryRef,
                           emex_file_t *file)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return false;
    }

    vfd_t *d = emex_file_dup_vfd(file);
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

    ssize_t s = VpageObjRead(file->vpageObjRef, 0, memory->memory, image_size);

    if((size_t)s < image_size)
    {
        diag_error(NULL, "mapping boot image failed\n");
        return false;
    }

    return true;
}

bool E64MemoryAction(E64MemoryRef memoryRef,
                        uint64_t addr, size_t size,
                        uint64_t *value,
                        kE64MemoryAction action)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return false;
    }

    if(addr >> 53)
    {
        return false;
    }

    if(unlikely(!E64MemoryAccessIsWithinBounds(memoryRef, addr, size)))
    {
        return false;
    }

    uint8_t *mem_ptr = memory->memory + addr;

    switch(action)
    {
        case kE64MemoryActionPageDirectory:
        case kE64MemoryActionExecute:
        case kE64MemoryActionRead:
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
                    return false;
            }
            return true;
        case kE64MemoryActionWrite:
            if(unlikely(memory->ktrr_size > addr))
            {
                return false;
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
                    return false;
            }
            return true;
        default:
            return false;
    }
}

void E64MemoryCoreAction(E64MemoryRef memoryRef,
                            emex64_core_t *core,
                            uint64_t addr,
                            size_t size,
                            uint64_t *value,
                            kE64MemoryAction action)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return;
    }

    if(unlikely((core->cr_state.crexc.exception == kE64ExceptionBadAccess || core->cr_state.crexc.exception == kE64ExceptionKTRRViolation) && !core->in_interrupt))
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
            core->cr_state.crexc.exception = kE64ExceptionBadAccess;
            return;
        }

        E64MMIORegionRef mmio_region = E64MMIOBusGetRegionForAddress(core->machine->mmio_bus, addr);
        void *device = E64MMIORegionGetDevice(mmio_region);
        if(likely(mmio_region != NULL))
        {
            uint64_t offset = addr - E64MMIORegionGetBaseAddress(mmio_region);
            switch(action)
            {
                case kE64MemoryActionRead:
                    mmio_read_fn read = E64MMIORegionGetReadSymbol(mmio_region);
                    *value = read(core, device, offset, (int)size);
                    return;
                case kE64MemoryActionWrite:
                    mmio_write_fn write = E64MMIORegionGetWriteSymbol(mmio_region);
                    write(core, device, offset, *value, (int)size);
                    return;
                default:
                    core->cr_state.crexc.exception = kE64ExceptionBadAccess;
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
        if(!emex64_mmu_translate(memory, core, addr, action, &addr))
        {
            core->cr_state.crexc.exception = kE64ExceptionPageFault;
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
            case kE64MemoryActionPageDirectory:
            case kE64MemoryActionExecute:
            case kE64MemoryActionRead:
                E64MemoryCoreAction(memoryRef, core, addr, lo_size, &lo_val, action);
                E64MemoryCoreAction(memoryRef, core, page_end, hi_size, &hi_val, action);
                *value = lo_val | (hi_val << hi_shift);
                return;
            case kE64MemoryActionWrite:
                lo_mask = (lo_size == 8) ? ~0ULL : (1ULL << hi_shift) - 1;
                lo_val = *value & lo_mask;
                hi_val = *value >> hi_shift;
                E64MemoryCoreAction(memoryRef, core, addr, lo_size, &lo_val, action);
                E64MemoryCoreAction(memoryRef, core, page_end, hi_size, &hi_val, action);
                return;
        }
        return;
    }
    else
rw_fastpath:
    {
        if(unlikely(!E64MemoryAccessIsWithinBounds(memoryRef, addr, size)))
        {
            core->cr_state.crexc.exception = kE64ExceptionBadAccess;
            return;
        }

        uint8_t *mem_ptr = memory->memory + addr;

        switch(action)
        {
            case kE64MemoryActionPageDirectory:
            case kE64MemoryActionExecute:
            case kE64MemoryActionRead:
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
                        core->cr_state.crexc.exception = kE64ExceptionBadAccess; 
                        return;
                }
                return;
            case kE64MemoryActionWrite:
                if(unlikely(memory->ktrr_size > addr))
                {
                    core->cr_state.crexc.exception = kE64ExceptionKTRRViolation;
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
                        core->cr_state.crexc.exception = kE64ExceptionBadAccess; 
                        return;
                }
                return;
        }
    }
}

bool E64MemoryCoreCopyIn(E64MemoryRef memoryRef,
                            emex64_core_t *core,
                            uint8_t *dst,
                            uint64_t addr,
                            size_t len,
                            kE64MemoryAction action)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return false;
    }

    /* do not allow other actions than rx */
    assert(action != kE64MemoryActionWrite);

    if(unlikely((core->cr_state.crexc.exception == kE64ExceptionBadAccess || core->cr_state.crexc.exception == kE64ExceptionKTRRViolation) && !core->in_interrupt))
    {
        return false;
    }

    /* there will never be a buffer copy out on the MMIO regions */
    if(unlikely((addr >> 53) || ((addr + len - 1) >> 53)))
    {
        core->cr_state.crexc.exception = kE64ExceptionBadAccess;
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
            if(unlikely(!emex64_mmu_translate(memory, core, addr, action, &paddr)))
            {
                core->cr_state.crexc.exception = kE64ExceptionPageFault;
                return false;
            }

            size_t page_left = (size_t)(EMEX64_PAGE_SIZE - (addr & EMEX64_PAGE_MASK));
            if(chunk > page_left)
            {
                chunk = page_left;
            }
        }

        if(unlikely(!E64MemoryAccessIsWithinBounds(memoryRef, paddr, chunk)))
        {
            core->cr_state.crexc.exception = kE64ExceptionBadAccess;
            return false;
        }

        /*
         * only a kernel level core may execute kernel space
         * code when KTRR is locked.
         *
         * fixme: causes SIGBUS
         */
        /*if(action == kE64MemoryActionExecute)
        {
            if(unlikely(core->rl[kE64RegisterCR0] >= kE64ElevationLevelKernel && core->machine->memory->ktrr_locked && core->machine->memory->ktrr_size < (addr + len)))
            {
                chunk = (size_t)(core->machine->memory->ktrr_size - addr);
            }
        }*/

        memcpy(dst, &memory->memory[paddr], chunk);

        dst += chunk;
        addr += chunk;
        len -= chunk;
    }

    return true;
}
