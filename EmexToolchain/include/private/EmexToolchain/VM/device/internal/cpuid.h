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

#ifndef EMEX64VM_DEVICE_CPUID_H
#define EMEX64VM_DEVICE_CPUID_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/base.h>
#include <EmexToolchain/VM/E64Core.h>

#define EMEX64_CPUID_SIZE       0x80

#define CPUID_REG_VENDOR        0x00
#define CPUID_REG_MODEL         0x08
#define CPUID_REG_ISA           0x10
#define CPUID_REG_FEATURES      0x18
#define CPUID_FEATURE_PAC       (1ULL << 0)
#define CPUID_REG_BRAND         0x40
#define CPUID_BRAND_SIZE        64

#define EMEX64_CPU_VENDOR       "emexLabs"
#define EMEX64_CPU_BRAND        "emex64e-2050"

UInt64 emex64_cpuid_read(E64CoreRef core, void *device, UInt64 offset, EFSize size);

#endif /* EMEX64VM_DEVICE_CPUID_H */
