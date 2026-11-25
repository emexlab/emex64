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

#ifndef __E64MEMORY_H
#define __E64MEMORY_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETVirtualMachine/E64Type.h>

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

typedef EFObjectRef E64MemoryRef;

void __E64MemoryCoreAction(E64MemoryRef memoryRef, E64CoreRef core, UInt64 addr, EFSize size, UInt64 *value, E64MemoryActionType actionType);
Boolean __E64MemoryCoreCopyIn(E64MemoryRef memoryRef, E64CoreRef core, UInt8 *dst, UInt64 addr, EFSize len, E64MemoryActionType actionType);

void *__E64MemoryCoreMap(E64MemoryRef memoryRef, E64CoreRef core, UInt64 addr, EFSize size, E64MemoryActionType actionType);
Boolean __E64MemoryCoreProbe(E64MemoryRef memoryRef, E64CoreRef core, UInt64 addr, UInt64 len, E64MemoryActionType actionType);
Boolean __E64MemoryCoreWatchPage(E64MemoryRef memoryRef, E64CoreRef core, UInt64 vaddr, UInt64 *pte, UInt64 *root);

Boolean __E64MemoryCoreTranslate(E64MemoryRef memoryRef, E64CoreRef core, UInt64 vaddr, E64MemoryActionType actionType, UInt64 *paddr);

void __E64MemoryReset(E64MemoryRef memoryRef);

#endif /* __E64MEMORY_H */
