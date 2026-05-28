/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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
#include <emex64vm/mmio.h>

la64_mmio_bus_t *la64_mmio_alloc(void)
{
    return calloc(1, sizeof(la64_mmio_bus_t));
}

void la64_mmio_dealloc(la64_mmio_bus_t *bus)
{
    free(bus);
}

bool la64_mmio_register(la64_mmio_bus_t *bus,
                        uint64_t base,
                        uint64_t size,
                        void *device,
                        mmio_read_fn read,
                        mmio_write_fn write)
{
    assert(bus->region_count < MAX_MMIO_REGIONS);

    /* overlap check */
    for(int i = 0; i < bus->region_count; i++)
    {
        la64_mmio_region_t *r = &bus->regions[i];
        if(base < r->base_addr + r->size &&
           base + size > r->base_addr)
        {
            return false;
        }
    }

    /* setup mmio region */
    la64_mmio_region_t *region = &bus->regions[bus->region_count++];
    region->base_addr = base;
    region->size = size;
    region->device = device;
    region->read = read;
    region->write = write;

    /* check and set addresses */
    if(bus->start_addr > base)
    {
        bus->start_addr = base;
    }

    uint64_t end_addr_canditate = base + size;
    if(bus->end_addr < end_addr_canditate)
    {
        bus->end_addr = end_addr_canditate;
    }

    return true;
}

la64_mmio_region_t *la64_mmio_find(la64_mmio_bus_t *bus,
                                   uint64_t addr)
{
    /* sanity check */
    if(addr < bus->start_addr ||
       addr > bus->end_addr)
    {
        return NULL;
    }

    /* fast path */
    if(bus->last_region != NULL &&
        addr >= bus->last_region->base_addr &&
        addr < bus->last_region->base_addr + bus->last_region->size)
    {
        return bus->last_region;
    }

    /* finding mmio region, hopefully x3 */
    for(int i = 0; i < bus->region_count; i++)
    {
        la64_mmio_region_t *r = &bus->regions[i];
        if(addr >= r->base_addr &&
           addr < r->base_addr + r->size)
        {
            bus->last_region = r;
            return r;
        }
    }

    return NULL;
}
