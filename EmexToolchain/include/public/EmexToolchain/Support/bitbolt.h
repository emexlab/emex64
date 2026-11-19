/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EMEX64_BITBOLT_H
#define EMEX64_BITBOLT_H

#include <string.h>
#include <EmexFoundation/EmexFoundation.h>

typedef struct {
    const UInt8 *buf;
    UInt32 pos;
    UInt32 have;
    UInt64 win;
} bitbolt_t;

__attribute__((always_inline)) UInt64 bb_load64(const UInt8 *p)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    UInt64 v;
    memcpy(&v, p, 8);
    return v;
#else
    return  (UInt64)p[0] | ((UInt64)p[1] <<  8) | ((UInt64)p[2] << 16) | ((UInt64)p[3] << 24) | ((UInt64)p[4] << 32) | ((UInt64)p[5] << 40) | ((UInt64)p[6] << 48) | ((UInt64)p[7] << 56);
#endif /* defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ */
}

__attribute__((always_inline)) UInt64 bb_mask(unsigned n)
{
    return n >= 64 ? ~0ULL : ((1ULL << n) - 1ULL);
}

__attribute__((always_inline)) UInt64 bb_read(bitbolt_t *bb,
                                              unsigned n)
{
    if(__builtin_expect(n > bb->have, 0))
    {
        const UInt8 *p = bb->buf + (bb->pos >> 3);
        unsigned shift = bb->pos & 7;
        UInt64 lo = bb_load64(p);

        if(__builtin_expect(n > 64 - shift, 0))
        {
            UInt64 v = ((lo >> shift) | ((UInt64)p[8] << (64 - shift))) & bb_mask(n);
            bb->pos += n;
            bb->have = 0;
            return v;
        }

        bb->win = lo >> shift;
        bb->have = 64 - shift;
    }

    UInt64 v = bb->win & bb_mask(n);
    bb->win = n >= 64 ? 0 : (bb->win >> n);
    bb->have -= n;
    bb->pos += n;
    return v;
}

__attribute__((always_inline)) void bb_align(bitbolt_t *bb)
{
    UInt32 skip = (0u - bb->pos) & 7u;
    if(skip <= bb->have)
    {
        bb->win >>= skip;
        bb->have -= skip;
    }
    else
    {
        bb->have = 0;
    }
    bb->pos += skip;
}

#endif /* EMEX64_BITBOLT_H */
