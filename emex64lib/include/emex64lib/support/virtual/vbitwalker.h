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

#ifndef VBITWALKER_H
#define VBITWALKER_H

#include <emex64lib/support/virtual/vfd.h>
#include <emex64lib/support/endian.h>

typedef struct {
    vfd_t *d;
    size_t byte_pos;
    uint8_t bit_idx;
    bw_endian_t endian;
} vbitwalker_t;

vbitwalker_t *vbitwalker_alloc(vfd_t *d, bw_endian_t endian);
void vbitwalker_dealloc(vbitwalker_t *fw);

void vbitwalker_reset(vbitwalker_t *fw);

int vbitwalker_write(vbitwalker_t *fw, uint64_t value, uint8_t num_bits);
uint64_t vbitwalker_read(vbitwalker_t *fw, uint8_t num_bits);
int vbitwalker_write_buf(vbitwalker_t *fw, const char *buf, size_t len);
int vbitwalker_read_buf(vbitwalker_t *fw, char *buf, size_t len);

void vbitwalker_seek(vbitwalker_t *fw, size_t byte_pos, uint8_t bit_idx);

void vbitwalker_skip(vbitwalker_t *fw, size_t num_bits);

size_t vbitwalker_bytes_used(const vbitwalker_t *fw);

void vbitwalker_align_byte(vbitwalker_t *fw);

void vbitwalker_sync(vbitwalker_t *fw);

#endif /* VBITWALKER_H */
