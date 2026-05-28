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

#ifndef EMEXUTILS_FDWALKER_H
#define EMEXUTILS_FDWALKER_H

#include <emexutils/bitwalker.h>

typedef struct {
    int         fd;
    size_t      byte_pos;
    uint8_t     bit_idx;
    bw_endian_t endian;
} fdwalker_t;

void fdwalker_init(fdwalker_t *fw, int fd, bw_endian_t endian);
//void fdwalker_init_read(fdwalker_t *fw, const uint8_t *buf, size_t len, bw_endian_t endian);

void fdwalker_reset(fdwalker_t *fw);

int fdwalker_write(fdwalker_t *fw, uint64_t value, uint8_t num_bits);
uint64_t fdwalker_read(fdwalker_t *fw, uint8_t num_bits);
int fdwalker_write_buf(fdwalker_t *fw, const char *buf, size_t len);
int fdwalker_read_buf(fdwalker_t *fw, char *buf, size_t len);

void fdwalker_seek(fdwalker_t *fw, size_t byte_pos, uint8_t bit_idx);

void fdwalker_skip(fdwalker_t *fw, size_t num_bits);

size_t fdwalker_bytes_used(const fdwalker_t *fw);

void fdwalker_align_byte(fdwalker_t *fw);

#endif /* EMEXUTILS_FDWALKER_H */
