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

#ifndef EMEX64VM_DEVICE_RTC_H
#define EMEX64VM_DEVICE_RTC_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/base.h>

#define EMEX64_RTC_SIZE 0x38

#define RTC_REG_SECONDS 0x00
#define RTC_REG_MINUTES 0x08
#define RTC_REG_HOURS   0x10
#define RTC_REG_DAY     0x18
#define RTC_REG_MONTH   0x20
#define RTC_REG_YEAR    0x28
#define RTC_REG_WEEKDAY 0x30

typedef struct __E64Core *E64CoreRef;

UInt64 emex64_rtc_read(E64CoreRef core, void *device, UInt64 offset, EFSize size);

#endif /* EMEX64VM_DEVICE_RTC_H */
