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

#ifndef EMEX64VM_DEVICE_BASE_H
#define EMEX64VM_DEVICE_BASE_H

#include <EmexToolchain/vm/memory.h>

#define EMEX64_MMIO_BASE        0x0020000000000000

#define EMEX64_IC_BASE          (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 0)) /* Interrupt Controller */
#define EMEX64_APIC_BASE        (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 1)) /* Advanced Programmable Interrupt Controller */
#define EMEX64_TIMER_BASE       (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 2))
#define EMEX64_RTC_BASE         (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 3))
#define EMEX64_UART_BASE        (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 4))
#define EMEX64_MC_BASE          (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 5))
#define EMEX64_PLATFORM_BASE    (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 6))
#define EMEX64_8042_BASE        (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 7))

#define EMEX64_FB_BASE          (EMEX64_MMIO_BASE + 0x0010000000000000)

#endif /* EMEX64VM_DEVICE_BASE_H */
