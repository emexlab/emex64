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

#ifndef EMEX64_VPAGE_H
#define EMEX64_VPAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <EmexFoundation/EmexFoundation.h>

typedef struct vpage {
    UInt8 *p;
    size_t len;
    struct vpage *prev;
    struct vpage *next;
} vpage_t;

void *__vpage_alloc(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
vpage_t *vpage_alloc();
void vpage_dealloc(vpage_t *p);

size_t vpage_get_size(vpage_t *p);
Boolean vpage_gib_page(vpage_t *p);
Boolean vpage_bind_page(vpage_t *p);

size_t vpage_write(vpage_t *p, size_t off, const UInt8 *b, size_t len);
size_t vpage_read(vpage_t *p, size_t off, UInt8 *b, size_t len);

#endif /* EMEX64_VPAGE_H */
