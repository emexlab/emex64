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

#ifndef EMEX64VM_MMIO_H
#define EMEX64VM_MMIO_H

#include <stdint.h>
#include <stdbool.h>
#include <evObj/evObj.h>

typedef struct emex64_core emex64_core_t;

typedef uint64_t (*mmio_read_fn)(emex64_core_t *core, void *device, uint64_t offset, int size);
typedef void (*mmio_write_fn)(emex64_core_t *core, void *device, uint64_t offset, uint64_t value, int size);

typedef struct {
    uint64_t base_addr;
    uint64_t size;
    void *device;
    mmio_read_fn read;
    mmio_write_fn write;
} emex64_mmio_region_t;

#define MAX_MMIO_REGIONS 32

typedef EVObjectRef Emex64MMIOBusRef;

Emex64MMIOBusRef Emex64MMIOBusCreate(EVAllocator *allocator);

bool Emex64MMIOBusRegisterDevice(Emex64MMIOBusRef MMIOBusRef, uint64_t base, uint64_t size, void *device, mmio_read_fn read, mmio_write_fn write);
emex64_mmio_region_t *Emex64MMIOBusGetRegionForAddress(Emex64MMIOBusRef MMIOBusRef, uint64_t addr);

#endif /* EMEX64VM_MMIO_H */
