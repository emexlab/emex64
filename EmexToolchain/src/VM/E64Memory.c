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
#include <pthread.h>
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
#include <EmexToolchain/VM/device/internal/controller/apic.h>

typedef struct E64Memory {
    EFObject header;
    UInt8 *memory;
    UInt64 memory_size;

    /* KTRR is in the memory controller it self */
    UInt64 ktrr_base;
    UInt64 ktrr_size;
    Boolean ktrr_locked;

    emex64_snoop_t snoop;
    EFFileRef image;
} *E64Memory;

static void __E64MemoryDeinit(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64Memory)memoryRef;
    if(memory->memory != MAP_FAILED && memory->memory != NULL)
    {
        munmap(memory->memory, memory->memory_size);
    }
    free(memory->snoop.flags);
    free(memory->snoop.gen);
    if(memory->image != NULL)
    {
        EFRelease(memory->image);
    }
}

static EFClassDefinitionV4 E64MemoryClass = {
    .header = {
        .version = 4,
        .typeID = kEFTypeIDNone,
        .name = EFSTR_FILESCOPE("E64Memory"),
    },
    .init = NULL,
    .deinit = __E64MemoryDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = NULL,
};

typedef struct emex64_mmu_entry_lookup {
    Boolean fail;
    UInt64 *pte;
} emex64_mmu_entry_lookup_t;

static inline Boolean emex64_traps_enabled(E64CoreRef core)
{
    return (core->cr_state.crstat & E64_CRSTAT_TRAPS) != 0;
}

static inline void emex64_memory_fault(E64CoreRef core,
                                       E64Exception exception,
                                       UInt64 addr,
                                       E64MemoryActionType actionType)
{
    if(core->cr_state.crexc.exception == kE64ExceptionNone)
    {
        core->cr_state.crexc.exception = exception;
    }
    if(!core->trap.fault_valid)
    {
        core->trap.fault_valid = true;
        core->trap.fault_addr = addr;
        core->trap.fault_access = actionType == kE64MemoryActionTypeWrite ? E64_ACCESS_WRITE : actionType == kE64MemoryActionTypeExecute ? E64_ACCESS_EXECUTE : E64_ACCESS_READ;
    }
}

static inline Boolean emex64_mmio_allowed(E64CoreRef core)
{
    if(core->cr_state.crel.level >= kE64ElevationLevelKernel)
    {
        return true;
    }
    return !emex64_traps_enabled(core) && !(core->cr_state.crptb.enabled && !core->in_interrupt);
}

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

    memory->memory_size = EMEX64_PAGE_ROUND_UP(size);
    memory->memory = mmap(NULL, memory->memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(memory->memory == MAP_FAILED)
    {
        return NULL;
    }

    memory->snoop.chunks = memory->memory_size >> EMEX64_CHUNK_SHIFT;
    memory->snoop.scan_lo = UINT64_MAX;
    memory->snoop.scan_hi = 0;
    memory->snoop.flags = calloc(memory->snoop.chunks, sizeof(UInt8));
    memory->snoop.gen = calloc(memory->snoop.chunks, sizeof(UInt32));
    if(memory->snoop.flags == NULL || memory->snoop.gen == NULL)
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
    memory->snoop.ktrr_locked = true;
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

static inline Boolean emex64_ktrr_overlaps(E64Memory memory,
                                           UInt64 addr,
                                           UInt64 len)
{
    if(memory->ktrr_size == 0 || len == 0)
    {
        return false;
    }
    UInt64 end = memory->ktrr_base + memory->ktrr_size;
    return addr < end && addr + len > memory->ktrr_base;
}

UInt64 E64MemoryGetKTRRBase(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL)
    {
        return 0;
    }

    return memory->ktrr_base;
}

Boolean E64MemorySetKTRRBase(E64MemoryRef memoryRef,
                             UInt64 base)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL || memory->ktrr_locked)
    {
        return false;
    }

    memory->ktrr_base = base;
    memory->snoop.ktrr_base = base;
    return true;
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
    memory->snoop.ktrr_size = size;
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
                                                              UInt16 idx,
                                                              UInt8 what,
                                                              UInt32 *gen)
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

    UInt64 entry_pa = pt_addr + (UInt64)idx * sizeof(UInt64);
    emex64_snoop_mark(&memory->snoop, entry_pa, what);
    if(gen != NULL)
    {
        *gen = __atomic_load_n(&memory->snoop.gen[entry_pa >> EMEX64_CHUNK_SHIFT], __ATOMIC_ACQUIRE);
    }

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
                                            UInt64 *oaddr,
                                            UInt64 **pte_out,
                                            UInt32 *gen_out)
{
    UInt8 what = actionType == kE64MemoryActionTypePageDirectory ? EMEX64_CHUNK_PAGE_TABLE : EMEX64_CHUNK_PTE;
    emex64_mmu_entry_lookup_t lookup = emex64_mmu_lookup_pte(memory, core, pt_addr, pxd_idx, what, gen_out);
    if(unlikely(lookup.fail))
    {
        return false;
    }
    *pte_out = lookup.pte;

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
    emex64_tlb_entry_t *hit = emex64_tlb_lookup(&core->tlb, &memory->snoop, vaddr);
    if(likely(hit != NULL))
    {
        UInt8 checkflg = actionType;
        if(core->cr_state.crel.level < kE64ElevationLevelKernel)
        {
            checkflg |= kE64MMUPTUser;
        }
        if(likely((hit->flags & checkflg) == checkflg &&
                  (actionType != kE64MemoryActionTypeWrite || (hit->flags & kE64MMUPTDirty))))
        {
            *paddr = hit->ppage + (vaddr & 0x1FFF);
            return true;
        }
    }

    /*
     * getting page global directory from physical frame number
     * stored in the 5th level (yk the control register x3).
     */
    UInt64 pgd_addr = core->cr_state.crptb.pgd_addr;
    UInt64 epoch = __atomic_load_n(&memory->snoop.tlb_epoch, __ATOMIC_ACQUIRE);

    /* still unknown page directory addresses */
    UInt64 pud_addr, pmd_addr, pte_addr, phys_page_base_addr;
    UInt64 *used[4];
    UInt32 pte_gen = 0;

    /* now access each table */
    if(!emex64_mmu_access_pxd(memory, core, pgd_addr, ((vaddr >> 43) & 0x3FF), kE64MemoryActionTypePageDirectory, &pud_addr, &used[0], NULL) ||   /* 10 bits for each level index  */
       !emex64_mmu_access_pxd(memory, core, pud_addr, ((vaddr >> 33) & 0x3FF), kE64MemoryActionTypePageDirectory, &pmd_addr, &used[1], NULL) ||
       !emex64_mmu_access_pxd(memory, core, pmd_addr, ((vaddr >> 23) & 0x3FF), kE64MemoryActionTypePageDirectory, &pte_addr, &used[2], NULL) ||
       !emex64_mmu_access_pxd(memory, core, pte_addr, ((vaddr >> 13) & 0x3FF), actionType, &phys_page_base_addr, &used[3], &pte_gen))
    {
        return false;
    }

    *paddr = phys_page_base_addr + (vaddr & 0x1FFF); /* 13bit offset (addressing within a page) */

    UInt64 vpn = vaddr >> 13;
    emex64_tlb_entry_t *e = emex64_tlb_slot(&core->tlb, vpn, core->tlb.asid);
    e->vpn = vpn;
    e->ppage = phys_page_base_addr;
    e->epoch = epoch;
    e->pte_chunk = (UInt64)((UInt8 *)used[3] - memory->memory) >> EMEX64_CHUNK_SHIFT;
    e->pte_gen = pte_gen;
    e->asid = core->tlb.asid;
    e->flags = (UInt8)(*used[3] & EMEX64_MEMORY_MMU_MASK_FLAGS);
    e->valid = true;

    return true;
}

struct emex64_snoop *E64MemoryGetSnoop(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    return memory != NULL ? &memory->snoop : NULL;
}

UInt8 *E64MemoryGetRAM(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    return memory != NULL ? memory->memory : NULL;
}

Boolean E64MemoryCoreTranslate(E64MemoryRef memoryRef,
                               E64CoreRef core,
                               UInt64 vaddr,
                               E64MemoryActionType actionType,
                               UInt64 *paddr)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL || (vaddr >> 53))
    {
        return false;
    }
    if(!core->cr_state.crptb.enabled || core->in_interrupt)
    {
        *paddr = vaddr;
        return vaddr < memory->memory_size;
    }
    return emex64_mmu_translate(memory, core, vaddr, actionType, paddr);
}

void *E64MemoryMapPhysical(E64MemoryRef memoryRef,
                           UInt64 paddr,
                           UInt64 len,
                           Boolean forWrite)
{
    E64Memory memory = (E64MemoryRef)memoryRef;
    if(memory == NULL || len == 0)
    {
        return NULL;
    }

    if(paddr >= memory->memory_size || len > memory->memory_size - paddr)
    {
        return NULL;
    }

    if(forWrite && emex64_ktrr_overlaps(memory, paddr, len))
    {
        return NULL;
    }

    if(forWrite)
    {
        emex64_snoop_write(&memory->snoop, paddr, len);
    }

    return memory->memory + paddr;
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

    if(memory->image != fileRef)
    {
        if(memory->image != NULL)
        {
            EFRelease(memory->image);
        }
        memory->image = (EFFileRef)EFRetain(fileRef);
    }
    return true;
}

void E64MemoryReset(E64MemoryRef memoryRef)
{
    E64Memory memory = (E64Memory)memoryRef;
    if(memory == NULL)
    {
        return;
    }

    if(mmap(memory->memory, memory->memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0) == MAP_FAILED)
    {
        memset(memory->memory, 0, memory->memory_size);
    }
    if(memory->image != NULL)
    {
        E64MemoryLoadImage(memoryRef, memory->image);
    }

    memory->ktrr_base = 0;
    memory->ktrr_size = 0;
    memory->ktrr_locked = false;
    memory->snoop.ktrr_base = 0;
    memory->snoop.ktrr_size = 0;
    memory->snoop.ktrr_locked = false;

    memset(memory->snoop.flags, 0, memory->snoop.chunks);
    for(UInt64 c = 0; c < memory->snoop.chunks; c++)
    {
        memory->snoop.gen[c]++;
    }
    memory->snoop.tlb_epoch++;
    memory->snoop.code_seq++;
    memory->snoop.map_seq++;
    memory->snoop.scan_lo = UINT64_MAX;
    memory->snoop.scan_hi = 0;
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
            if(unlikely(emex64_ktrr_overlaps(memory, addr, size)))
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
            emex64_snoop_write(&memory->snoop, addr, size);
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

    if(unlikely(core->cr_state.crexc.exception != kE64ExceptionNone && emex64_traps_enabled(core)))
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
        emex64_core_context_changed(core);

        if(unlikely(addr >> 54))
        {
            emex64_memory_fault(core, kE64ExceptionBadAccess, addr, actionType);
            return;
        }

        /*
         * MMIO shall never be accessed when paging is enabled or when
         * the access is not aligned to a quad word boundary, as
         * the decoder later in actual emex64 hardware would explode
         * in size otherwise. MMIO busses are complex!
         */
        if(unlikely(!emex64_mmio_allowed(core) || !EMEX64_IS_ALIGNED_64(addr) || size != 8))
        {
            emex64_memory_fault(core, kE64ExceptionBadAccess, addr, actionType);
            return;
        }

        if(addr >= EMEX64_APIC_BASE && addr < EMEX64_APIC_BASE + EMEX64_APIC_SIZE)
        {
            switch(actionType)
            {
                case kE64MemoryActionTypeRead:
                    *value = emex64_lapic_read(core, addr - EMEX64_APIC_BASE);
                    return;
                case kE64MemoryActionTypeWrite:
                    emex64_lapic_write(core, addr - EMEX64_APIC_BASE, *value);
                    return;
                default:
                    emex64_memory_fault(core, kE64ExceptionBadAccess, addr, actionType);
                    return;
            }
        }

        __E64Machine machine = (__E64Machine)core->machine;
        if(addr == EMEX64_TIMER_BASE + TIMER_REG_COUNT && actionType == kE64MemoryActionTypeRead &&
           likely(machine->timer != NULL) && likely(emex64_timer_read_count(machine->timer, value)))
        {
            return;
        }

        pthread_mutex_lock(&machine->mmio_lock);
        E64MMIORegionRef mmio_region = E64MMIOBusGetRegionForAddress(machine->mmio_bus, addr);
        void *device = E64MMIORegionGetDevice(mmio_region);
        if(likely(mmio_region != NULL))
        {
            UInt64 offset = addr - E64MMIORegionGetBaseAddress(mmio_region);
            switch(actionType)
            {
                case kE64MemoryActionTypeRead:
                    mmio_read_fn read = E64MMIORegionGetReadSymbol(mmio_region);
                    *value = read(core, device, offset, size);
                    break;
                case kE64MemoryActionTypeWrite:
                    mmio_write_fn write = E64MMIORegionGetWriteSymbol(mmio_region);
                    if(write != NULL)
                    {
                        write(core, device, offset, *value, size);
                    }
                    break;
                default:
                    emex64_memory_fault(core, kE64ExceptionBadAccess, addr, actionType);
                    break;
            }
            pthread_mutex_unlock(&machine->mmio_lock);
            return;
        }
        pthread_mutex_unlock(&machine->mmio_lock);

        emex64_memory_fault(core, kE64ExceptionBadAccess, addr, actionType);
        return;
    }

    /*
     * find out if paging is enabled, if not write vaddr to paddr,
     * because that means paddr is vaddr because virtual addressing
     * is already off.
     *
     * we read it as if it was a 5th level entry, but its just a
     * control register.. for simplicity we do that hahaha.
     */
    UInt64 vaddr = addr;
    if(core->cr_state.crptb.enabled && !core->in_interrupt)
    {
        if(!emex64_mmu_translate(memory, core, addr, actionType, &addr))
        {
            emex64_memory_fault(core, kE64ExceptionPageFault, vaddr, actionType);
            return;
        }
    }
    else
    {
        goto rw_fastpath;
    }

    UInt64 page_end = (vaddr & ~EMEX64_PAGE_MASK) + EMEX64_PAGE_SIZE;
    EFSize lo_size = (EFSize)(page_end - vaddr);

    if(lo_size < size)
    {
        addr = vaddr;
        EFSize hi_size = size - lo_size;
        UInt64 hi_shift = lo_size * 8;
        UInt64 lo_val, hi_val, lo_mask;

        switch(actionType)
        {
            case kE64MemoryActionTypePageDirectory:
            case kE64MemoryActionTypeExecute:
            case kE64MemoryActionTypeRead:
                lo_val = 0;
                hi_val = 0;
                E64MemoryCoreAction(memoryRef, core, addr, lo_size, &lo_val, actionType);
                E64MemoryCoreAction(memoryRef, core, page_end, hi_size, &hi_val, actionType);
                if(unlikely(core->cr_state.crexc.exception != kE64ExceptionNone && emex64_traps_enabled(core)))
                {
                    return;
                }
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
            emex64_memory_fault(core, kE64ExceptionBadAccess, addr, actionType);
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
                    {
                        UInt64 tmp = 0;
                        for(EFSize i = 0; i < size; i++)
                        {
                            tmp |= (UInt64)mem_ptr[i] << (i * 8);
                        }
                        *value = tmp;
                        break;
                    }
                }
                return;
            case kE64MemoryActionTypeWrite:
                if(unlikely(emex64_ktrr_overlaps(memory, addr, size)))
                {
                    emex64_memory_fault(core, kE64ExceptionKTRRViolation, addr, actionType);
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
                    {
                        UInt64 tmp = *value;
                        for(EFSize i = 0; i < size; i++)
                        {
                            mem_ptr[i] = (UInt8)(tmp >> (i * 8));
                        }
                        break;
                    }
                }
                emex64_snoop_write(&memory->snoop, addr, size);
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
        emex64_memory_fault(core, kE64ExceptionBadAccess, addr, actionType);
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
                emex64_memory_fault(core, kE64ExceptionPageFault, addr, actionType);
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
            emex64_memory_fault(core, kE64ExceptionBadAccess, addr, actionType);
            return false;
        }

        /* a locked KTRR region is the only code kernel may execute */
        if(actionType == kE64MemoryActionTypeExecute && memory->ktrr_locked &&
           core->cr_state.crel.level >= kE64ElevationLevelKernel &&
           (paddr < memory->ktrr_base || paddr + chunk > memory->ktrr_base + memory->ktrr_size))
        {
            emex64_memory_fault(core, kE64ExceptionKTRRViolation, addr, actionType);
            return false;
        }

        memcpy(dst, &memory->memory[paddr], chunk);

        dst += chunk;
        addr += chunk;
        len -= chunk;
    }

    return true;
}

void *E64MemoryCoreMap(E64MemoryRef memoryRef,
                       E64CoreRef core,
                       UInt64 addr,
                       EFSize size,
                       E64MemoryActionType actionType)
{
    E64Memory memory = (E64Memory)memoryRef;
    if(memory == NULL || core == NULL || size == 0)
    {
        return NULL;
    }

    UInt64 last_addr = addr + (UInt64)size - 1;
    if(last_addr < addr)
    {
        return NULL;
    }

    if((addr >> 53) || (last_addr >> 53))
    {
        return NULL;
    }

    /* paging disabled */
    if(!core->cr_state.crptb.enabled || core->in_interrupt)
    {
        if(!E64MemoryAccessIsWithinBounds(memoryRef, addr, size))
        {
            return NULL;
        }

        if(actionType == kE64MemoryActionTypeWrite)
        {
            if(emex64_ktrr_overlaps(memory, addr, size))
            {
                return NULL;
            }
            emex64_snoop_write(&memory->snoop, addr, size);
        }

        return memory->memory + addr;
    }

    /* paging enabled */
    UInt64 vaddr = addr;
    UInt64 remaining = size;
    UInt64 consumed = 0;

    UInt64 first_paddr = 0;

    while(remaining > 0)
    {
        UInt64 paddr;
        if(!emex64_mmu_translate(memory, core, vaddr, actionType, &paddr))
        {
            return NULL;
        }

        UInt64 page_left = EMEX64_PAGE_SIZE - (vaddr & EMEX64_PAGE_MASK);
        UInt64 chunk = remaining;
        if(chunk > page_left)
        {
            chunk = page_left;
        }

        if(!E64MemoryAccessIsWithinBounds(memoryRef, paddr, chunk))
        {
            return NULL;
        }

        if(actionType == kE64MemoryActionTypeWrite)
        {
            if(emex64_ktrr_overlaps(memory, paddr, chunk))
            {
                return NULL;
            }
            emex64_snoop_write(&memory->snoop, paddr, chunk);
        }

        if(consumed == 0)
        {
            first_paddr = paddr;
        }
        else
        {
            UInt64 expected_paddr = first_paddr + consumed;
            if(paddr != expected_paddr)
            {
                return NULL;
            }
        }

        vaddr += chunk;
        consumed += chunk;
        remaining -= chunk;
    }

    return memory->memory + first_paddr;
}
