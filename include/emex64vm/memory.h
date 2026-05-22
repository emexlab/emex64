/*
 * MIT License
 *
 * Copyright (c) 2024 cr4zyengineer
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

#ifndef LA64VM_MEMORY_H
#define LA64VM_MEMORY_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <emex64vm/core.h>

typedef struct la64_memory {
    uint8_t *memory;
    uint64_t memory_size;
} la64_memory_t;

la64_memory_t *la64_memory_alloc(uint64_t size);
void la64_memory_dealloc(la64_memory_t *memory);

bool la64_memory_load_image(la64_memory_t *memory, const char *image_path);

void *la64_memory_access(la64_core_t *core, uint64_t addr, size_t size);
bool la64_memory_read(la64_core_t *core, uint64_t addr, size_t size, uint64_t *value);
bool la64_memory_write(la64_core_t *core, uint64_t addr, uint64_t value, size_t size);

#endif /* LA64VM_MEMORY_H */
