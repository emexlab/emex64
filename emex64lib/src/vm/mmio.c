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

typedef struct E64Region {
    EFObject header;
    uint64_t base_addr;
    uint64_t size;
    void *device;
    mmio_read_fn read;
    mmio_write_fn write;
} *E64MMIORegion;

static EFClass E64MMIORegionClass = {
    .name = "E64MMIORegion",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = NULL,
    .equal = NULL,
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
                                           uint64_t base,
                                           uint64_t size,
                                           void *device,
                                           mmio_read_fn read,
                                           mmio_write_fn write)
{
    E64MMIORegion MMIORegion = EFObjectAlloc(allocatorRef, E64MMIORegionGetTypeID(), sizeof(struct E64Region));
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

uint64_t E64MMIORegionGetBaseAddress(E64MMIORegionRef MMIORegionRef)
{
    E64MMIORegion MMIORegion = (E64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return 0;
    }

    return MMIORegion->base_addr;
}

uint64_t E64MMIORegionGetSize(E64MMIORegionRef MMIORegionRef)
{
    E64MMIORegion MMIORegion = (E64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return 0;
    }

    return MMIORegion->size;
}

void *E64MMIORegionGetDevice(E64MMIORegionRef MMIORegionRef)
{
    E64MMIORegion MMIORegion = (E64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return NULL;
    }

    return MMIORegion->device;
}

mmio_read_fn E64MMIORegionGetReadSymbol(E64MMIORegionRef MMIORegionRef)
{
    E64MMIORegion MMIORegion = (E64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return emex64_mmio_fallback_read;
    }

    return MMIORegion->read;
}

mmio_write_fn E64MMIORegionGetWriteSymbol(E64MMIORegionRef MMIORegionRef)
{
    E64MMIORegion MMIORegion = (E64MMIORegion)MMIORegionRef;
    if(MMIORegion == NULL)
    {
        return emex64_mmio_fallback_write;
    }

    return MMIORegion->write;
}

typedef struct E64MMIOBus {
    EFObject header;
    EFIndex lastRegionIndex;
    EFMutableArrayRef regions;
} *E64MMIOBus;

static void __E64MMIOBusDeinit(E64MMIOBusRef MMIOBusRef)
{
    E64MMIOBus MMIOBus = (E64MMIOBus)MMIOBusRef;
    if(MMIOBus->regions != NULL)
    {
        EFRelease(MMIOBus->regions);
    }
}

static EFClass E64MMIOBusClass = {
    .name = "E64MMIOBus",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __E64MMIOBusDeinit,
    .equal = NULL,
};

static void E64MMIOBusRegisterClass(void)
{
    EFClassRegister(&E64MMIOBusClass);
}

EFTypeID E64MMIOBusGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, E64MMIOBusRegisterClass);
    return E64MMIOBusClass.typeID;
}

E64MMIOBusRef E64MMIOBusCreate(EFAllocatorRef allocatorRef)
{
    E64MMIOBus MMIOBus = EFObjectAlloc(allocatorRef, E64MMIOBusGetTypeID(), sizeof(struct E64MMIOBus));
    if(MMIOBus == NULL)
    {
        return NULL;
    }

    MMIOBus->lastRegionIndex = 0;
    MMIOBus->regions = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksObjectCallbacks, 0);
    if(MMIOBus->regions == NULL)
    {
        EFRelease(MMIOBus);
        return NULL;
    }

    return (E64MMIOBusRef)MMIOBus;
}

bool E64MMIOBusRegisterRegion(E64MMIOBusRef MMIOBusRef,
                                 E64MMIORegionRef MMIORegionRef)
{

    E64MMIOBus MMIOBus = (E64MMIOBus)MMIOBusRef;
    if(MMIOBus == NULL || MMIORegionRef == NULL || EFGetTypeID(MMIORegionRef) != E64MMIORegionGetTypeID())
    {
        return false;
    }

    /* region registration */
    if(!EFArrayAppendValue(MMIOBus->regions, MMIORegionRef))
    {
        return false;
    }

    return true;
}

E64MMIORegionRef E64MMIOBusGetRegionForAddress(E64MMIOBusRef MMIOBusRef,
                                                     uint64_t addr)
{
    E64MMIOBus MMIOBus = (E64MMIOBus)MMIOBusRef;
    if(MMIOBus == NULL)
    {
        return NULL;
    }

    /* fast path */
    E64MMIORegion lastMMIORegion = (E64MMIORegion)EFArrayGetValueAtIndex(MMIOBus->regions, MMIOBus->lastRegionIndex);
    if(lastMMIORegion != NULL)
    {
        if(addr >= lastMMIORegion->base_addr && addr < lastMMIORegion->base_addr + lastMMIORegion->size)
        {
            return (E64MMIORegionRef)lastMMIORegion;
        }
    }

    /* finding mmio region, hopefully x3 */
    EFIndex count = EFArrayGetCount(MMIOBus->regions);
    for(EFIndex index = 0; index < count; index++)
    {
        E64MMIORegion MMIORegion = (E64MMIORegion)EFArrayGetValueAtIndex(MMIOBus->regions, index);
        if(addr >= MMIORegion->base_addr && addr < MMIORegion->base_addr + MMIORegion->size)
        {
            MMIOBus->lastRegionIndex = index;
            return MMIORegion;
        }
    }

    return NULL;
}
