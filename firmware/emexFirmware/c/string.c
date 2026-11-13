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

void *memset(void *dst,
             int value,
             size_t len)
{
    __asm__ __volatile__("setm %0, %1, %2" : : "r"(dst), "r"((u64)value), "r"(len) : "memory");
    return dst;
}

void *memcpy(void *dst,
             const void *src,
             size_t len)
{
    __asm__ __volatile__("cpym %0, %1, %2" : : "r"(dst), "r"(src), "r"(len) : "memory");
    return dst;
}

void *memmove(void *dst,
              const void *src,
              size_t len)
{
    __asm__ __volatile__("movm %0, %1, %2" : : "r"(dst), "r"(src), "r"(len) : "memory");
    return dst;
}
