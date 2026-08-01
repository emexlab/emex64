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
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/Support/likely.h>
#include <EmexToolchain/VM/E64Memory.h>
#include <EmexToolchain/VM/E64Core.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/E64MMIOBus.h>

typedef struct E64Memory {
    EFObject header;
    UInt8 *memory;
    UInt64 memory_size;
    UInt64 ktrr_size;
    Boolean ktrr_locked;
} *E64Memory;

static void __E64MemoryDeinit(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64Memory)memoryRef;
    if(memory->memory != MAP_FAILED)
    {
        munmap(memory->memory, memory->memory_size);
    }
}

static EFClassDefinitionV2 E64MemoryClass = {
    .header = {
        .version = 2,
        .typeID = kEFTypeIDNone,
        .name = NULL,
    },
    .name = "E64Memory",
    .init = NULL,
    .deinit = __E64MemoryDeinit,
    .equal = NULL,
    .copyDescription = NULL,
};

typedef struct emex64_mmu_entry_lookup {
    Boolean fail;
    UInt64 *pte;
} emex64_mmu_entry_lookup_t;

static void E64MemoryRegisterClass(void)
{
    EFClassRegister(&E64MemoryClass);
}

EFTypeID E64MemoryGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, E64MemoryRegisterClass);
    return E64MemoryClass.header.typeID;
}

E64MemoryRef E64MemoryCreate(EFAllocatorRef allocatorRef,
                             UInt64 size)
{
    EFAUTOREL E64Memory memory = EFObjectCreate(allocatorRef, E64MemoryGetTypeID(), (EFIndex)sizeof(struct E64Memory));
    if(memory == NULL)
    {
        return NULL;
    }

    /* allocate raw memory (using mmap for larger sizes, better than malloc in this case) */
    memory->memory_size = EMEX64_PAGE_ROUND_UP(size);
    memory->memory = mmap(NULL, memory->memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(memory->memory == MAP_FAILED)
    {
        return NULL;
    }

    return (E64MemoryRef)EFAUTOTRANSFER(memory);
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

Boolean E64MemoryIsKTRRLocked(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return false;
    }

    return memory->ktrr_locked;
}

UInt64 E64MemoryGetKTRRSize(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return 0;
    }

    return memory->ktrr_size;
}

Boolean E64MemorySetKTRRSize(E64MemoryRef memoryRef,
                             UInt64 size)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL || memory->ktrr_locked)
    {
        return false;
    }

    memory->ktrr_size = size;
    return true;
}

UInt64 E64MemoryGetSize(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return 0;
    }

    return memory->memory_size;
}

Boolean E64MemoryAccessIsWithinBounds(E64MemoryRef memoryRef,
                                      UInt64 address,
                                      UInt64 size)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return false;
    }

    UInt64 addr_end = address + size;
    if(address > addr_end || memory->memory_size < addr_end)
    {
        return false;
    }
    return true;
}

static inline emex64_mmu_entry_lookup_t emex64_mmu_lookup_pte(E64Memory memory,
                                                              E64CoreRef core,
                                                              UInt64 pt_addr,
                                                              UInt16 idx)
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
    UInt64 *pt = (UInt64*)&memory->memory[pt_addr];
    UInt64 *pte = &pt[idx];

    if(unlikely(!((*pte & EMEX64_MEMORY_MMU_MASK_FLAGS) & kE64MMUPTPresent)))
    {
        return (emex64_mmu_entry_lookup_t){ .fail = true, .pte = NULL };
    }

    return (emex64_mmu_entry_lookup_t){ .fail = false, .pte = pte };
}

static inline Boolean emex64_mmu_access_pxd(E64Memory memory,
                                            E64CoreRef core,
                                            UInt64 pt_addr,
                                            UInt16 pxd_idx,
                                            E64MemoryActionType actionType,
                                            UInt64 *oaddr)
{
    emex64_mmu_entry_lookup_t lookup = emex64_mmu_lookup_pte(memory, core, pt_addr, pxd_idx);
    if(unlikely(lookup.fail))
    {
        return false;
    }

    UInt64 mmu_flags = 0;
    if(actionType != kE64MemoryActionTypePageDirectory)
    {
        UInt8 checkflg = actionType;

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

    UInt64 pfn = (*(lookup.pte) & EMEX64_MEMORY_MMU_MASK_PFN) >> 8;
    UInt64 physaddr = EMEX64_PAGE_ROUND_DOWN(pfn << 13);
    if(unlikely(!EMEX64_IN_PHYS_MEMORY(physaddr, EMEX64_PAGE_SIZE, memory->memory, memory->memory_size)))
    {
        return false;
    }

    switch(actionType)
    {
        case kE64MemoryActionTypePageDirectory:
            /* not a normal page access */
            break;
        case kE64MemoryActionTypeWrite:
            mmu_flags |= kE64MMUPTDirty;
            /* fallthrough */
        case kE64MemoryActionTypeRead:
        case kE64MemoryActionTypeExecute:
            mmu_flags |= kE64MMUPTAccessed;
            *(lookup.pte) = (*(lookup.pte) & ~EMEX64_MEMORY_MMU_MASK_FLAGS) | mmu_flags;
    }

    *oaddr = physaddr;

    return true;
}

static inline Boolean emex64_mmu_translate(E64Memory memory,
                                           E64CoreRef core,
                                           UInt64 vaddr,
                                           E64MemoryActionType actionType,
                                           UInt64 *paddr)
{
    /*
     * getting page global directory from physical frame number
     * stored in the 5th level (yk the control register x3).
     */
    UInt64 pgd_addr = core->cr_state.crptb.pgd_addr;

    /* still unknown page directory addresses */
    UInt64 pud_addr, pmd_addr, pte_addr, phys_page_base_addr;

    /* now access each table */
    if(!emex64_mmu_access_pxd(memory, core, pgd_addr, ((vaddr >> 43) & 0x3FF), kE64MemoryActionTypePageDirectory, &pud_addr) ||   /* 10 bits for each level index  */
       !emex64_mmu_access_pxd(memory, core, pud_addr, ((vaddr >> 33) & 0x3FF), kE64MemoryActionTypePageDirectory, &pmd_addr) ||
       !emex64_mmu_access_pxd(memory, core, pmd_addr, ((vaddr >> 23) & 0x3FF), kE64MemoryActionTypePageDirectory, &pte_addr) ||
       !emex64_mmu_access_pxd(memory, core, pte_addr, ((vaddr >> 13) & 0x3FF), actionType, &phys_page_base_addr))
    {
        return false;
    }

    *paddr = phys_page_base_addr + (vaddr & 0x1FFF); /* 13bit offset (addressing within a page) */

    return true;
}

Boolean E64MemoryLoadImage(E64MemoryRef memoryRef,
                           EFFileRef fileRef)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return false;
    }

    EFAUTOREL EFFileHandleRef fileHandle = EFFileCopyFileHandle(EFGetAllocator(memoryRef), fileRef);
    if(fileHandle == NULL)
    {
        diag_fatal(NULL, "failed to dup file descriptor from file\n");
        return false;
    }

    EFIndex imageLength = EFFileHandleGetLength(fileHandle);
    if(imageLength > (EFIndex)memory->memory_size)
    {
        diag_error(NULL, "firmware image is too large");
        return false;
    }

    /*
     * overmap the memory with the file in a dirty way tehe ^^
     * meaning that when ever the vm writes to this memory
     * it will become writable as the OS then copies the memory
     * to a writable page, this is much faster than copying it
     * our selves.
     */
    SInt32 fileDescriptor = EFFileHandleGetFileDescriptor(fileHandle);
    EFAUTOREL EFMappingRef mapping = EFMappingCreate(EFGetAllocator(memoryRef), memory->memory, (EFSize)imageLength, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fileDescriptor, 0);
    if(mapping == NULL)
    {
        diag_error(NULL, "mapping firmware image failed");
        return false;
    }
    EFMappingDisableUnmap(mapping);

    return true;
}

Boolean E64MemoryAction(E64MemoryRef memoryRef,
                        UInt64 addr, EFSize size,
                        UInt64 *value,
                        E64MemoryActionType actionType)
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

    UInt8 *mem_ptr = memory->memory + addr;

    switch(actionType)
    {
        case kE64MemoryActionTypePageDirectory:
        case kE64MemoryActionTypeExecute:
        case kE64MemoryActionTypeRead:
            switch(size)
            {
                case 1:
                    *value = *(UInt8 *)mem_ptr;
                    break;
                case 2:
                {
                    UInt16 tmp;
                    memcpy(&tmp, mem_ptr, 2);
                    *value = TO_HOST16(tmp);
                    break;
                }
                case 4:
                {
                    UInt32 tmp;
                    memcpy(&tmp, mem_ptr, 4);
                    *value = TO_HOST32(tmp);
                    break;
                }
                case 8:
                {
                    UInt64 tmp;
                    memcpy(&tmp, mem_ptr, 8);
                    *value = TO_HOST64(tmp);
                    break;
                }
                default:
                    return false;
            }
            return true;
        case kE64MemoryActionTypeWrite:
            if(unlikely(memory->ktrr_size > addr))
            {
                return false;
            }
            switch(size)
            {
                case 1:
                    *(UInt8 *)mem_ptr = (UInt8)*value;
                    break;
                case 2:
                {
                    UInt16 tmp = (UInt16)*value;
                    tmp = TO_HOST16(tmp);
                    memcpy(mem_ptr, &tmp, 2);
                    break;
                }
                case 4:
                {
                    UInt32 tmp = (UInt32)*value;
                    tmp = TO_HOST32(tmp);
                    memcpy(mem_ptr, &tmp, 4);
                    break;
                }
                case 8:
                {
                    UInt64 tmp = (UInt64)*value;
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
                         E64CoreRef core,
                         UInt64 addr,
                         EFSize size,
                         UInt64 *value,
                         E64MemoryActionType actionType)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL || unlikely((core->cr_state.crexc.exception == kE64ExceptionBadAccess || core->cr_state.crexc.exception == kE64ExceptionKTRRViolation) && !core->in_interrupt))
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
        if(unlikely((core->cr_state.crptb.enabled && !core->in_interrupt) || (addr < EMEX64_FB_BASE && (!EMEX64_IS_ALIGNED_64(addr) || size != 8))))
        {
            core->cr_state.crexc.exception = kE64ExceptionBadAccess;
            return;
        }

        E64MMIORegionRef mmio_region = E64MMIOBusGetRegionForAddress(core->machine->mmio_bus, addr);
        void *device = E64MMIORegionGetDevice(mmio_region);
        if(likely(mmio_region != NULL))
        {
            UInt64 offset = addr - E64MMIORegionGetBaseAddress(mmio_region);
            switch(actionType)
            {
                case kE64MemoryActionTypeRead:
                    mmio_read_fn read = E64MMIORegionGetReadSymbol(mmio_region);
                    *value = read(core, device, offset, size);
                    return;
                case kE64MemoryActionTypeWrite:
                    mmio_write_fn write = E64MMIORegionGetWriteSymbol(mmio_region);
                    write(core, device, offset, *value, size);
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
        if(!emex64_mmu_translate(memory, core, addr, actionType, &addr))
        {
            core->cr_state.crexc.exception = kE64ExceptionPageFault;
            return;
        }
    }
    else
    {
        goto rw_fastpath;
    }

    UInt64 page_end = (addr & ~EMEX64_PAGE_MASK) + EMEX64_PAGE_SIZE;
    EFSize lo_size = (EFSize)(page_end - addr);

    if(lo_size < size)
    {
        EFSize hi_size = size - lo_size;
        UInt64 hi_shift = lo_size * 8;
        UInt64 lo_val, hi_val, lo_mask;

        switch(actionType)
        {
            case kE64MemoryActionTypePageDirectory:
            case kE64MemoryActionTypeExecute:
            case kE64MemoryActionTypeRead:
                E64MemoryCoreAction(memoryRef, core, addr, lo_size, &lo_val, actionType);
                E64MemoryCoreAction(memoryRef, core, page_end, hi_size, &hi_val, actionType);
                *value = lo_val | (hi_val << hi_shift);
                return;
            case kE64MemoryActionTypeWrite:
                lo_mask = (lo_size == 8) ? ~0ULL : (1ULL << hi_shift) - 1;
                lo_val = *value & lo_mask;
                hi_val = *value >> hi_shift;
                E64MemoryCoreAction(memoryRef, core, addr, lo_size, &lo_val, actionType);
                E64MemoryCoreAction(memoryRef, core, page_end, hi_size, &hi_val, actionType);
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

        UInt8 *mem_ptr = memory->memory + addr;

        switch(actionType)
        {
            case kE64MemoryActionTypePageDirectory:
            case kE64MemoryActionTypeExecute:
            case kE64MemoryActionTypeRead:
                switch(size)
                {
                    case 1:
                        *value = *(UInt8 *)mem_ptr;
                        break;
                    case 2:
                    {
                        UInt16 tmp;
                        memcpy(&tmp, mem_ptr, 2);
                        *value = TO_HOST16(tmp);
                        break;
                    }
                    case 4:
                    {
                        UInt32 tmp;
                        memcpy(&tmp, mem_ptr, 4);
                        *value = TO_HOST32(tmp);
                        break;
                    }
                    case 8:
                    {
                        UInt64 tmp;
                        memcpy(&tmp, mem_ptr, 8);
                        *value = TO_HOST64(tmp);
                        break;
                    }
                    default:
                        core->cr_state.crexc.exception = kE64ExceptionBadAccess;
                        return;
                }
                return;
            case kE64MemoryActionTypeWrite:
                if(unlikely(memory->ktrr_size > addr))
                {
                    core->cr_state.crexc.exception = kE64ExceptionKTRRViolation;
                    return;
                }
                switch(size)
                {
                    case 1:
                        *(UInt8 *)mem_ptr = (UInt8)*value;
                        break;
                    case 2:
                    {
                        UInt16 tmp = (UInt16)*value;
                        tmp = TO_HOST16(tmp);
                        memcpy(mem_ptr, &tmp, 2);
                        break;
                    }
                    case 4:
                    {
                        UInt32 tmp = (UInt32)*value;
                        tmp = TO_HOST32(tmp);
                        memcpy(mem_ptr, &tmp, 4);
                        break;
                    }
                    case 8:
                    {
                        UInt64 tmp = (UInt64)*value;
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

Boolean E64MemoryCoreCopyIn(E64MemoryRef memoryRef,
                            E64CoreRef core,
                            UInt8 *dst,
                            UInt64 addr,
                            EFSize len,
                            E64MemoryActionType actionType)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return false;
    }

    /* do not allow other actions than rx */
    assert(actionType != kE64MemoryActionTypeWrite);

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

    Boolean paging = core->cr_state.crptb.enabled && !core->in_interrupt;

    /* walking the MMU once per page */
    while(len > 0)
    {
        UInt64 paddr = addr;
        EFSize chunk = len;

        if(paging)
        {
            if(unlikely(!emex64_mmu_translate(memory, core, addr, actionType, &paddr)))
            {
                core->cr_state.crexc.exception = kE64ExceptionPageFault;
                return false;
            }

            EFSize page_left = (EFSize)(EMEX64_PAGE_SIZE - (addr & EMEX64_PAGE_MASK));
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
                chunk = (EFSize)(core->machine->memory->ktrr_size - addr);
            }
        }*/

        memcpy(dst, &memory->memory[paddr], chunk);

        dst += chunk;
        addr += chunk;
        len -= chunk;
    }

    return true;
}
