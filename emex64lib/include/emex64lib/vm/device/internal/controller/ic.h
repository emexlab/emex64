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

#ifndef EMEX64VM_DEVICE_IC_H
#define EMEX64VM_DEVICE_IC_H

#include <stdint.h>
#include <stdbool.h>
#include <emex64lib/vm/device/base.h>

#define EMEX64_INTC_SIZE      0x30

/* internal devices */
#define EMEX64_IRQ_EXCEPTION  0
#define EMEX64_IRQ_TIMER      1
#define EMEX64_IRQ_DISK       2
#define EMEX64_IRQ_NETWORK    3
#define EMEX64_IRQ_SOFTWARE   4

/* board devices */
#define EMEX64_IRQ_UART       5
#define EMEX64_IRQ_8042       6   /* emex8042 MMIO chip fires interrupt when device gets plugged in for example */

#define EMEX64_IRQ_MAX        63

#define EMEX64_INTC_REG_PENDING   0x00
#define EMEX64_INTC_REG_ENABLED   0x08
#define EMEX64_INTC_REG_CTRL      0x10
#define EMEX64_INTC_REG_VECTOR    0x18
#define EMEX64_INTC_REG_ACK       0x20
#define EMEX64_INTC_REG_CURRENT   0x28

/* control register bits */
#define EMEX64_INTC_CTRL_ENABLE   (1 << 0)

typedef struct emex64_core emex64_core_t;
typedef struct emex64_machine emex64_machine_t;

typedef struct emex64_intc {
    uint64_t pending;
    uint64_t enabled;
    uint64_t ctrl;
    uint64_t vector_base;
    int64_t  current_irq;
} emex64_intc_t;

emex64_intc_t *emex64_intc_alloc(emex64_machine_t *machine);
void emex64_intc_dealloc(emex64_intc_t *intc);

void emex64_raise_interrupt(emex64_machine_t *machine, int irq_line);
void emex64_clear_interrupt(emex64_machine_t *machine, int irq_line);
bool emex64_serve_interrupt_if_needed(emex64_core_t *core);

uint64_t emex64_intc_read(emex64_core_t *core, void *device, uint64_t offset, int size);
void emex64_intc_write(emex64_core_t *core, void *device, uint64_t offset, uint64_t value, int size);

#endif /* EMEX64VM_DEVICE_IC_H */
