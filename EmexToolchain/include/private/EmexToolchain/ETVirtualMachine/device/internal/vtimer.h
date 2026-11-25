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

#ifndef EMEX64VM_DEVICE_VTIMER_H
#define EMEX64VM_DEVICE_VTIMER_H

#include <EmexFoundation/EmexFoundation.h>

#define TIMER_REG_CTRL      0x00
#define TIMER_REG_COUNT     0x08
#define TIMER_REG_COMPARE   0x10
#define TIMER_REG_STATUS    0x18
#define TIMER_REG_FREQ      0x20

#define TIMER_CTRL_ENABLE   (1 << 0)
#define TIMER_CTRL_IRQ_EN   (1 << 1)
#define TIMER_CTRL_PERIODIC (1 << 2)
#define TIMER_STATUS_IRQ    (1 << 0)

#define TIMER_VIRTUAL_FREQ  1000000000ULL

typedef struct emex64_vtimer {
    UInt64 ctrl;
    UInt64 count;
    UInt64 compare;
    UInt64 status;

    UInt64 last_host_cycles;
    UInt64 tick_remainder;
    UInt64 min_elapsed;
} emex64_vtimer_t;

void emex64_vtimer_init(emex64_vtimer_t *t);
Boolean emex64_vtimer_advance(emex64_vtimer_t *t, UInt64 host_cycles);
static inline Boolean emex64_vtimer_tick(emex64_vtimer_t *t, UInt64 host_cycles)
{
    if(__builtin_expect(host_cycles - t->last_host_cycles < t->min_elapsed, 1))
    {
        return false;
    }
    return emex64_vtimer_advance(t, host_cycles);
}

UInt64 emex64_vtimer_read(emex64_vtimer_t *t, UInt64 offset);
void emex64_vtimer_write(emex64_vtimer_t *t, UInt64 offset, UInt64 value);

UInt64 emex64_host_freq(void);

#endif /* EMEX64VM_DEVICE_VTIMER_H */
