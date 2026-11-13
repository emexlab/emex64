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

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Memory.h>

#define EMEX64_MMIO_BASE        0x0020000000000000

#define EMEX64_IC_BASE          (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 0))     /* Interrupt Controller */
#define EMEX64_APIC_BASE        (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 1))     /* Advanced Programmable Interrupt Controller */
#define EMEX64_TIMER_BASE       (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 2))
#define EMEX64_RTC_BASE         (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 3))     /* Real Time Clock */
#define EMEX64_UART_BASE        (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 4))     /* Universal Asynchronous Receiver Transmitter */
#define EMEX64_MC_BASE          (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 5))     /* Memory Controller */
#define EMEX64_PLATFORM_BASE    (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 6))
#define EMEX64_8042_BASE        (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 7))
#define EMEX64_PCIE_BASE        (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 8))     /* Peripheral Component Interconnect Express Root Complex */
#define EMEX64_IOAPIC_BASE      (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 9))     /* Input Output Advanced Programmable Interrupt Controller */
#define EMEX64_CPUID_BASE       (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 10))    /* Central Processing Unit Identification */
#define EMEX64_NVRAM_BASE       (EMEX64_MMIO_BASE + (EMEX64_PAGE_SIZE * 11))    /* Non Volatile Random Access Memory */

/* PCIe related crap */
#define EMEX64_PCIE_ECAM_BASE   (EMEX64_MMIO_BASE + 0x0000000100000000)
#define EMEX64_PCIE_ECAM_SIZE   0x0000000020000000
#define EMEX64_PCIE_MMIO_BASE   (EMEX64_MMIO_BASE + 0x0000000200000000)
#define EMEX64_PCIE_MMIO_SIZE   0x0000000100000000

#endif /* EMEX64VM_DEVICE_BASE_H */
