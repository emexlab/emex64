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

#ifndef EMEX64VM_DEVICE_TIMER_H
#define EMEX64VM_DEVICE_TIMER_H

#include <stdint.h>
#include <EmexToolchain/vm/core.h>
#include <EmexToolchain/vm/device/base.h>

#define EMEX64_TIMER_SIZE   0x28

#define TIMER_REG_CTRL      0x00
#define TIMER_REG_COUNT     0x08
#define TIMER_REG_COMPARE   0x10
#define TIMER_REG_STATUS    0x18
#define TIMER_REG_FREQ      0x20        /* read only!!! */

#define TIMER_CTRL_ENABLE   (1 << 0)
#define TIMER_CTRL_IRQ_EN   (1 << 1)
#define TIMER_CTRL_PERIODIC (1 << 2)
#define TIMER_STATUS_IRQ    (1 << 0)

#define TIMER_VIRTUAL_FREQ  1000000ULL

typedef struct __E64Machine *E64MachineRef;

typedef struct emex64_timer {
    UInt64 ctrl;
    UInt64 count;
    UInt64 compare;
    UInt64 status;
    
    UInt64 host_freq;
    UInt64 last_host_cycles;
    
    E64MachineRef machine;
    UInt64 tick_remainder;
} emex64_timer_t;

emex64_timer_t *emex64_timer_alloc(E64MachineRef machine);
void emex64_timer_dealloc(emex64_timer_t *timer);
void emex64_timer_tick(emex64_timer_t *timer, UInt64 host_cycles);
UInt64 emex64_get_host_cycles(void);

UInt64 emex64_timer_read(emex64_core_t *core, void *device, UInt64 offset, int size);
void emex64_timer_write(emex64_core_t *core, void *device, UInt64 offset, UInt64 value, int size);

#endif /* EMEX64VM_DEVICE_TIMER_H */
