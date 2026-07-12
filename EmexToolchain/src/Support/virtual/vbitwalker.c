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

#include <stdlib.h>
#include <assert.h>
#include <EmexToolchain/Support/virtual/vbitwalker.h>

vbitwalker_t *vbitwalker_alloc(vfd_t *d,
                               bw_endian_t endian)
{
    vbitwalker_t *vb = malloc(sizeof(vbitwalker_t));
    if(vb == NULL)
    {
        return NULL;
    }

    vfd_t *nd = vfd_dup(d);
    if(nd == NULL)
    {
        free(vb);
        return NULL;
    }

    /* setting properties */
    vb->d = nd;
    vb->byte_pos = 0;
    vb->bit_idx = 0;
    vb->endian = endian;

    return vb;
}

void vbitwalker_dealloc(vbitwalker_t *vb)
{
    if(vb == NULL)
    {
        return;
    }

    vfd_close(vb->d);
    free(vb);
}

void vbitwalker_reset(vbitwalker_t *vb)
{
    vb->byte_pos = 0;
    vb->bit_idx = 0;
}

int vbitwalker_write(vbitwalker_t *vb,
                     UInt64 value,
                     UInt8 num_bits)
{
    assert(vb != NULL);

    if(num_bits == 0 || num_bits > 64)
    {
        return -1;
    }

    UInt64 mask = (num_bits == 64) ? ~0ULL : ((1ULL << num_bits) - 1);
    value &= mask;

    /*
     * for multi-byte values we gonna have to convert host endian to target endian
     * in case they aint the same
     */
    if(num_bits > 8)
    {
        UInt8 num_bytes = (num_bits + 7) / 8;
        if(vb->endian == BW_BIG_ENDIAN)
        {
            value = bswap_n(value, num_bytes);
        }
    }

    UInt8 win[9] = {0};
    vfd_seek(vb->d, vb->byte_pos, SEEK_SET);
    if(vfd_read(vb->d, win, sizeof win) < 0)
    {
        return -1;
    }

    __uint128_t chunk = load_window_le(win, sizeof win);
    chunk |= (__uint128_t)value << vb->bit_idx;
    store_window_le(win, chunk, sizeof win);

    vfd_seek(vb->d, vb->byte_pos, SEEK_SET);
    if(vfd_write(vb->d, win, sizeof win) != (ssize_t)sizeof win)
    {
        return -1;
    }

    /* advance cursor */
    size_t tmp = vb->bit_idx + num_bits;
    vb->byte_pos += tmp >> 3;
    vb->bit_idx = tmp & 7;

    return 0;
}

UInt64 vbitwalker_read(vbitwalker_t *vb,
                         UInt8 num_bits)
{
    if(num_bits == 0 || num_bits > 64)
    {
        return 0;
    }

    UInt8 win[9] = {0};
    vfd_seek(vb->d, vb->byte_pos, SEEK_SET);
    if(vfd_read(vb->d, win, sizeof win) < 0)
    {
        return 0;
    }

    __uint128_t chunk = load_window_le(win, sizeof win);
    chunk >>= vb->bit_idx;

    UInt64 mask  = (num_bits == 64) ? UINT64_MAX : ((1ULL << num_bits) - 1);
    UInt64 value = chunk & mask;

    /* endian fix */
    if(num_bits > 8)
    {
        UInt8 num_bytes = (num_bits + 7) / 8;
        if(vb->endian == BW_BIG_ENDIAN)
        {
            value = bswap_n(value, num_bytes);
        }
    }

    return value;
}

int vbitwalker_write_buf(vbitwalker_t *vb,
                         const char *buf,
                         size_t len)
{
    vbitwalker_align_byte(vb);
    vfd_seek(vb->d, vb->byte_pos, SEEK_SET);
    ssize_t written = vfd_write(vb->d, buf, len);
    vb->byte_pos += written;
    return written;
}

int vbitwalker_read_buf(vbitwalker_t *vb,
                        char *buf,
                        size_t len)
{
    vbitwalker_align_byte(vb);
    vfd_seek(vb->d, vb->byte_pos, SEEK_SET);
    ssize_t reddit = vfd_read(vb->d, buf, len);
    vb->byte_pos += reddit;
    return reddit;
}

void vbitwalker_seek(vbitwalker_t *vb,
                     size_t byte_pos,
                     UInt8 bit_idx)
{
    vb->byte_pos = byte_pos;
    vb->bit_idx = bit_idx;
}

void vbitwalker_skip(vbitwalker_t *vb,
                     size_t num_bits)
{
    size_t tmp = vb->bit_idx + num_bits;
    vb->byte_pos += tmp >> 3;
    vb->bit_idx = tmp & 7;
}

size_t vbitwalker_bytes_used(const vbitwalker_t *vb)
{
    return vb->byte_pos + ((vb->bit_idx == 0) ? 0 : 1);
}

void vbitwalker_align_byte(vbitwalker_t *vb)
{
    if(vb->bit_idx != 0)
    {
        vb->bit_idx = 0;
        vb->byte_pos += 1;
    }
}

void vbitwalker_sync(vbitwalker_t *vb)
{
    vfd_sync(vb->d);
}
