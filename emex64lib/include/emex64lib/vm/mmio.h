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

#ifndef EMEX64VM_MMIO_H
#define EMEX64VM_MMIO_H

#include <stdint.h>
#include <stdbool.h>
#include <EmexFoundation/EmexFoundation.h>

typedef struct emex64_core emex64_core_t;

typedef uint64_t (*mmio_read_fn)(emex64_core_t *core, void *device, uint64_t offset, int size);
typedef void (*mmio_write_fn)(emex64_core_t *core, void *device, uint64_t offset, uint64_t value, int size);

typedef EFObjectRef Emex64MMIORegionRef;

EFTypeID Emex64MMIORegionGetTypeID(void);

Emex64MMIORegionRef Emex64MMIORegionCreate(EFAllocatorRef allocatorRef, uint64_t base, uint64_t size, void *device, mmio_read_fn read, mmio_write_fn write);

uint64_t Emex64MMIORegionGetBaseAddress(Emex64MMIORegionRef MMIORegionRef);
uint64_t Emex64MMIORegionGetSize(Emex64MMIORegionRef MMIORegionRef);
void *Emex64MMIORegionGetDevice(Emex64MMIORegionRef MMIORegionRef);
mmio_read_fn Emex64MMIORegionGetReadSymbol(Emex64MMIORegionRef MMIORegionRef);
mmio_write_fn Emex64MMIORegionGetWriteSymbol(Emex64MMIORegionRef MMIORegionRef);

typedef EFObjectRef Emex64MMIOBusRef;

EFTypeID Emex64MMIOBusGetTypeID(void);

Emex64MMIOBusRef Emex64MMIOBusCreate(EFAllocatorRef allocatorRef);

bool Emex64MMIOBusRegisterRegion(Emex64MMIOBusRef MMIOBusRef, Emex64MMIORegionRef MMIORegionRef);
Emex64MMIORegionRef Emex64MMIOBusGetRegionForAddress(Emex64MMIOBusRef MMIOBusRef, uint64_t addr);

#endif /* EMEX64VM_MMIO_H */
