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
    Emex64MemoryRef memory;
    Emex64MMIOBusRef mmio_bus;
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
