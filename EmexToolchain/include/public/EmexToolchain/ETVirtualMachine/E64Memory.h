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
#include <stddef.h>
#include <string.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETVirtualMachine/E64Core.h>
#ifdef ET_PRIVATE
#include <EmexToolchain/ETVirtualMachine/__E64Memory.h>
#else
#include <EmexToolchain/ETVirtualMachine/E64Type.h>
#endif /* ET_PRIVATE */

typedef EFObjectRef E64MemoryRef;

EFTypeID E64MemoryGetTypeID(void);

E64MemoryRef E64MemoryCreate(EFAllocatorRef allocatorRef, UInt64 size);

void E64MemoryLockKTRR(E64MemoryRef memoryRef);
Boolean E64MemoryIsKTRRLocked(E64MemoryRef memoryRef);

UInt64 E64MemoryGetKTRRBase(E64MemoryRef memoryRef);
Boolean E64MemorySetKTRRBase(E64MemoryRef memoryRef, UInt64 base);
UInt64 E64MemoryGetKTRRSize(E64MemoryRef memoryRef);
Boolean E64MemorySetKTRRSize(E64MemoryRef memoryRef, UInt64 size);

UInt64 E64MemoryGetSize(E64MemoryRef memoryRef);
struct emex64_snoop *E64MemoryGetSnoop(E64MemoryRef memoryRef);
Boolean E64MemoryAccessIsWithinBounds(E64MemoryRef memoryRef, UInt64 address, UInt64 size);

Boolean E64MemoryLoadImage(E64MemoryRef memoryRef, EFFileRef fileRef);

void *E64MemoryMapPhysical(E64MemoryRef memoryRef, UInt64 paddr, UInt64 len, Boolean forWrite);
Boolean E64MemoryAction(E64MemoryRef memoryRef, UInt64 addr, EFSize size, UInt64 *value, E64MemoryActionType actionType);

UInt8 *E64MemoryGetRAM(E64MemoryRef memoryRef);

#endif /* E64MEMORY_H */
