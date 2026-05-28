/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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

#ifndef EMEX64VM_MACHINE_H
#define EMEX64VM_MACHINE_H

#include <emex64vm/core.h>
#include <emex64vm/memory.h>
#include <emex64vm/mmio.h>

#include <emex64vm/device/timer.h>
#include <emex64vm/device/interrupt.h>
#include <emex64vm/device/uart.h>

#if defined(__linux__)  || defined(__APPLE__)
#include <emex64vm/device/display.h>
#endif /* __linux__ */

#include <stdint.h>

typedef struct la64_machine {
    la64_core_t *core;
    la64_memory_t *memory;
    la64_mmio_bus_t *mmio_bus;
    la64_intc_t *intc;
    la64_timer_t *timer;
    la64_uart_t *uart;
#if defined(__linux__)  || defined(__APPLE__)
    la64_display_t *display;
#endif /* __linux__ */
} la64_machine_t;

la64_machine_t *la64_machine_alloc(uint64_t memory_size);
void la64_machine_dealloc(la64_machine_t *machine);

#endif /* EMEX64VM_MACHINE_H */
