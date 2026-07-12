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

#ifndef EMEX64VM_DEVICE_MC_H
#define EMEX64VM_DEVICE_MC_H

#include <stdint.h>
#include <EmexToolchain/VM/device/base.h>

#define EMEX64_MC_SIZE              0x18

#define EMEX64_MC_REG_SIZE          0x00
#define EMEX64_MC_REG_KTRR_SIZE     0x08
#define EMEX64_MC_REG_KTRR_LOCKED   0x10

typedef struct __E64Core *E64CoreRef;

UInt64 emex64_mc_read(E64CoreRef core, void *device, UInt64 offset, int size);
void emex64_mc_write(E64CoreRef core, void *device, UInt64 offset, UInt64 value, int size);

#endif /* EMEX64VM_DEVICE_MC_H */
