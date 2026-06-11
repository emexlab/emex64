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

#ifndef EMEX64VM_DEVICE_BASE_H
#define EMEX64VM_DEVICE_BASE_H

#include <emex64lib/vm/memory.h>

#define EMEX64_MMIO_BASE        0x0020000000000000

#define EMEX64_INTC_BASE        (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 0))
#define EMEX64_TIMER_BASE       (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 1))
#define EMEX64_RTC_BASE         (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 2))
#define EMEX64_UART_BASE        (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 3))
#define EMEX64_MC_BASE          (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 4))
#define EMEX64_PLATFORM_BASE    (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 5))
#define EMEX64_8042_BASE        (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 6))
#define EMEX64_AC97_BASE        (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 7))

#define EMEX64_FB_BASE          (EMEX64_MMIO_BASE + 0x0010000000000000)

#endif /* EMEX64VM_DEVICE_BASE_H */
