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

#ifndef E64MEMORY_H
#define E64MEMORY_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <EmexToolchain/support/file.h>
#include <EmexToolchain/vm/E64Core.h>
#include <EmexFoundation/EmexFoundation.h>

#define EMEX64_PAGE_SIZE 0x2000
#define EMEX64_PAGE_MASK (EMEX64_PAGE_SIZE - 1)
#define EMEX64_PAGE_ROUND_DOWN(x) ((x) & ~((EMEX64_PAGE_SIZE) - 1))
#define EMEX64_PAGE_ROUND_UP(x) (((x) + (EMEX64_PAGE_SIZE) - 1) & ~((EMEX64_PAGE_SIZE) - 1))
#define EMEX64_IN_PHYS_MEMORY(addr, access_size, mem_base, mem_size) (((uintptr_t)(addr) < (uintptr_t)(mem_size)) && ((uintptr_t)(addr) + (access_size) <= (uintptr_t)(mem_size)))
#define EMEX64_BYTES_TO_PAGE_BOUNDARY(addr) (EMEX64_PAGE_SIZE - ((uintptr_t)(addr) & EMEX64_PAGE_MASK))
#define EMEX64_CROSS_PAGE_OFFSET(addr, access_size) (((access_size) > EMEX64_BYTES_TO_PAGE_BOUNDARY(addr)) ? EMEX64_BYTES_TO_PAGE_BOUNDARY(addr) : 0)
#define EMEX64_IS_ALIGNED_64(addr) (((addr) & 0x7) == 0)

#define EMEX64_MEMORY_WRITE_HELPER(mapping, offset, size, value)        \
    {                                                                   \
        UInt64 mask = (size == 8) ? ~0ULL : (1ULL << (size * 8)) - 1; \
        void *ptr = ((UInt8*)mapping) + offset;                       \
        UInt64 raw = *(UInt64 *)ptr;                                \
        raw = (raw & ~mask) | (value & mask);                           \
        *(UInt64 *)ptr = raw;                                         \
    }

#define EMEX64_MEMORY_READ_HELPER(mapping, offset, size, out_value)     \
    {                                                                   \
        void *ptr = ((UInt8*)mapping) + offset;                       \
        UInt64 raw = *(UInt64 *)ptr;                                \
        UInt64 mask = (size == 8) ? ~0ULL : (1ULL << (size * 8)) - 1; \
        out_value = raw & mask;                                         \
    }

#define EMEX64_MEMORY_MMU_MASK_FLAGS    0b0000000000000000000000000000000000000000000000000000000011111111
#define EMEX64_MEMORY_MMU_MASK_PFN      0b1111111111111111111111111111111111111111111111111111111100000000

/* page table entry bit flags */
typedef enum: UInt8 {
    kE64MMUPTPresent =   0b00000001, /* marks a PTE or PXD as present */
    kE64MMUPTUser =      0b00000010, /* marks a PTE as user accessible, meaning user mode can access that page */
    kE64MMUPTDirty =     0b00000100, /* marks a PTE as dirty, writes on it cause a page fault TODO: to be implemented */
    kE64MMUPTRead =      0b00001000, /* marks a PTE as readable */
    kE64MMUPTWrite =     0b00010000, /* marks a PTE as writable (most MMU's don't have that, but this one does) */
    kE64MMUPTExec =      0b00100000, /* marks a PTE as executable (means the CPU core can fetch instructions from it and execute them) */
    kE64MMUPTAccessed =  0b01000000, /* marks a PTE as accessed (MMU sets this bit when this has been accessed) */
} E64MMUPT;

typedef enum: UInt8 {
    kE64MemoryActionTypeRead =          kE64MMUPTRead,
    kE64MemoryActionTypeWrite =         kE64MMUPTWrite,
    kE64MemoryActionTypeExecute =       kE64MMUPTExec,
    kE64MemoryActionTypePageDirectory,
} E64MemoryActionType;

typedef EFObjectRef E64MemoryRef;

EFTypeID E64MemoryGetTypeID(void);

E64MemoryRef E64MemoryCreate(EFAllocatorRef allocatorRef, UInt64 size);

/*
E64MemoryRef E64MemoryCreateCopy(EVAllocator *allocator, E64MemoryRef memoryRef);
*/

void E64MemoryLockKTRR(E64MemoryRef memoryRef);
Boolean E64MemoryIsKTRRLocked(E64MemoryRef memoryRef);
UInt64 E64MemoryGetKTRRSize(E64MemoryRef memoryRef);
Boolean E64MemorySetKTRRSize(E64MemoryRef memoryRef, UInt64 size);

UInt64 E64MemoryGetSize(E64MemoryRef memoryRef);
Boolean E64MemoryAccessIsWithinBounds(E64MemoryRef memoryRef, UInt64 address, UInt64 size);

Boolean E64MemoryLoadImage(E64MemoryRef memoryRef, emex_file_t *file);

Boolean E64MemoryAction(E64MemoryRef memoryRef, UInt64 addr, size_t size, UInt64 *value, E64MemoryActionType actionType);

/* API that only the VM shall use */
void E64MemoryCoreAction(E64MemoryRef memoryRef, E64CoreRef core, UInt64 addr, size_t size, UInt64 *value, E64MemoryActionType actionType);
Boolean E64MemoryCoreCopyIn(E64MemoryRef memoryRef, E64CoreRef core, UInt8 *dst, UInt64 addr, size_t len, E64MemoryActionType actionType);

#endif /* E64MEMORY_H */
