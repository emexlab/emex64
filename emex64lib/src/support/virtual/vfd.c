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

#include <emex64lib/support/virtual/vfd.h>

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
            break;
    }

    free(d);
    return vret;
}

ssize_t vfd_read(vfd_t *d,
                 void *buf,
                 size_t count)
{
    switch(d->type)
    {
        case kVFDTypeReal:
            return read(d->fd, buf, count);
            break;
        case kVFDTypeVirtual:
            /* s0n */
            break;
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
            break;
        case kVFDTypeVirtual:
            /* s0n */
            break;
    }
    return -1;
}

int vfd_truncate(vfd_t *d,
                 off_t length)
{
    switch(d->type)
    {
        case kVFDTypeReal:
            return ftruncate(d->fd, length);
            break;
        case kVFDTypeVirtual:
            /* s0n */
            break;
    }
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
            break;
        case kVFDTypeVirtual:
            /* s0n */
            break;
    }
    return 0;
}

void vfd_sync(vfd_t *d)
{
    switch(d->type)
    {
        case kVFDTypeReal:
            fsync(d->fd);
            break;
        case kVFDTypeVirtual:
            /* s0n */
            break;
    }
}

int vfd_stat(vfd_t *d,
             struct stat *stat)
{
    switch(d->type)
    {
        case kVFDTypeReal:
            return fstat(d->fd, stat);
            break;
        case kVFDTypeVirtual:
            /* s0n */
            break;
    }

    return -1;
}
