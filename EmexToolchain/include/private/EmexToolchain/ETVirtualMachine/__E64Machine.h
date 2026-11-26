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

#ifndef __E64MACHINE_H
#define __E64MACHINE_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETVirtualMachine/E64Core.h>
#include <EmexToolchain/ETVirtualMachine/E64MachineOptions.h>
#include <EmexToolchain/ETVirtualMachine/E64Memory.h>
#include <EmexToolchain/ETVirtualMachine/E64MMIOBus.h>
#include <EmexToolchain/ETVirtualMachine/device/internal/controller/E64IC.h>
#include <EmexToolchain/ETVirtualMachine/device/internal/timer.h>
#include <EmexToolchain/ETVirtualMachine/device/board/uart.h>
#include <EmexToolchain/ETVirtualMachine/device/board/controller/8042.h>
#include <EmexToolchain/ETVirtualMachine/device/board/display.h>
#include <EmexToolchain/ETVirtualMachine/device/board/pcie/pcie.h>
#include <EmexToolchain/ETVirtualMachine/device/board/pcie/sata.h>
#include <EmexToolchain/ETVirtualMachine/device/board/pcie/nvme.h>
#include <EmexToolchain/ETVirtualMachine/device/board/pcie/usb.h>
#include <EmexToolchain/ETVirtualMachine/device/board/pcie/gpu.h>
#include <EmexToolchain/ETVirtualMachine/device/board/pcie/net.h>
#include <EmexToolchain/ETVirtualMachine/device/board/pcie/sb16.h>

#define E64_MACHINE_MAX_CORES 16

typedef struct __E64Machine {
    EFObject header;

    /* multicore */
    E64CoreRef cores[E64_MACHINE_MAX_CORES];
    UInt8 coreCount;
    pthread_mutex_t mmioLock;

    /* machine power state */
    volatile Boolean poweredOff;
    volatile Boolean resetting;
    UInt8 resetParked;
    UInt64 resetGeneration;
    pthread_mutex_t resetLock;
    pthread_cond_t resetCond;

    /* devices */
    E64MemoryRef memory;
    E64MMIOBusRef mmioBus;
    E64ICRef interruptController;
    emex64_timer_t *timer;
    emex64_uart_t *uart;
    emex64_display_t *display;
    emex64_8042_t *emex8042;
    emex64_pcie_t *pcie;
    emex64_sata_t *sata;
    emex64_nvme_t *nvme;
    emex64_usb_t *usb;
    emex64_gpu_t *gpu;
    emex64_net_t *net;
    emex64_sb16_t *sb16;
    struct emex64_nvram *nvram;
    Boolean embedded;

    /* the host of this virtual machine */
    E64MachineHost host;
} *__E64Machine;

void __E64MachineResetHardware(__E64Machine machine);

#endif /* __E64MACHINE_H */
