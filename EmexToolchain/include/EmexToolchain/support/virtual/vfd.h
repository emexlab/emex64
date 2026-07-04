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

#ifndef EMEX64_VFD_H
#define EMEX64_VFD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <EmexToolchain/support/virtual/vpageobj.h>

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
            VpageObjRef vpageObjRef;
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

int vfd_putc(vfd_t *d, char c);
int vfd_puts(vfd_t *d, const char *s);
void vfdprintf(vfd_t *d, char *fmt, ...);

#endif /* EMEX64_VFD_H */
