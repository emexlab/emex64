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

typedef EFObjectRef E64MMIORegionRef;

EFTypeID E64MMIORegionGetTypeID(void);

E64MMIORegionRef E64MMIORegionCreate(EFAllocatorRef allocatorRef, uint64_t base, uint64_t size, void *device, mmio_read_fn read, mmio_write_fn write);

uint64_t E64MMIORegionGetBaseAddress(E64MMIORegionRef MMIORegionRef);
uint64_t E64MMIORegionGetSize(E64MMIORegionRef MMIORegionRef);
void *E64MMIORegionGetDevice(E64MMIORegionRef MMIORegionRef);
mmio_read_fn E64MMIORegionGetReadSymbol(E64MMIORegionRef MMIORegionRef);
mmio_write_fn E64MMIORegionGetWriteSymbol(E64MMIORegionRef MMIORegionRef);

typedef EFObjectRef E64MMIOBusRef;

EFTypeID E64MMIOBusGetTypeID(void);

E64MMIOBusRef E64MMIOBusCreate(EFAllocatorRef allocatorRef);

bool E64MMIOBusRegisterRegion(E64MMIOBusRef MMIOBusRef, E64MMIORegionRef MMIORegionRef);
E64MMIORegionRef E64MMIOBusGetRegionForAddress(E64MMIOBusRef MMIOBusRef, uint64_t addr);

#endif /* EMEX64VM_MMIO_H */
