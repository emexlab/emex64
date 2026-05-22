/*
 * MIT License
 *
 * Copyright (c) 2024 cr4zyengineer
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

#ifndef LA64VM_DEVICE_UART_H
#define LA64VM_DEVICE_UART_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

#define LA64_UART_BASE      0x1FE00300
#define LA64_UART_SIZE      0x10

#define UART_BUF_SIZE          64

#define UART_REG_DATA          0x00
#define UART_REG_STATUS        0x04
#define UART_REG_CONTROL       0x08

#define UART_STATUS_RX_READY   (1 << 0)
#define UART_STATUS_TX_EMPTY   (1 << 1)
#define UART_STATUS_RX_FULL    (1 << 2)
#define UART_STATUS_OVERFLOW   (1 << 3)

#define UART_CTRL_RX_IRQ_EN    (1 << 0)
#define UART_CTRL_TX_IRQ_EN    (1 << 1)
#define UART_CTRL_RESET        (1 << 2)

typedef struct la64_machine la64_machine_t;

typedef struct {
    uint8_t rx_buf[UART_BUF_SIZE];
    uint32_t rx_head, rx_tail;
    uint32_t status;
    uint32_t control;
    
    pthread_t thread;
    pthread_mutex_t mutex;
    atomic_bool running;

    la64_machine_t *machine;
} la64_uart_t;

la64_uart_t *la64_uart_alloc(la64_machine_t *machine);
void la64_uart_dealloc(la64_uart_t *u);

uint64_t la64_uart_read(la64_core_t *core, void *device, uint64_t offset, int size);
void la64_uart_write(la64_core_t *core, void *device, uint64_t offset, uint64_t value, int size);

#endif /* LA64VM_DEVICE_UART_H */