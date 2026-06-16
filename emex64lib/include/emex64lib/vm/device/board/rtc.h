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

#ifndef EMEX64VM_DEVICE_RTC_H
#define EMEX64VM_DEVICE_RTC_H

#include <stdint.h>

#include <emex64lib/vm/device/base.h>

#define EMEX64_RTC_SIZE 0x38

#define RTC_REG_SECONDS 0x00
#define RTC_REG_MINUTES 0x08
#define RTC_REG_HOURS   0x10
#define RTC_REG_DAY     0x18
#define RTC_REG_MONTH   0x20
#define RTC_REG_YEAR    0x28
#define RTC_REG_WEEKDAY 0x30

typedef struct emex64_core emex64_core_t;

uint64_t emex64_rtc_read(emex64_core_t *core, void *device, uint64_t offset, int size);

#endif /* EMEX64VM_DEVICE_RTC_H */
