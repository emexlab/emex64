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

#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <emex64lib/support/virtual/vfd.h>

#include <emex64lib/vm/memory.h>

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

vfd_t *vfd_vopen(int flg)
{
    vfd_t *d = calloc(1, sizeof(vfd_t));
    if(d == NULL)
    {
        return NULL;
    }

    d->type = kVFDTypeVirtual;
    d->virtual.flg = flg;
    d->virtual.p = vpage_alloc();
    d->virtual.off = 0;

    if(d->virtual.p == NULL)
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
            vpage_dealloc(d->virtual.p);
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
            nd->virtual.flg = d->virtual.flg;
            nd->virtual.off = d->virtual.off;
            nd->virtual.p = d->virtual.p;       /* this is fine! */
            nd->virtual.size = d->virtual.size; /* could require some vfd_vdatasource_t??? that is MRC ref counted?? */
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
            ssize_t vret = (ssize_t)vpage_read(d->virtual.p, (size_t)d->virtual.off, buf, count);
            if(vret > 0)
            {
                d->virtual.off += vret;
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
    try_pass:
        {
            size_t end_off = (size_t)d->virtual.off + count;
            if(end_off > vpage_get_size(d->virtual.p))
            {
                vpage_gib_page(d->virtual.p);
                goto try_pass;
            }
            d->virtual.off = (off_t)end_off;
            ssize_t vret = (ssize_t)vpage_write(d->virtual.p, (size_t)d->virtual.off, buf, count);
            if((size_t)d->virtual.off > d->virtual.size)
            {
                d->virtual.size = (size_t)d->virtual.off;
            }
            d->virtual.off = end_off;
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
            size_t oldlen = d->virtual.size;

            /* make sure the backing store can hold the new lenght */
            while(vpage_get_size(d->virtual.p) < newlen)
            {
                if(!vpage_gib_page(d->virtual.p)) { errno = ENOMEM; return -1; }
            }

            if(newlen < oldlen)
            {
                static const uint8_t zeros[EMEX64_PAGE_SIZE] = {0};
                size_t pos = newlen;
                while(pos < oldlen)
                {
                    size_t chunk = oldlen - pos;
                    if(chunk > sizeof(zeros)) chunk = sizeof(zeros);
                    vpage_write(d->virtual.p, pos, zeros, chunk);
                    pos += chunk;
                }
            }

            d->virtual.size = newlen;
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
                    base = d->virtual.off;
                    break;
                case SEEK_END:
                    base = (off_t)d->virtual.size;
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

            d->virtual.off = new_off;
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
            stat->st_size = (off_t)d->virtual.size;
            return 0;
        }
    }

    return -1;
}
