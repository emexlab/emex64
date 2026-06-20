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

#ifndef EMEX64ASM_ENDIAN_H
#define EMEX64ASM_ENDIAN_H

#include <stddef.h>
#include <stdint.h>

#define BW_LITTLE_ENDIAN 0
#define BW_BIG_ENDIAN 1

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#define BW_HOST_ENDIAN  BW_BIG_ENDIAN
#else
#define BW_HOST_ENDIAN  BW_LITTLE_ENDIAN
#endif

#if BW_HOST_ENDIAN == BW_BIG_ENDIAN
#define TO_HOST16(x) __builtin_bswap16(x)
#define TO_HOST32(x) __builtin_bswap32(x)
#define TO_HOST64(x) __builtin_bswap64(x)
#else
#define TO_HOST16(x) (x)
#define TO_HOST32(x) (x)
#define TO_HOST64(x) (x)
#endif

typedef uint8_t bw_endian_t;

static inline uint64_t bswap_n(uint64_t v,
                               uint8_t num_bytes)
{
    switch(num_bytes)
    {
        case 2: return __builtin_bswap16((uint16_t)v);
        case 3: return ((v >> 16) & 0xFF) | (v & 0xFF00) | ((v & 0xFF) << 16);
        case 4: return __builtin_bswap32((uint32_t)v);
        case 5:
        case 6:
        case 7:
        case 8: return __builtin_bswap64(v) >> ((8 - num_bytes) * 8);
        default: return v;
    }
}

static inline __uint128_t load_window_le(const uint8_t *p,
                                         size_t n)
{
    __uint128_t v = 0;
    for(size_t i = 0; i < n; i++)
    {
        v |= (__uint128_t)p[i] << (8 * i);
    }
    return v;
}

static inline void store_window_le(uint8_t *p,
                                   __uint128_t v,
                                   size_t n)
{
    for(size_t i = 0; i < n; i++)
    {
        p[i] = (uint8_t)(v >> (8 * i));
    }
}

#endif /* EMEX64ASM_ENDIAN_H */
