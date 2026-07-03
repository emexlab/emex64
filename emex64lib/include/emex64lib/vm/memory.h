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

#ifndef EMEX64VM_MEMORY_H
#define EMEX64VM_MEMORY_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <emex64lib/support/file.h>
#include <emex64lib/vm/core.h>
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
        uint64_t mask = (size == 8) ? ~0ULL : (1ULL << (size * 8)) - 1; \
        void *ptr = ((uint8_t*)mapping) + offset;                       \
        uint64_t raw = *(uint64_t *)ptr;                                \
        raw = (raw & ~mask) | (value & mask);                           \
        *(uint64_t *)ptr = raw;                                         \
    }

#define EMEX64_MEMORY_READ_HELPER(mapping, offset, size, out_value)     \
    {                                                                   \
        void *ptr = ((uint8_t*)mapping) + offset;                       \
        uint64_t raw = *(uint64_t *)ptr;                                \
        uint64_t mask = (size == 8) ? ~0ULL : (1ULL << (size * 8)) - 1; \
        out_value = raw & mask;                                         \
    }

#define EMEX64_MEMORY_MMU_MASK_FLAGS    0b0000000000000000000000000000000000000000000000000000000011111111
#define EMEX64_MEMORY_MMU_MASK_PFN      0b1111111111111111111111111111111111111111111111111111111100000000

/* page table entry bit flags */
typedef enum: uint8_t {
    kEmex64MMUPTPresent =   0b00000001, /* marks a PTE or PXD as present */
    kEmex64MMUPTUser =      0b00000010, /* marks a PTE as user accessible, meaning user mode can access that page */
    kEmex64MMUPTDirty =     0b00000100, /* marks a PTE as dirty, writes on it cause a page fault TODO: to be implemented */
    kEmex64MMUPTRead =      0b00001000, /* marks a PTE as readable */
    kEmex64MMUPTWrite =     0b00010000, /* marks a PTE as writable (most MMU's don't have that, but this one does) */
    kEmex64MMUPTExec =      0b00100000, /* marks a PTE as executable (means the CPU core can fetch instructions from it and execute them) */
    kEmex64MMUPTAccessed =  0b01000000, /* marks a PTE as accessed (MMU sets this bit when this has been accessed) */
} kEmex64MMUPT;

typedef enum: uint8_t {
    kEmex64MemoryActionRead =           kEmex64MMUPTRead,
    kEmex64MemoryActionWrite =          kEmex64MMUPTWrite,
    kEmex64MemoryActionExecute =        kEmex64MMUPTExec,
    kEmex64MemoryActionPageDirectory,
} kEmex64MemoryAction;

typedef EFObjectRef Emex64MemoryRef;

EFTypeID Emex64MemoryGetTypeID(void);

Emex64MemoryRef Emex64MemoryCreate(EFAllocatorRef allocatorRef, uint64_t size);

/*
Emex64MemoryRef Emex64MemoryCreateCopy(EVAllocator *allocator, Emex64MemoryRef memoryRef);
*/

void Emex64MemoryLockKTRR(Emex64MemoryRef memoryRef);
bool Emex64MemoryIsKTRRLocked(Emex64MemoryRef memoryRef);
uint64_t Emex64MemoryGetKTRRSize(Emex64MemoryRef memoryRef);
bool Emex64MemorySetKTRRSize(Emex64MemoryRef memoryRef, uint64_t size);

uint64_t Emex64MemoryGetSize(Emex64MemoryRef memoryRef);
bool Emex64MemoryAccessIsWithinBounds(Emex64MemoryRef memoryRef, uint64_t address, uint64_t size);

bool Emex64MemoryLoadImage(Emex64MemoryRef memoryRef, emex_file_t *file);

bool Emex64MemoryAction(Emex64MemoryRef memoryRef, uint64_t addr, size_t size, uint64_t *value, kEmex64MemoryAction action);

/* API that only the VM shall use */
void Emex64MemoryCoreAction(Emex64MemoryRef memoryRef, emex64_core_t *core, uint64_t addr, size_t size, uint64_t *value, kEmex64MemoryAction action);
bool Emex64MemoryCoreCopyIn(Emex64MemoryRef memoryRef, emex64_core_t *core, uint8_t *dst, uint64_t addr, size_t len, kEmex64MemoryAction read_action);

#endif /* EMEX64VM_MEMORY_H */
