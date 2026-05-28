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

#include <emexutils/bitwalker.h>

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "bitwalker requires a little-endian host"
#endif

/* helper */

bw_endian_t bw_host_endian(void)
{
    static const uint16_t test = 0x0001;
    return (*((const uint8_t*)&test) == 0x01) ? BW_LITTLE_ENDIAN : BW_BIG_ENDIAN;
}

uint64_t bw_swap_n(uint64_t v,
                   uint8_t num_bytes)
{
    switch (num_bytes)
    {
        case 2:  return __builtin_bswap16((uint16_t)v);
        case 3:  return ((v >> 16) & 0xFF) | (v & 0xFF00) | ((v & 0xFF) << 16);
        case 4:  return __builtin_bswap32((uint32_t)v);
        case 5:
        case 6:
        case 7:
        case 8:  return __builtin_bswap64(v) >> ((8 - num_bytes) * 8);
        default: return v;
    }
}

/* initialization */

void bitwalker_init(bitwalker_t *bw,
                    uint8_t *buf,
                    size_t capacity,
                    bw_endian_t endian)
{
    bw->buffer   = buf;
    bw->byte_pos = 0;
    bw->bit_idx  = 0;
    bw->capacity = capacity;
    bw->endian   = endian;
    memset(buf, 0, capacity);
}

void bitwalker_init_read(bitwalker_t *bw,
                         const uint8_t *buf,
                         size_t len,
                         bw_endian_t endian)
{
    bw->buffer   = (uint8_t *)buf;
    bw->byte_pos = 0;
    bw->bit_idx  = 0;
    bw->capacity = len;
    bw->endian   = endian;
}

/* management */

void bitwalker_reset(bitwalker_t *bw)
{
    bw->byte_pos = 0;
    bw->bit_idx = 0;
}

/* read and write */

int bitwalker_write(bitwalker_t *bw,
                    uint64_t value,
                    uint8_t num_bits)
{
    if(num_bits == 0 || num_bits > 64)
    {
        return -1;
    }
    else if(bw->byte_pos >= bw->capacity)
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
        if(bw_host_endian() != bw->endian)
        {
            value = bw_swap_n(value, num_bytes);
        }
    }

    __uint128_t chunk = 0;

    /* copy up to 8 bytes from buffer */
    size_t remain = bw->capacity - bw->byte_pos;
    size_t n = remain < 9 ? remain : 9;

    memcpy(&chunk, bw->buffer + bw->byte_pos, n);

    /* shift value into position */
    chunk |= (__uint128_t)value << bw->bit_idx;

    /* write back */
    memcpy(bw->buffer + bw->byte_pos, &chunk, n);

    /* advance cursor */
    size_t tmp = bw->bit_idx + num_bits;
    bw->byte_pos += tmp >> 3;
    bw->bit_idx = tmp & 7;
    
    return 0;
}

uint64_t bitwalker_read(bitwalker_t *bw,
                        uint8_t num_bits)
{
    if(num_bits == 0 || num_bits > 64)
    {
        return 0;
    }
    else if(bw->byte_pos >= bw->capacity)
    {
        return 0;
    }

    __uint128_t chunk = 0;

    /* copy up to 8 bytes */
    size_t remain = bw->capacity - bw->byte_pos;
    size_t n = remain < 9 ? remain : 9;

    memcpy(&chunk, bw->buffer + bw->byte_pos, n);

    /* shift away preceding bits */
    chunk >>= bw->bit_idx;

    uint64_t mask = (num_bits == 64) ? UINT64_MAX : ((1ULL << num_bits) - 1);

    uint64_t value = chunk & mask;

    bw->bit_idx += num_bits;
    bw->byte_pos += bw->bit_idx >> 3;
    bw->bit_idx &= 7;

    /* endian fix */
    if(num_bits > 8)
    {
        uint8_t num_bytes = (num_bits + 7) / 8;
        if(bw_host_endian() != bw->endian)
        {
            value = bw_swap_n(value, num_bytes);
        }
    }

    return value;
}

void bitwalker_skip(bitwalker_t *bw,
                    size_t num_bits)
{
    size_t tmp = bw->bit_idx + num_bits;
    bw->byte_pos += tmp >> 3;
    bw->bit_idx = tmp & 7;
}

size_t bitwalker_bytes_used(const bitwalker_t *bw)
{
    return bw->byte_pos + ((bw->bit_idx == 0) ? 0 : 1);
}

void bitwalker_align_byte(bitwalker_t *bw)
{
    if(bw->bit_idx != 0)
    {
        bw->bit_idx = 0;
        bw->byte_pos += 1;
    }
}
