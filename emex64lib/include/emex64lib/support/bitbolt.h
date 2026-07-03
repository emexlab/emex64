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

#include <stdint.h>
#include <string.h>

extern uint64_t kMask[65];

typedef struct {
    const uint8_t *buf;
    uint32_t pos;
} bitbolt_t;

static inline uint64_t cpy64le(const uint8_t *p)
{
    return  (uint64_t)p[0] | ((uint64_t)p[1] <<  8) | ((uint64_t)p[2] << 16) | ((uint64_t)p[3] << 24) | ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40) | ((uint64_t)p[6] << 48) | ((uint64_t)p[7] << 56);
}

static inline uint64_t bb_read(bitbolt_t *bb,
                               unsigned n)
{
    const uint8_t *p = (const uint8_t *)bb->buf + (bb->pos >> 3);
    uint64_t lo = cpy64le(p);
    uint64_t hi = cpy64le(p + 8);

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

#endif /* EMEX64_BITBOLT_H */
