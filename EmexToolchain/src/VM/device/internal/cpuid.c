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

#include <string.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/internal/cpuid.h>

static UInt64 cpuid_string(const char *s,
                           UInt64 offset,
                           UInt64 size)
{
    char field[CPUID_BRAND_SIZE] = { 0 };
    UInt64 value = 0;

    strncpy(field, s, sizeof(field));
    for(UInt64 i = 0; i < 8 && offset + i < size; i++)
    {
        value |= (UInt64)(UInt8)field[offset + i] << (8 * i);
    }
    return value;
}

UInt64 emex64_cpuid_read(E64CoreRef core,
                         void *device,
                         UInt64 offset,
                         EFSize size)
{
    if(offset >= CPUID_REG_BRAND && offset < CPUID_REG_BRAND + CPUID_BRAND_SIZE)
    {
        return cpuid_string(EMEX64_CPU_BRAND, offset - CPUID_REG_BRAND, CPUID_BRAND_SIZE);
    }

    switch(offset)
    {
        case CPUID_REG_VENDOR:
            return cpuid_string(EMEX64_CPU_VENDOR, 0, 8);
        case CPUID_REG_MODEL:
            return E64VM_CPU_MODEL;
        case CPUID_REG_ISA:
            return (UInt64)E64VM_ISA_MAX_VERSION | (UInt64)E64VM_ISA_MIN_VERSION << 16;
        case CPUID_REG_FEATURES:
            return CPUID_FEATURE_PAC;
        default:
            return 0;
    }
}
