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

#include <emexutils/fdwalker.h>
#include <unistd.h>
#include <assert.h>

void fdwalker_init(fdwalker_t *fw,
                   int fd,
                   bw_endian_t endian)
{
    /* you really shall duplicate the descriptor */
    fw->fd = dup(fd);

    if(fw->fd < 0)
    {
        return;
    }

    /* setting properties */
    fw->byte_pos = 0;
    fw->bit_idx  = 0;
    fw->endian   = endian;
    return;
}

void fdwalker_reset(fdwalker_t *fw)
{
    fw->byte_pos = 0;
    fw->bit_idx = 0;
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
        if(bw_host_endian() != fw->endian)
        {
            value = bw_swap_n(value, num_bytes);
        }
    }

    __uint128_t chunk = 0;

    /* copy up to 8 bytes from buffer */
    lseek(fw->fd, fw->byte_pos, SEEK_SET);
    read(fw->fd, &chunk, 9);

    /* shift value into position */
    chunk |= (__uint128_t)value << fw->bit_idx;

    /* write back */
    lseek(fw->fd, fw->byte_pos, SEEK_SET);
    write(fw->fd, &chunk, 9);

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

    __uint128_t chunk = 0;

    /* copy up to 8 bytes */
    lseek(fw->fd, fw->byte_pos, SEEK_SET);
    read(fw->fd, &chunk, 9);

    /* shift away preceding bits */
    chunk >>= fw->bit_idx;

    uint64_t mask = (num_bits == 64) ? UINT64_MAX : ((1ULL << num_bits) - 1);

    uint64_t value = chunk & mask;

    fw->bit_idx += num_bits;
    fw->byte_pos += fw->bit_idx >> 3;
    fw->bit_idx &= 7;

    /* endian fix */
    if(num_bits > 8)
    {
        uint8_t num_bytes = (num_bits + 7) / 8;
        if(bw_host_endian() != fw->endian)
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
    lseek(fw->fd, fw->byte_pos, SEEK_SET);
    ssize_t written = write(fw->fd, buf, len);
    fw->byte_pos += written;
    return written;
}

int fdwalker_read_buf(fdwalker_t *fw, char *buf, size_t len)
{
    /* s0n */
    (void)fw;
    (void)buf;
    (void)len;

    return 0;
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
