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

#ifndef EMEX64_ENDIAN_H
#define EMEX64_ENDIAN_H

#include <stddef.h>
#include <stdint.h>

#define BW_LITTLE_ENDIAN 0
#define BW_BIG_ENDIAN 1

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#define BW_HOST_ENDIAN  BW_BIG_ENDIAN
#else
#define BW_HOST_ENDIAN  BW_LITTLE_ENDIAN
#endif

#if BW_HOST_ENDIAN == BW_BIG_ENDIAN
#define TO_HOST16(x) __builtin_bswap16(x)
#define TO_HOST32(x) __builtin_bswap32(x)
#define TO_HOST64(x) __builtin_bswap64(x)
#else
#define TO_HOST16(x) (x)
#define TO_HOST32(x) (x)
#define TO_HOST64(x) (x)
#endif

typedef UInt8 bw_endian_t;

#endif /* EMEX64_ENDIAN_H */
