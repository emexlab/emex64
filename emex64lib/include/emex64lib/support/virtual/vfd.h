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

#ifndef VFD_H
#define VFD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <emex64lib/support/virtual/vpageobj.h>

typedef enum: uint8_t {
    kVFDTypeReal,
    kVFDTypeVirtual,
} kVFDType;

typedef struct vfd {
    kVFDType type;

    union {
        int fd;

        struct {
            off_t off;
            vpageobj_t *p;
        } vd;
    };
} vfd_t;

vfd_t *vfd_open(const char *path, int flg, ...);
vfd_t *vfd_open_fd(int fd);
vfd_t *vfd_vopen();
int vfd_close(vfd_t *d);

vfd_t *vfd_dup(vfd_t *d);

ssize_t vfd_read(vfd_t *d, void *buf, size_t count);
ssize_t vfd_write(vfd_t *d, const void *buf, size_t count);
int vfd_truncate(vfd_t *d, off_t length);

off_t vfd_seek(vfd_t *d, off_t off, int a);
void vfd_sync(vfd_t *d);
int vfd_stat(vfd_t *d, struct stat *stat);

char *vfd_gets(vfd_t *d, char *s, int n);

#endif /* VFD_H */
