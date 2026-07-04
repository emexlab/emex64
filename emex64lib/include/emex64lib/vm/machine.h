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

#ifndef EMEX64VM_MACHINE_H
#define EMEX64VM_MACHINE_H

#include <stdint.h>
#include <stdbool.h>
#include <emex64lib/vm/options.h>
#include <emex64lib/vm/core.h>
#include <emex64lib/vm/memory.h>
#include <emex64lib/vm/mmio.h>
#include <emex64lib/vm/device/internal/timer.h>
#include <emex64lib/vm/device/internal/controller/ic.h>
#include <emex64lib/vm/device/board/uart.h>
#include <emex64lib/vm/device/board/controller/8042.h>
#include <emex64lib/vm/device/board/display.h>

typedef struct emex64_machine {
    emex64_core_t *core;
    E64MemoryRef memory;
    E64MMIOBusRef mmio_bus;
    emex64_intc_t *intc;
    emex64_timer_t *timer;
    emex64_uart_t *uart;
    emex64_display_t *display;
    emex64_8042_t *emex8042;
} emex64_machine_t;

emex64_machine_t *emex64_machine_alloc(emex64_machine_options_t options);
void emex64_machine_dealloc(emex64_machine_t *machine);

emex64_machine_support_t emex64_machine_support_get(void);
emex64_machine_options_t emex64_machine_options_default(void);

#endif /* EMEX64VM_MACHINE_H */
