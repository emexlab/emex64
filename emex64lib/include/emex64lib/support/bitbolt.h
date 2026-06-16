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

#ifndef BITBOLT_H
#define BITBOLT_H

#include <stdint.h>
#include <string.h>

extern uint64_t kMask[65];

typedef struct {
    const uint8_t *buf;
    uint32_t pos;
} bitbolt_t;

static inline uint64_t bb_read(bitbolt_t *bb,
                               unsigned n)
{
    uint64_t lo, hi;
    memcpy(&lo, bb->buf + (bb->pos >> 3), sizeof lo);
    memcpy(&hi, bb->buf + (bb->pos >> 3) + 8, sizeof hi);

    unsigned shift = bb->pos & 7;

    uint64_t v;
    if(shift == 0)
    {
        v = lo;
    }
    else
    {
        v = (lo >> shift) | (hi << (64 - shift));
    }

    v &= kMask[n];
    bb->pos += n;
    return v;
}

static inline void bb_align(bitbolt_t *bb)
{
    bb->pos = (bb->pos + 7u) & ~7u;
}

#endif /* BITBOLT_H */
