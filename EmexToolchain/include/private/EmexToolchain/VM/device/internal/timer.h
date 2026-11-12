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

#include <pthread.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Core.h>
#include <EmexToolchain/VM/device/base.h>
#include <EmexToolchain/VM/device/internal/vtimer.h>

#define EMEX64_TIMER_SIZE   0x28

typedef struct __E64Machine *E64MachineRef;

typedef struct emex64_timer {
    emex64_vtimer_t vt;
    pthread_mutex_t lock;
    E64MachineRef machine;
    UInt64 seq;
    UInt64 snap_count;
    UInt64 snap_host;
    UInt64 snap_remainder;
    UInt64 snap_compare;
    UInt64 snap_mode;
} emex64_timer_t;

#define EMEX64_TIMER_SNAP_SLOW      0
#define EMEX64_TIMER_SNAP_STOPPED   1
#define EMEX64_TIMER_SNAP_COUNTING  2

#define EMEX64_TIMER_TICK_INTERVAL  32

emex64_timer_t *emex64_timer_alloc(E64MachineRef machine);
void emex64_timer_dealloc(emex64_timer_t *timer);
void emex64_timer_reset(emex64_timer_t *timer);
void emex64_timer_tick(emex64_timer_t *timer, UInt64 host_cycles);
Boolean emex64_timer_read_count(emex64_timer_t *timer, UInt64 *value);
UInt64 emex64_get_host_cycles(void);

UInt64 emex64_timer_read(E64CoreRef core, void *device, UInt64 offset, EFSize size);
void emex64_timer_write(E64CoreRef core, void *device, UInt64 offset, UInt64 value, EFSize size);

#endif /* EMEX64VM_DEVICE_TIMER_H */
