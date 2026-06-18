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

#include <evObj/alloc.h>
#include <evObj/reference.h>

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
    d->virtual.p = evo_alloc_fastpath(vpageobj);
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
            evo_release(d->virtual.p);
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
            if(!evo_retain(d->virtual.p))
            {
                goto fail;
            }
            nd->virtual.p = d->virtual.p;
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
            ssize_t vret = (ssize_t)vpage_read(d->virtual.p->root, (size_t)d->virtual.off, buf, count);
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
        {
            size_t start = (size_t)d->virtual.off;
            size_t end_off = start + count;

            while(end_off > vpage_get_size(d->virtual.p->root))
            {
                if(!vpage_gib_page(d->virtual.p->root))
                {
                    errno = ENOMEM;
                    return -1;
                }
            }

            ssize_t vret = (ssize_t)vpage_write(d->virtual.p->root, start, buf, count);
            if(vret < 0)
            {
                return vret;
            }

            d->virtual.off = (off_t)(start + (size_t)vret);
            if((size_t)d->virtual.off > d->virtual.p->extra_size_marker)
            {
                d->virtual.p->extra_size_marker = (size_t)d->virtual.off;
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
            size_t oldlen = d->virtual.p->extra_size_marker;

            /* make sure the backing store can hold the new lenght */
            while(vpage_get_size(d->virtual.p->root) < newlen)
            {
                if(!vpage_gib_page(d->virtual.p->root)) { errno = ENOMEM; return -1; }
            }

            if(newlen < oldlen)
            {
                static const uint8_t zeros[EMEX64_PAGE_SIZE] = {0};
                size_t pos = newlen;
                while(pos < oldlen)
                {
                    size_t chunk = oldlen - pos;
                    if(chunk > sizeof(zeros)) chunk = sizeof(zeros);
                    vpage_write(d->virtual.p->root, pos, zeros, chunk);
                    pos += chunk;
                }
            }

            d->virtual.p->extra_size_marker = newlen;
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
                    base = (off_t)d->virtual.p->extra_size_marker;
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
            stat->st_size = (off_t)d->virtual.p->extra_size_marker;
            return 0;
        }
    }

    return -1;
}
