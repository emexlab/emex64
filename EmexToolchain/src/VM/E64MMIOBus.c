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
#include <pthread.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64MMIOBus.h>

static void __E64MMIOBusDeinit(E64MMIOBusRef MMIOBusRef)
{
    __E64MMIOBus MMIOBus = (__E64MMIOBus)MMIOBusRef;
    if(MMIOBus->regions != NULL)
    {
        EFRelease(MMIOBus->regions);
    }
}

static EFClassDefinitionV2 E64MMIOBusClass = {
    .header = {
        .version = 2,
        .typeID = kEFTypeIDNone,
        .name = NULL,
    },
    .name = "E64MMIOBus",
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
    return E64MMIOBusClass.header.typeID;
}

E64MMIOBusRef E64MMIOBusCreate(EFAllocatorRef allocatorRef)
{
    EFAUTOREL __E64MMIOBus MMIOBus = (__E64MMIOBus)EFObjectCreate(allocatorRef, E64MMIOBusGetTypeID(), (EFIndex)sizeof(struct __E64MMIOBus));
    if(MMIOBus == NULL)
    {
        return NULL;
    }

    MMIOBus->lastRegionIndex = 0;
    MMIOBus->regions = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksObjectCallbacks, 0);
    if(MMIOBus->regions == NULL)
    {
        return NULL;
    }

    return (E64MMIOBusRef)EFAUTOTRANSFER(MMIOBus);
}

Boolean E64MMIOBusRegisterRegion(E64MMIOBusRef MMIOBusRef,
                                 E64MMIORegionRef MMIORegionRef)
{
    __E64MMIOBus MMIOBus = (__E64MMIOBus)MMIOBusRef;
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
                                               UInt64 addr)
{
    __E64MMIOBus MMIOBus = (__E64MMIOBus)MMIOBusRef;
    if(MMIOBus == NULL)
    {
        return NULL;
    }

    /* fast path */
    __E64MMIORegion lastMMIORegion = (__E64MMIORegion)EFArrayGetValueAtIndex(MMIOBus->regions, MMIOBus->lastRegionIndex);
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
        __E64MMIORegion MMIORegion = (__E64MMIORegion)EFArrayGetValueAtIndex(MMIOBus->regions, index);
        if(addr >= MMIORegion->base_addr && addr < MMIORegion->base_addr + MMIORegion->size)
        {
            MMIOBus->lastRegionIndex = index;
            return MMIORegion;
        }
    }

    return NULL;
}
