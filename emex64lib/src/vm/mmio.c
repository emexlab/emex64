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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <emex64lib/vm/mmio.h>
#include <EmexFoundation/EmexFoundation.h>

uint64_t emex64_mmio_fallback_read(emex64_core_t *core,
                                   void *device,
                                   uint64_t offset,
                                   int size)
{
    return 0;
}

void emex64_mmio_fallback_write(emex64_core_t *core,
                                void *device,
                                uint64_t offset,
                                uint64_t value,
                                int size)
{
    return;
}

typedef struct Emex64Region {
    EFObject header;
    uint64_t base_addr;
    uint64_t size;
    void *device;
    mmio_read_fn read;
    mmio_write_fn write;
} *Emex64MMIORegion;

static EFClass Emex64MMIORegionClass = {
    .name = "Emex64MMIORegion",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = NULL,
    .equal = NULL,
};

static void Emex64MMIORegionRegisterClass(void)
{
    EFClassRegister(&Emex64MMIORegionClass);
}

EFTypeID Emex64MMIORegionGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, Emex64MMIORegionRegisterClass);
    return Emex64MMIORegionClass.typeID;
}

Emex64MMIORegionRef Emex64MMIORegionCreate(EFAllocatorRef allocatorRef,
                                           uint64_t base,
                                           uint64_t size,
                                           void *device,
                                           mmio_read_fn read,
                                           mmio_write_fn write)
{
    Emex64MMIORegion MMIORegion = EFObjectAlloc(allocatorRef, Emex64MMIORegionGetTypeID(), sizeof(struct Emex64Region));
    if(MMIORegion == NULL)
    {
        return NULL;
    }

    MMIORegion->base_addr = base;
    MMIORegion->size = size;
    MMIORegion->device = device;
    MMIORegion->read = read;
    MMIORegion->write = write;

    return (Emex64MMIORegionRef)MMIORegion;
}

uint64_t Emex64MMIORegionGetBaseAddress(Emex64MMIORegionRef MMIORegionRef)
{
    Emex64MMIORegion MMIORegion = (Emex64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return 0;
    }

    return MMIORegion->base_addr;
}

uint64_t Emex64MMIORegionGetSize(Emex64MMIORegionRef MMIORegionRef)
{
    Emex64MMIORegion MMIORegion = (Emex64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return 0;
    }

    return MMIORegion->size;
}

void *Emex64MMIORegionGetDevice(Emex64MMIORegionRef MMIORegionRef)
{
    Emex64MMIORegion MMIORegion = (Emex64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return NULL;
    }

    return MMIORegion->device;
}

mmio_read_fn Emex64MMIORegionGetReadSymbol(Emex64MMIORegionRef MMIORegionRef)
{
    Emex64MMIORegion MMIORegion = (Emex64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return emex64_mmio_fallback_read;
    }

    return MMIORegion->read;
}

mmio_write_fn Emex64MMIORegionGetWriteSymbol(Emex64MMIORegionRef MMIORegionRef)
{
    Emex64MMIORegion MMIORegion = (Emex64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return emex64_mmio_fallback_write;
    }

    return MMIORegion->write;
}

typedef struct Emex64MMIOBus {
    EFObject header;
    Emex64MMIORegion last_region;
    Emex64MMIORegion regions[MAX_MMIO_REGIONS];
    int region_count;
} *Emex64MMIOBus;

static void __Emex64MMIOBusDeinit(Emex64MMIOBusRef MMIOBusRef)
{
    Emex64MMIOBus MMIOBus = (Emex64MMIOBus)MMIOBusRef;
    if(MMIOBus == NULL || MMIOBus->region_count >= MAX_MMIO_REGIONS)
    {
        return;
    }

    for(int i = 0; i < MMIOBus->region_count; i++)
    {
        EFRelease(MMIOBus->regions[i]);
    }
}

static EFClass Emex64MMIOBusClass = {
    .name = "Emex64MMIOBus",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __Emex64MMIOBusDeinit,
    .equal = NULL,
};

static void Emex64MMIOBusRegisterClass(void)
{
    EFClassRegister(&Emex64MMIOBusClass);
}

EFTypeID Emex64MMIOBusGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, Emex64MMIOBusRegisterClass);
    return Emex64MMIOBusClass.typeID;
}

Emex64MMIOBusRef Emex64MMIOBusCreate(EFAllocatorRef allocatorRef)
{
    Emex64MMIOBus MMIOBus = EFObjectAlloc(allocatorRef, Emex64MMIOBusGetTypeID(), sizeof(struct Emex64MMIOBus));
    if(MMIOBus == NULL)
    {
        return NULL;
    }

    MMIOBus->last_region = NULL;
    MMIOBus->region_count = 0;

    return (Emex64MMIOBusRef)MMIOBus;
}

bool Emex64MMIOBusRegisterRegion(Emex64MMIOBusRef MMIOBusRef,
                                 Emex64MMIORegionRef MMIORegionRef)
{

    Emex64MMIOBus MMIOBus = (Emex64MMIOBus)MMIOBusRef;
    if(MMIOBus == NULL || MMIOBus->region_count >= MAX_MMIO_REGIONS)
    {
        return false;
    }

    /* region registration */
    MMIORegionRef = EFRetain(MMIORegionRef);
    if(MMIORegionRef == NULL)
    {
        return false;
    }
    MMIOBus->regions[MMIOBus->region_count++] = MMIORegionRef;

    return true;
}

Emex64MMIORegionRef Emex64MMIOBusGetRegionForAddress(Emex64MMIOBusRef MMIOBusRef,
                                                     uint64_t addr)
{
    Emex64MMIOBus MMIOBus = (Emex64MMIOBus)MMIOBusRef;
    if(MMIOBus == NULL)
    {
        return NULL;
    }

    /* fast path */
    if(MMIOBus->last_region != NULL &&
       addr >= MMIOBus->last_region->base_addr &&
       addr < MMIOBus->last_region->base_addr + MMIOBus->last_region->size)
    {
        return MMIOBus->last_region;
    }

    /* finding mmio region, hopefully x3 */
    for(int i = 0; i < MMIOBus->region_count; i++)
    {
        Emex64MMIORegion r = MMIOBus->regions[i];
        if(addr >= r->base_addr &&
           addr < r->base_addr + r->size)
        {
            MMIOBus->last_region = r;
            return r;
        }
    }

    return NULL;
}
