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
#include <EmexToolchain/vm/core.h>
#include <EmexToolchain/vm/E64Memory.h>
#include <EmexToolchain/vm/E64MMIO.h>
#include <EmexToolchain/vm/device/internal/timer.h>
#include <EmexToolchain/vm/device/internal/controller/ic.h>
#include <EmexToolchain/vm/device/board/uart.h>
#include <EmexToolchain/vm/device/board/controller/8042.h>
#include <EmexToolchain/vm/device/board/display.h>
#include <EmexFoundation/EmexFoundation.h>

typedef struct __E64Machine *E64MachineRef;

typedef struct __E64Machine {
    emex64_core_t *core;
    E64MemoryRef memory;
    E64MMIOBusRef mmio_bus;
    emex64_intc_t *intc;
    emex64_timer_t *timer;
    emex64_uart_t *uart;
    emex64_display_t *display;
    emex64_8042_t *emex8042;
} *__E64Machine;

EFTypeID E64MachineGetTypeID(void);

E64MachineRef E64MachineCreate(EFAllocatorRef allocatorRef);
E64MachineRef E64MachineCreateWithOptions(EFAllocatorRef allocatorRef, E64MachineOptions options);

emex64_core_t *E64MachineGetCore(E64MachineRef machineRef);
E64MemoryRef E64MachineGetMemory(E64MachineRef machineRef);
E64MMIOBusRef E64MachineGetMMIOBus(E64MachineRef machineRef);
emex64_intc_t *E64MachineGetIC(E64MachineRef machineRef);
emex64_timer_t *E64MachineGetTimer(E64MachineRef machineRef);
emex64_uart_t *E64MachineGetUART(E64MachineRef machineRef);
emex64_display_t *E64MachineGetDisplay(E64MachineRef machineRef);
emex64_8042_t *E64MachineGet8042(E64MachineRef machineRef);

E64MachineSupport E64MachineSupportGet(void);
E64MachineOptions E64MachineOptionsGetDefault(void);

#endif /* E64MACHINE_H */
