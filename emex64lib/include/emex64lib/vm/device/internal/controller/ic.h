/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
