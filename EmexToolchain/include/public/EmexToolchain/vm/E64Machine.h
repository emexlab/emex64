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

#ifndef E64MACHINE_H
#define E64MACHINE_H

#include <stdint.h>
#include <stdbool.h>
#include <EmexToolchain/vm/E64MachineOptions.h>
#include <EmexToolchain/vm/E64Core.h>
#include <EmexToolchain/vm/E64Memory.h>
#include <EmexToolchain/vm/E64MMIO.h>
#include <EmexToolchain/vm/device/base.h>
#include <EmexFoundation/EmexFoundation.h>

typedef struct __E64Machine *E64MachineRef;

typedef struct __E64Machine {
    EFObject header;
    E64CoreRef core;
    E64MemoryRef memory;
    E64MMIOBusRef mmio_bus;
    void *intc;
    void *timer;
    void *uart;
    void *display;
    void *emex8042;
} *__E64Machine;

EFTypeID E64MachineGetTypeID(void);

E64MachineRef E64MachineCreate(EFAllocatorRef allocatorRef);
E64MachineRef E64MachineCreateWithOptions(EFAllocatorRef allocatorRef, E64MachineOptions options);

E64CoreRef E64MachineGetCore(E64MachineRef machineRef);
E64MemoryRef E64MachineGetMemory(E64MachineRef machineRef);
E64MMIOBusRef E64MachineGetMMIOBus(E64MachineRef machineRef);

#endif /* E64MACHINE_H */
