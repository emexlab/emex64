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

#ifndef EMEX64VM_DEVICE_TIMER_H
#define EMEX64VM_DEVICE_TIMER_H

#include <stdint.h>
#include <emex64lib/vm/core.h>
#include <emex64lib/vm/device/base.h>

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

typedef struct emex64_machine emex64_machine_t;

typedef struct emex64_timer {
    uint64_t ctrl;
    uint64_t count;
    uint64_t compare;
    uint64_t status;
    
    uint64_t host_freq;
    uint64_t last_host_cycles;
    
    emex64_machine_t *machine;
    uint64_t tick_remainder;
} emex64_timer_t;

emex64_timer_t *emex64_timer_alloc(emex64_machine_t *core);
void emex64_timer_dealloc(emex64_timer_t *timer);
void emex64_timer_tick(emex64_timer_t *timer, uint64_t host_cycles);
uint64_t emex64_get_host_cycles(void);

uint64_t emex64_timer_read(emex64_core_t *core, void *device, uint64_t offset, int size);
void emex64_timer_write(emex64_core_t *core, void *device, uint64_t offset, uint64_t value, int size);

#endif /* EMEX64VM_DEVICE_TIMER_H */
