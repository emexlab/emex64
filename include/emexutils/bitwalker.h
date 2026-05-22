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

#ifndef BITWALKER_H
#define BITWALKER_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef enum {
    BW_LITTLE_ENDIAN,
    BW_BIG_ENDIAN
} bw_endian_t;

typedef struct {
    uint8_t    *buffer;
    size_t      byte_pos;
    uint8_t     bit_idx;
    size_t      capacity;
    bw_endian_t endian;
} bitwalker_t;

bw_endian_t bw_host_endian(void);
uint64_t bw_swap_n(uint64_t v, uint8_t num_bytes);

void bitwalker_init(bitwalker_t *bw, uint8_t *buf, size_t capacity, bw_endian_t endian);
void bitwalker_init_read(bitwalker_t *bw, const uint8_t *buf, size_t len, bw_endian_t endian);

void bitwalker_reset(bitwalker_t *bw);

int bitwalker_write(bitwalker_t *bw, uint64_t value, uint8_t num_bits);
uint64_t bitwalker_read(bitwalker_t *bw, uint8_t num_bits);

void bitwalker_skip(bitwalker_t *bw, size_t num_bits);

size_t bitwalker_bytes_used(const bitwalker_t *bw);

void bitwalker_align_byte(bitwalker_t *bw);

#endif /* BITWALKER_H */
