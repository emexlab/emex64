/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <emex64lib/vm/mmio.h>
#include <evObj/evObj.h>

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

typedef struct Emex64MMIOBus {
    EVObject header;
    emex64_mmio_region_t *last_region;
    emex64_mmio_region_t regions[MAX_MMIO_REGIONS];
    int region_count;
} *Emex64MMIOBus;

static EVClass Emex64MMIOBusClass = {
    .name = "Emex64MMIOBus",
    .typeID = kEVNotATypeID,
    .size = sizeof(struct Emex64MMIOBus),
    .init = NULL,
    .deinit = NULL,
    .copy = NULL,
    .equal = NULL,
};

static void Emex64MMIOBusRegisterClass(void)
{
    EVClassRegister(&Emex64MMIOBusClass);
}

EVTypeID Emex64MMIOBusGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, Emex64MMIOBusRegisterClass);
    return Emex64MMIOBusClass.typeID;
}

Emex64MMIOBusRef Emex64MMIOBusCreate(EVAllocator *allocator)
{
    Emex64MMIOBus MMIOBus = EVObjectAlloc(allocator, Emex64MMIOBusGetTypeID());
    if(MMIOBus == NULL)
    {
        return NULL;
    }

    MMIOBus->last_region = NULL;
    MMIOBus->region_count = 0;

    return (Emex64MMIOBusRef)MMIOBus;
}

bool Emex64MMIOBusRegisterDevice(Emex64MMIOBusRef MMIOBusRef,
                                 uint64_t base,
                                 uint64_t size,
                                 void *device,
                                 mmio_read_fn read,
                                 mmio_write_fn write)
{

    Emex64MMIOBus MMIOBus = (Emex64MMIOBus)MMIOBusRef;
    if(MMIOBus == NULL || MMIOBus->region_count >= MAX_MMIO_REGIONS)
    {
        return false;
    }

    /* region registration */
    emex64_mmio_region_t *region = &MMIOBus->regions[MMIOBus->region_count++];
    region->base_addr = base;
    region->size = size;
    region->device = device;
    region->read = read ? read : emex64_mmio_fallback_read;
    region->write = write ? write : emex64_mmio_fallback_write;

    return true;
}

emex64_mmio_region_t *Emex64MMIOBusGetRegionForAddress(Emex64MMIOBusRef MMIOBusRef,
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
        emex64_mmio_region_t *r = &MMIOBus->regions[i];
        if(addr >= r->base_addr &&
           addr < r->base_addr + r->size)
        {
            MMIOBus->last_region = r;
            return r;
        }
    }

    return NULL;
}
