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

#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <EmexToolchain/support/virtual/vfd.h>
#include <EmexToolchain/vm/E64Memory.h>

vfd_t *vfd_open(const char *path,
                int flg,
                ...)
{
    vfd_t *d = calloc(1, sizeof(vfd_t));
    if(d == NULL)
    {
        return NULL;
    }

    /* potentially getting mode */
    mode_t mode = 0;
    if(flg & O_CREAT)
    {
        va_list ap;
        va_start(ap, flg);
        mode = va_arg(ap, int);
        va_end(ap);
    }

    /* really opening the file */
    d->fd = open(path, flg, mode);
    if(d->fd < 0)
    {
        free(d);
        return NULL;
    }

    return d;
}

vfd_t *vfd_open_fd(int fd)
{
    fd = dup(fd);
    if(fd < 0)
    {
        return NULL;
    }

    vfd_t *d = calloc(1, sizeof(vfd_t));
    if(d == NULL)
    {
        return NULL;
    }

    d->fd = fd;
    return d;
}

vfd_t *vfd_vopen()
{
    vfd_t *d = calloc(1, sizeof(vfd_t));
    if(d == NULL)
    {
        return NULL;
    }

    d->type = kVFDTypeVirtual;
    d->vd.vpageObjRef = VpageObjCreate(kEFAllocatorDefault);
    d->vd.off = 0;

    if(d->vd.vpageObjRef == NULL)
    {
        return NULL;
    }

    return d;
}

int vfd_close(vfd_t *d)
{
    int vret = -1;
    switch(d->type)
    {
        case kVFDTypeReal:
            vret = close(d->fd);
            break;
        case kVFDTypeVirtual:
            EFRelease(d->vd.vpageObjRef);
            vret = 0;
            break;
    }

    free(d);
    return vret;
}

vfd_t *vfd_dup(vfd_t *d)
{
    vfd_t *nd = calloc(1, sizeof(vfd_t));
    if(nd == NULL)
    {
        return NULL;
    }

    nd->type = d->type;

    switch(d->type)
    {
        case kVFDTypeReal:
            nd->fd = dup(d->fd);
            if(nd->fd < 0)
            {
                goto fail;
            }
            break;
        case kVFDTypeVirtual:
            /* copy entire state */
            nd->vd.off = d->vd.off;
            nd->vd.vpageObjRef = EFRetain(d->vd.vpageObjRef);
            if(nd->vd.vpageObjRef == NULL)
            {
                goto fail;
            }
            break;
        default:
        fail:
            free(d);
            return NULL;
    }

    return nd;
}

ssize_t vfd_read(vfd_t *d,
                 void *buf,
                 size_t count)
{
    switch(d->type)
    {
        case kVFDTypeReal:
            return read(d->fd, buf, count);
        case kVFDTypeVirtual:
        {
            ssize_t vret = (ssize_t)VpageObjRead(d->vd.vpageObjRef, (size_t)d->vd.off, buf, count);
            if(vret > 0)
            {
                d->vd.off += vret;
            }
            return vret;
        }
    }
    return -1;
}

ssize_t vfd_write(vfd_t *d,
                  const void *buf,
                  size_t count)
{
    switch(d->type)
    {
        case kVFDTypeReal:
            return write(d->fd, buf, count);
        case kVFDTypeVirtual:
        {
            size_t start = (size_t)d->vd.off;
            size_t end_off = start + count;

            while(end_off > VpageObjGetSize(d->vd.vpageObjRef))
            {
                if(!VpageObjExtendPage(d->vd.vpageObjRef))
                {
                    errno = ENOMEM;
                    return -1;
                }
            }

            ssize_t vret = (ssize_t)VpageObjWrite(d->vd.vpageObjRef, start, buf, count);
            if(vret < 0)
            {
                return vret;
            }

            d->vd.off = (off_t)(start + (size_t)vret);
            if((size_t)d->vd.off > VpageObjGetEndMarker(d->vd.vpageObjRef))
            {
                VpageObjSetEndMarker(d->vd.vpageObjRef, (size_t)d->vd.off);
            }
            return vret;
        }
    }
    return -1;
}

int vfd_truncate(vfd_t *d, off_t length)
{
    switch(d->type)
    {
        case kVFDTypeReal:
            return ftruncate(d->fd, length);
        case kVFDTypeVirtual:
        {
            if(length < 0)
            {
                errno = EINVAL;
                return -1;
            }

            size_t newlen = (size_t)length;
            size_t oldlen = VpageObjGetEndMarker(d->vd.vpageObjRef);

            /* make sure the backing store can hold the new lenght */
            while(VpageObjGetSize(d->vd.vpageObjRef) < newlen)
            {
                if(!VpageObjExtendPage(d->vd.vpageObjRef)) { errno = ENOMEM; return -1; }
            }

            if(newlen < oldlen)
            {
                static const uint8_t zeros[EMEX64_PAGE_SIZE] = {0};
                size_t pos = newlen;
                while(pos < oldlen)
                {
                    size_t chunk = oldlen - pos;
                    if(chunk > sizeof(zeros)) chunk = sizeof(zeros);
                    VpageObjWrite(d->vd.vpageObjRef, pos, zeros, chunk);
                    pos += chunk;
                }
            }

            VpageObjSetEndMarker(d->vd.vpageObjRef, newlen);
            return 0;
        }
    }

    errno = EINVAL;
    return -1;
}

off_t vfd_seek(vfd_t *d,
               off_t off,
               int a)
{
    switch(d->type)
    {
        case kVFDTypeReal:
            return lseek(d->fd, off, a);
        case kVFDTypeVirtual:
        {
            off_t base;
            switch(a)
            {
                case SEEK_SET:
                    base = 0;
                    break;
                case SEEK_CUR:
                    base = d->vd.off;
                    break;
                case SEEK_END:
                    base = (off_t)VpageObjGetEndMarker(d->vd.vpageObjRef);
                    break;
                default:
                    errno = EINVAL;
                    return (off_t)-1;
            }

            off_t new_off;
            if(__builtin_add_overflow(base, off, &new_off))
            {
                errno = EOVERFLOW;
                return (off_t)-1;
            }
            if(new_off < 0)
            {
                errno = EINVAL;
                return (off_t)-1;
            }

            d->vd.off = new_off;
            return new_off;
        }
    }

    errno = EINVAL;
    return (off_t)-1;
}

void vfd_sync(vfd_t *d)
{
    switch(d->type)
    {
        case kVFDTypeReal:
            fsync(d->fd);
            break;
        case kVFDTypeVirtual:
            /* no need for sync here */
            break;
    }
}

int vfd_stat(vfd_t *d,
             struct stat *stat)
{
    fflush(stdout);
    switch(d->type)
    {
        case kVFDTypeReal:
            fflush(stdout);
            return fstat(d->fd, stat);
        case kVFDTypeVirtual:
        {
            fflush(stdout);
            stat->st_size = (off_t)VpageObjGetEndMarker(d->vd.vpageObjRef);
            return 0;
        }
    }

    return -1;
}

char *vfd_gets(vfd_t *d,
               char *s,
               int n)
{
    if(s == NULL || n <= 0)
    {
        return NULL;
    }

    if(n == 1)
    {
        s[0] = '\0';
        return s;
    }

    int i = 0;
    while(i < n - 1)
    {
        char c;
        ssize_t r = vfd_read(d, &c, 1);

        if(r < 0)
        {

            return NULL;
        }
        if(r == 0)
        {
            if(i == 0)
            {
                return NULL;
            }
            break;
        }

        s[i++] = c;
        if(c == '\n')
        {
            break;
        }
    }

    s[i] = '\0';
    return s;
}

int vfd_putc(vfd_t *d, char c)
{
    return (int)vfd_write(d, &c, 1);
}
 
int vfd_puts(vfd_t *d, const char *s)
{
    int count = 0;
 
    if(!s)
    {
        s = "(null)";
    }
 
    while(*s)
    {
        count += vfd_putc(d, *s++);
    }
 
    return count;
}
 
static inline int vfd_putnbr_base_unsigned(vfd_t *d,
                                           uint64_t n,
                                           const char *base)
{
    int count = 0;
    uint64_t radix = 0;
 
    while(base[radix])
    {
        radix++;
    }
 
    if(n >= radix)
    {
        count += vfd_putnbr_base_unsigned(d, n / radix, base);
    }
 
    count += vfd_putc(d, base[n % radix]);
    return count;
}
 
static inline int vfd_putnbr_signed(vfd_t *d, long n)
{
    int count = 0;
 
    if(n < 0)
    {
        count += vfd_putc(d, '-');
        n = -n;
    }
 
    count += vfd_putnbr_base_unsigned(d, (uint64_t)n, "0123456789");
 
    return count;
}
 
static inline int vfd_put_binary(vfd_t *d, unsigned int n)
{
    return vfd_putnbr_base_unsigned(d, n, "01");
}
 
static inline int vfd_put_pointer(vfd_t *d, void *p)
{
    int count = 0;
    count += vfd_puts(d, "0x");
    count += vfd_putnbr_base_unsigned(d, (uintptr_t)p, "0123456789abcdef");
    return count;
}
 
static inline int vfd_put_float(vfd_t *d, double n)
{
    int count = 0;
    long ipart = (long)n;
    double fpart = n - ipart;
 
    if(n < 0)
    {
        count += vfd_putc(d, '-');
        n = -n;
        ipart = -ipart;
        fpart = -fpart;
    }
 
    count += vfd_putnbr_signed(d, ipart);
    count += vfd_putc(d, '.');
 
    for(int i = 0; i < 6; i++)
    {
        fpart *= 10;
        count += vfd_putc(d, (int)fpart + '0');
        fpart -= (int)fpart;
    }
 
    return count;
}

void vfdprintf(vfd_t *d, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int i = 0;
    while(fmt[i])
    {
        if(fmt[i] == '%' && fmt[i + 1])
        {
            i++;
            switch(fmt[i])
            {
                case 'c':
                    vfd_putc(d, (char)va_arg(args, int));
                    break;
                case 's':
                    vfd_puts(d, va_arg(args, char *));
                    break;
                case 'd':
                    vfd_putnbr_signed(d, va_arg(args, int));
                    break;
                case 'u':
                    vfd_putnbr_base_unsigned(d, va_arg(args, unsigned int), "0123456789");
                    break;
                case 'b':
                    vfd_put_binary(d, va_arg(args, unsigned int));
                    break;
                case 'x':
                    vfd_putnbr_base_unsigned(d, va_arg(args, unsigned int), "0123456789abcdef");
                    break;
                case 'X':
                    vfd_putnbr_base_unsigned(d, va_arg(args, unsigned int), "0123456789ABCDEF");
                    break;
                case 'p':
                    vfd_put_pointer(d, va_arg(args, void *));
                    break;
                case 'f':
                    vfd_put_float(d, va_arg(args, double));
                    break;
                case 'l':
                    switch(fmt[i + 1])
                    {
                        case 'l':
                            switch(fmt[i + 2])
                            {
                                case 'd':
                                    i += 2;
                                    vfd_putnbr_signed(d, va_arg(args, int64_t));
                                    break;
                                case 'u':
                                    i += 2;
                                    vfd_putnbr_base_unsigned(d, va_arg(args, uint64_t), "0123456789");
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case 'd':
                            i++;
                            vfd_putnbr_signed(d, va_arg(args, long));
                            break;
                        case 'u':
                            i++;
                            vfd_putnbr_base_unsigned(d, va_arg(args, unsigned long), "0123456789");
                            break;
                        default:
                            break;
                    }
                    break;
                case '%':
                    vfd_putc(d, '%');
                    break;
                default:
                    break;
            }
        }
        else
        {
            vfd_putc(d, fmt[i]);
        }
        i++;
    }

    va_end(args);
}
