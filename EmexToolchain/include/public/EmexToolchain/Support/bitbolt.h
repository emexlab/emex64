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

extern UInt64 kMask[65];

typedef struct {
    const UInt8 *buf;
    UInt32 pos;
} bitbolt_t;

static inline UInt64 cpy64le(const UInt8 *p)
{
    return  (UInt64)p[0] | ((UInt64)p[1] <<  8) | ((UInt64)p[2] << 16) | ((UInt64)p[3] << 24) | ((UInt64)p[4] << 32) | ((UInt64)p[5] << 40) | ((UInt64)p[6] << 48) | ((UInt64)p[7] << 56);
}

static inline UInt64 bb_read(bitbolt_t *bb,
                               unsigned n)
{
    const UInt8 *p = (const UInt8 *)bb->buf + (bb->pos >> 3);
    UInt64 lo = cpy64le(p);
    UInt64 hi = cpy64le(p + 8);

    unsigned shift = bb->pos & 7;

    UInt64 v;
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

#endif /* EMEX64_BITBOLT_H */
