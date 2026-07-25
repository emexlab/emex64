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

#ifndef __E64MMIOREGION_H
#define __E64MMIOREGION_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/__E64Core.h>

typedef UInt64 (*mmio_read_fn)(__E64Core core, void *device, UInt64 offset, EFSize size);
typedef void (*mmio_write_fn)(__E64Core core, void *device, UInt64 offset, UInt64 value, EFSize size);

typedef struct __E64MMIORegion {
    EFObject header;
    UInt64 base_addr;
    UInt64 size;
    void *device;
    mmio_read_fn read;
    mmio_write_fn write;
} *__E64MMIORegion;

#endif /* __E64MMIOREGION_H */
