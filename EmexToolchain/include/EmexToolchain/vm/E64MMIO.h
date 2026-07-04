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

#ifndef E64MMIO_H
#define E64MMIO_H

#include <stdint.h>
#include <stdbool.h>
#include <EmexFoundation/EmexFoundation.h>

typedef struct __E64Core *E64CoreRef;

typedef UInt64 (*mmio_read_fn)(E64CoreRef core, void *device, UInt64 offset, int size);
typedef void (*mmio_write_fn)(E64CoreRef core, void *device, UInt64 offset, UInt64 value, int size);

typedef EFObjectRef E64MMIORegionRef;

EFTypeID E64MMIORegionGetTypeID(void);

E64MMIORegionRef E64MMIORegionCreate(EFAllocatorRef allocatorRef, UInt64 base, UInt64 size, void *device, mmio_read_fn read, mmio_write_fn write);

UInt64 E64MMIORegionGetBaseAddress(E64MMIORegionRef MMIORegionRef);
UInt64 E64MMIORegionGetSize(E64MMIORegionRef MMIORegionRef);
void *E64MMIORegionGetDevice(E64MMIORegionRef MMIORegionRef);
mmio_read_fn E64MMIORegionGetReadSymbol(E64MMIORegionRef MMIORegionRef);
mmio_write_fn E64MMIORegionGetWriteSymbol(E64MMIORegionRef MMIORegionRef);

typedef EFObjectRef E64MMIOBusRef;

EFTypeID E64MMIOBusGetTypeID(void);

E64MMIOBusRef E64MMIOBusCreate(EFAllocatorRef allocatorRef);

Boolean E64MMIOBusRegisterRegion(E64MMIOBusRef MMIOBusRef, E64MMIORegionRef MMIORegionRef);
E64MMIORegionRef E64MMIOBusGetRegionForAddress(E64MMIOBusRef MMIOBusRef, UInt64 addr);

#endif /* E64MMIO_H */
