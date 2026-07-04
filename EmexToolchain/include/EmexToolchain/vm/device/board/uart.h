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

#ifndef EMEX64VM_DEVICE_UART_H
#define EMEX64VM_DEVICE_UART_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>
#include <EmexToolchain/vm/device/base.h>

#define UART_BUF_SIZE          64

#define EMEX64_UART_SIZE    0x18

#define UART_REG_DATA       0x00
#define UART_REG_STATUS     0x08
#define UART_REG_CONTROL    0x10

#define UART_STATUS_RX_READY   (1 << 0)
#define UART_STATUS_TX_EMPTY   (1 << 1)
#define UART_STATUS_RX_FULL    (1 << 2)
#define UART_STATUS_OVERFLOW   (1 << 3)

#define UART_CTRL_RX_IRQ_EN    (1 << 0)
#define UART_CTRL_TX_IRQ_EN    (1 << 1)
#define UART_CTRL_RESET        (1 << 2)

typedef struct __E64Machine *E64MachineRef;

typedef struct {
    UInt8 rx_buf[UART_BUF_SIZE];
    UInt32 rx_head, rx_tail;
    UInt32 status;
    UInt32 control;
    
    pthread_t thread;
    pthread_mutex_t mutex;
    atomic_bool running;

    E64MachineRef machine;
} emex64_uart_t;

emex64_uart_t *emex64_uart_alloc(E64MachineRef machine);
void emex64_uart_dealloc(emex64_uart_t *u);

UInt64 emex64_uart_read(emex64_core_t *core, void *device, UInt64 offset, int size);
void emex64_uart_write(emex64_core_t *core, void *device, UInt64 offset, UInt64 value, int size);

#endif /* EMEX64VM_DEVICE_UART_H */
