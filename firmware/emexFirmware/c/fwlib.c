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

#include <fw.h>

static void out_str(const char *s,
                    int width,
                    int left)
{
    int len = 0;
    while(s[len])
    {
        len++;
    }
    if(!left)
    {
        for(; width > len; width--)
        {
            fw_putc(' ');
        }
    }
    while(*s)
    {
        fw_putc(*s++);
    }
    for(; left && width > len; width--)
    {
        fw_putc(' ');
    }
}

static void out_num(u64 v,
                    unsigned base,
                    int is_signed,
                    int width,
                    int zero,
                    int left)
{
    char buf[24], tmp[26];
    int n = 0, i = 0;

    if(is_signed && (s64)v < 0)
    {
        tmp[i++] = '-';
        v = -v;
    }
    do
    {
        buf[n++] = "0123456789abcdef"[v % base];
        v /= base;
    } while(v);

    if(zero && !left)
    {
        while(i + n < width)
        {
            buf[n++] = '0';
        }
    }
    while(n)
    {
        tmp[i++] = buf[--n];
    }
    tmp[i] = 0;
    out_str(tmp, width, left);
}

void fw_vprintf(const char *fmt,
                va_list ap)
{
    for(; *fmt; fmt++)
    {
        int width = 0, zero = 0, left = 0, lng = 0;

        if(*fmt != '%')
        {
            fw_putc(*fmt);
            continue;
        }
        fmt++;
        if(*fmt == '-')
        {
            left = 1;
            fmt++;
        }
        if(*fmt == '0')
        {
            zero = 1;
            fmt++;
        }
        while(*fmt >= '0' && *fmt <= '9')
        {
            width = width * 10 + (*fmt++ - '0');
        }
        while(*fmt == 'l')
        {
            lng = 1;
            fmt++;
        }

        switch(*fmt)
        {
            case 'd':
                out_num(lng ? (u64)va_arg(ap, long) : (u64)(s64)va_arg(ap, int), 10, 1, width, zero, left);
                break;
            case 'u':
                out_num(lng ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int), 10, 0, width, zero, left);
                break;
            case 'x':
                out_num(lng ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int), 16, 0, width, zero, left);
                break;
            case 'p':
                fw_putc('0');
                fw_putc('x');
                out_num((u64)va_arg(ap, void *), 16, 0, width, zero, left);
                break;
            case 'c':
                fw_putc(va_arg(ap, int));
                break;
            case 's':
            {
                const char *s = va_arg(ap, const char *);
                out_str(s ? s : "(null)", width, left);
                break;
            }
            case '%':
                fw_putc('%');
                break;
            default:
                fw_putc('%');
                fw_putc(*fmt);
                break;
        }
    }
}

void fw_printf(const char *fmt,
               ...)
{
    va_list ap;
    va_start(ap, fmt);
    fw_vprintf(fmt, ap);
    va_end(ap);
}

void fw_put_size(u64 bytes)
{
    static const char *units[] = { "B", "KiB", "MiB", "GiB", "TiB" };
    int u = 0;
    while(u < 4 && bytes >= 1024 && (bytes % 1024 == 0 || bytes >= 1024 * 10))
    {
        bytes /= 1024;
        u++;
    }
    fw_printf("%lu %s", bytes, units[u]);
}

int fw_memcmp(const void *a,
              const void *b,
              u64 len)
{
    const u8 *p = a, *q = b;
    for(; len; len--, p++, q++)
    {
        if(*p != *q)
        {
            return *p - *q;
        }
    }
    return 0;
}

static u32 crc_table[256];
static int crc_ready;

u32 fw_crc32_update(u32 crc,
                    const void *data,
                    u64 len)
{
    const u8 *p = data;

    if(!crc_ready)
    {
        for(u32 i = 0; i < 256; i++)
        {
            u32 c = i;
            for(int k = 0; k < 8; k++)
            {
                c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
            }
            crc_table[i] = c;
        }
        crc_ready = 1;
    }

    crc = ~crc;
    while(len--)
    {
        crc = crc_table[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }

    return ~crc;
}

void fw_jump(u64 entry,
             void *arg)
{
    register u64 r0 __asm__("r0") = (u64)arg;
    __asm__ __volatile__("b %1" : : "r"(r0), "r"(entry) : "memory");
    __builtin_unreachable();
}
