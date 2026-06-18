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

#ifndef VPAGE_H
#define VPAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

typedef struct vpage {
    uint8_t *p;
    size_t len;
    struct vpage *prev;
    struct vpage *next;
} vpage_t;

vpage_t *vpage_alloc();
void vpage_dealloc(vpage_t *p);
vpage_t *vpage_copy(vpage_t *p);

size_t vpage_get_size(vpage_t *p);
bool vpage_gib_page(vpage_t *p);
bool vpage_bind_page(vpage_t *p);

size_t vpage_write(vpage_t *p, size_t off, const uint8_t *b, size_t len);
size_t vpage_read(vpage_t *p, size_t off, uint8_t *b, size_t len);

void *vpage_mmap_anonymous_copy(vpage_t *p);

#endif /* VPAGE_H */
