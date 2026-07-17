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

#include <EmexToolchain/VM/E64MMIORegion.h>

UInt64 emex64_mmio_fallback_read(E64CoreRef core,
                                 void *device,
                                 UInt64 offset,
                                 int size)
{
    return 0;
}

void emex64_mmio_fallback_write(E64CoreRef core,
                                void *device,
                                UInt64 offset,
                                UInt64 value,
                                int size)
{
    return;
}

static EFClass E64MMIORegionClass = {
    .name = "E64MMIORegion",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = NULL,
    .equal = NULL,
    .copyDescription = NULL,
};

static void E64MMIORegionRegisterClass(void)
{
    EFClassRegister(&E64MMIORegionClass);
}

EFTypeID E64MMIORegionGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, E64MMIORegionRegisterClass);
    return E64MMIORegionClass.typeID;
}

E64MMIORegionRef E64MMIORegionCreate(EFAllocatorRef allocatorRef,
                                     UInt64 base,
                                     UInt64 size,
                                     void *device,
                                     mmio_read_fn read,
                                     mmio_write_fn write)
{
    __E64MMIORegion MMIORegion = (__E64MMIORegion)EFObjectCreate(allocatorRef, E64MMIORegionGetTypeID(), (EFIndex)sizeof(struct __E64MMIORegion));
    if(MMIORegion == NULL)
    {
        return NULL;
    }

    MMIORegion->base_addr = base;
    MMIORegion->size = size;
    MMIORegion->device = device;
    MMIORegion->read = read;
    MMIORegion->write = write;

    return (E64MMIORegionRef)MMIORegion;
}

UInt64 E64MMIORegionGetBaseAddress(E64MMIORegionRef MMIORegionRef)
{
    __E64MMIORegion MMIORegion = (__E64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return 0;
    }

    return MMIORegion->base_addr;
}

UInt64 E64MMIORegionGetSize(E64MMIORegionRef MMIORegionRef)
{
    __E64MMIORegion MMIORegion = (__E64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return 0;
    }

    return MMIORegion->size;
}

void *E64MMIORegionGetDevice(E64MMIORegionRef MMIORegionRef)
{
    __E64MMIORegion MMIORegion = (__E64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return NULL;
    }

    return MMIORegion->device;
}

mmio_read_fn E64MMIORegionGetReadSymbol(E64MMIORegionRef MMIORegionRef)
{
    __E64MMIORegion MMIORegion = (__E64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return emex64_mmio_fallback_read;
    }

    return MMIORegion->read;
}

mmio_write_fn E64MMIORegionGetWriteSymbol(E64MMIORegionRef MMIORegionRef)
{
    __E64MMIORegion MMIORegion = (__E64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return emex64_mmio_fallback_write;
    }

    return MMIORegion->write;
}
