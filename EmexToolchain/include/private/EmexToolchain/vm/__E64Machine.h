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

#ifndef __E64MACHINE_H
#define __E64MACHINE_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/vm/E64Core.h>
#include <EmexToolchain/vm/E64Memory.h>
#include <EmexToolchain/vm/E64MMIO.h>
#include <EmexToolchain/vm/device/internal/timer.h>
#include <EmexToolchain/vm/device/internal/controller/ic.h>
#include <EmexToolchain/vm/device/board/uart.h>
#include <EmexToolchain/vm/device/board/controller/8042.h>
#include <EmexToolchain/vm/device/board/display.h>

typedef struct __E64Machine {
    EFObject header;
    E64CoreRef core;
    E64MemoryRef memory;
    E64MMIOBusRef mmio_bus;
    emex64_intc_t *intc;
    emex64_timer_t *timer;
    emex64_uart_t *uart;
    emex64_display_t *display;
    emex64_8042_t *emex8042;
} *__E64Machine;

#endif /* __E64MACHINE_H */
