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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include <emex64lib/support/fdwalker.h>

fdwalker_t *fdwalker_alloc(int fd,
                           bw_endian_t endian)
{
    vfd_t *d = vfd_open_fd(dup(fd));
    if(d == NULL)
    {
        return NULL;
    }

    fdwalker_t *fw = malloc(sizeof(fdwalker_t));
    if(fw == NULL)
    {
        vfd_close(d);
        return NULL;
    }

    /* setting properties */
    fw->d = d;
    fw->byte_pos = 0;
    fw->bit_idx = 0;
    fw->endian = endian;

    return fw;
}

void fdwalker_dealloc(fdwalker_t *fw)
{
    if(fw == NULL)
    {
        return;
    }
    
    vfd_close(fw->d);
    free(fw);
}

void fdwalker_reset(fdwalker_t *fw)
{
    fw->byte_pos = 0;
    fw->bit_idx = 0;
}

static __uint128_t load_window_le(const uint8_t *p, size_t n)
{
    __uint128_t v = 0;
    for(size_t i = 0; i < n; i++)
    {
        v |= (__uint128_t)p[i] << (8 * i);
    }
    return v;
}

static void store_window_le(uint8_t *p, __uint128_t v, size_t n)
{
    for(size_t i = 0; i < n; i++)
    {
        p[i] = (uint8_t)(v >> (8 * i));
    }
}

int fdwalker_write(fdwalker_t *fw,
                   uint64_t value,
                   uint8_t num_bits)
{
    assert(fw != NULL);

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
        if(fw->endian == BW_BIG_ENDIAN)
        {
            value = bw_swap_n(value, num_bytes);
        }
    }

    uint8_t win[9] = {0};
    vfd_seek(fw->d, fw->byte_pos, SEEK_SET);
    if(vfd_read(fw->d, win, sizeof win) < 0)
    {
        return -1;
    }

    __uint128_t chunk = load_window_le(win, sizeof win);
    chunk |= (__uint128_t)value << fw->bit_idx;
    store_window_le(win, chunk, sizeof win);

    vfd_seek(fw->d, fw->byte_pos, SEEK_SET);
    if(vfd_write(fw->d, win, sizeof win) != (ssize_t)sizeof win)
    {
        return -1;
    }

    /* advance cursor */
    size_t tmp = fw->bit_idx + num_bits;
    fw->byte_pos += tmp >> 3;
    fw->bit_idx = tmp & 7;
    
    return 0;
}

uint64_t fdwalker_read(fdwalker_t *fw,
                       uint8_t num_bits)
{
    if(num_bits == 0 || num_bits > 64)
    {
        return 0;
    }

    uint8_t win[9] = {0};
    vfd_seek(fw->d, fw->byte_pos, SEEK_SET);
    if(vfd_read(fw->d, win, sizeof win) < 0)
    {
        return 0;
    }

    __uint128_t chunk = load_window_le(win, sizeof win);
    chunk >>= fw->bit_idx;

    uint64_t mask  = (num_bits == 64) ? UINT64_MAX : ((1ULL << num_bits) - 1);
    uint64_t value = chunk & mask;

    /* endian fix */
    if(num_bits > 8)
    {
        uint8_t num_bytes = (num_bits + 7) / 8;
        if(fw->endian == BW_BIG_ENDIAN)
        {
            value = bw_swap_n(value, num_bytes);
        }
    }

    return value;
}

int fdwalker_write_buf(fdwalker_t *fw,
                       const char *buf,
                       size_t len)
{
    fdwalker_align_byte(fw);
    vfd_seek(fw->d, fw->byte_pos, SEEK_SET);
    ssize_t written = vfd_write(fw->d, buf, len);
    fw->byte_pos += written;
    return written;
}

int fdwalker_read_buf(fdwalker_t *fw,
                      char *buf,
                      size_t len)
{
    fdwalker_align_byte(fw);
    vfd_seek(fw->d, fw->byte_pos, SEEK_SET);
    ssize_t reddit = vfd_read(fw->d, buf, len);
    fw->byte_pos += reddit;
    return reddit;
}

void fdwalker_seek(fdwalker_t *fw,
                   size_t byte_pos,
                   uint8_t bit_idx)
{
    fw->byte_pos = byte_pos;
    fw->bit_idx = bit_idx;
}

void fdwalker_skip(fdwalker_t *fw,
                   size_t num_bits)
{
    size_t tmp = fw->bit_idx + num_bits;
    fw->byte_pos += tmp >> 3;
    fw->bit_idx = tmp & 7;
}

size_t fdwalker_bytes_used(const fdwalker_t *fw)
{
    return fw->byte_pos + ((fw->bit_idx == 0) ? 0 : 1);
}

void fdwalker_align_byte(fdwalker_t *fw)
{
    if(fw->bit_idx != 0)
    {
        fw->bit_idx = 0;
        fw->byte_pos += 1;
    }
}

void fdwalker_sync(fdwalker_t *fw)
{
    vfd_sync(fw->d);
}
