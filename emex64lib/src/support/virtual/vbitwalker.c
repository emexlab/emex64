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

#include <stdlib.h>
#include <assert.h>
#include <emex64lib/support/virtual/vbitwalker.h>

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
                     uint64_t value,
                     uint8_t num_bits)
{
    assert(vb != NULL);

    if(num_bits == 0 || num_bits > 64)
    {
        return -1;
    }

    uint64_t mask = (num_bits == 64) ? ~0ULL : ((1ULL << num_bits) - 1);
    value &= mask;

    /*
     * for multi-byte values we gonna have to convert host endian to target endian
     * in case they aint the same
     */
    if(num_bits > 8)
    {
        uint8_t num_bytes = (num_bits + 7) / 8;
        if(vb->endian == BW_BIG_ENDIAN)
        {
            value = bswap_n(value, num_bytes);
        }
    }

    uint8_t win[9] = {0};
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

uint64_t vbitwalker_read(vbitwalker_t *vb,
                         uint8_t num_bits)
{
    if(num_bits == 0 || num_bits > 64)
    {
        return 0;
    }

    uint8_t win[9] = {0};
    vfd_seek(vb->d, vb->byte_pos, SEEK_SET);
    if(vfd_read(vb->d, win, sizeof win) < 0)
    {
        return 0;
    }

    __uint128_t chunk = load_window_le(win, sizeof win);
    chunk >>= vb->bit_idx;

    uint64_t mask  = (num_bits == 64) ? UINT64_MAX : ((1ULL << num_bits) - 1);
    uint64_t value = chunk & mask;

    /* endian fix */
    if(num_bits > 8)
    {
        uint8_t num_bytes = (num_bits + 7) / 8;
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
                     uint8_t bit_idx)
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
