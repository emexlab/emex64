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

#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <pthread.h>
#include <unistd.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/device/board/uart.h>
#include <EmexToolchain/VM/device/internal/controller/E64IC.h>

static struct termios uart_orig_termios;

static void uart_set_raw_mode(void)
{
    struct termios raw;
    tcgetattr(STDIN_FILENO, &uart_orig_termios);
    raw = uart_orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void uart_restore_mode(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &uart_orig_termios);
}

static void uart_update_irq(emex64_uart_t *u)
{
    int level = ((u->control & UART_CTRL_RX_IRQ_EN) && (u->status & UART_STATUS_RX_READY)) || ((u->control & UART_CTRL_TX_IRQ_EN) && (u->status & UART_STATUS_TX_EMPTY));
    if(level)
    {
        E64ICRaiseInterrupt(u->machine->intc, EMEX64_IRQ_UART);
    }
    else
    {
        E64ICClearInterrupt(u->machine->intc, EMEX64_IRQ_UART);
    }
}

static void *uart_input_thread(void *arg)
{
    emex64_uart_t *u = (emex64_uart_t *)arg;

    UInt8 ch;

    while(atomic_load(&u->running))
    {
        fd_set fds;
        struct timeval tv;

        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ready = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);

        if(ready <= 0)
        {
            continue;
        }

        ssize_t n = read(STDIN_FILENO, &ch, 1);

        if(n <= 0)
        {
            continue;
        }

        if(ch == 0x03)
        {
            atomic_store(&u->running, false);
            break;
        }

        pthread_mutex_lock(&u->mutex);

        UInt32 next = (u->rx_tail + 1) % UART_BUF_SIZE;

        if(next == u->rx_head)
        {
            u->status |= UART_STATUS_OVERFLOW;
        }
        else
        {
            u->rx_buf[u->rx_tail] = ch;
            u->rx_tail = next;
            u->status |= UART_STATUS_RX_READY;

            if(((u->rx_tail - u->rx_head) % UART_BUF_SIZE) > (UART_BUF_SIZE - 4))
            {
                u->status |= UART_STATUS_RX_FULL;
            }

            uart_update_irq(u);
        }

        pthread_mutex_unlock(&u->mutex);
    }

    return NULL;
}

static inline void emex64_uart_start(emex64_uart_t *u)
{
    if(u->running)
    {
        return;
    }

    atomic_store(&u->running, true);
    uart_set_raw_mode();
    pthread_create(&u->thread, NULL, uart_input_thread, u);
}

static inline void emex64_uart_stop(emex64_uart_t *u)
{
    if(!u->running)
    {
        return;
    }

    atomic_store(&u->running, false);
    pthread_join(u->thread, NULL);
    uart_restore_mode();
}

emex64_uart_t *emex64_uart_alloc(E64MachineRef machine)
{
    emex64_uart_t *u = malloc(sizeof(emex64_uart_t));
    if(u == NULL)
    {
        return NULL;
    }

    EFAUTOREL E64MMIORegionRef UARTRegion = E64MMIORegionCreate(kEFAllocatorDefault, EMEX64_UART_BASE, EMEX64_UART_SIZE, u, emex64_uart_read, emex64_uart_write);
    if(UARTRegion == NULL || !E64MMIOBusRegisterRegion(machine->mmio_bus, UARTRegion))
    {
        free(u);
        return NULL;
    }

    u->machine = machine;
    u->status = UART_STATUS_TX_EMPTY;

    pthread_mutex_init(&u->mutex, NULL);
    atomic_store(&u->running, false);

    emex64_uart_start(u);

    return u;
}

void emex64_uart_dealloc(emex64_uart_t *u)
{
    emex64_uart_stop(u);
    pthread_mutex_destroy(&u->mutex);
    free(u);
}

UInt64 emex64_uart_read(E64CoreRef core,
                        void *device,
                        UInt64 offset,
                        int size)
{
    emex64_uart_t *u = (emex64_uart_t *)device;

    pthread_mutex_lock(&u->mutex);
    UInt64 result = 0;

    switch(offset)
    {
        case UART_REG_DATA:
            if(u->rx_head != u->rx_tail)
            {
                result = u->rx_buf[u->rx_head];
                u->rx_head = (u->rx_head + 1) % UART_BUF_SIZE;
                if(u->rx_head == u->rx_tail)
                {
                    u->status &= ~UART_STATUS_RX_READY;
                }
                u->status &= ~UART_STATUS_RX_FULL;
                uart_update_irq(u);
            }
            break;
        case UART_REG_STATUS:
            result = u->status;
            break;
        case UART_REG_CONTROL:
            result = u->control;
            break;
        default:
            break;
    }

    pthread_mutex_unlock(&u->mutex);
    return result;
}

void emex64_uart_write(E64CoreRef core,
                       void *device,
                       UInt64 offset,
                       UInt64 value,
                       int size)
{
    emex64_uart_t *u = (emex64_uart_t *)device;

    pthread_mutex_lock(&u->mutex);

    switch(offset)
    {
        case UART_REG_DATA:
            putchar((char)(value & 0xFF));
            fflush(stdout);
            u->status |= UART_STATUS_TX_EMPTY;
            uart_update_irq(u);
            break;
        case UART_REG_CONTROL:
            u->control = (UInt32)value;
            if(value & UART_CTRL_RESET)
            {
                u->rx_head = u->rx_tail = 0;
                u->status = UART_STATUS_TX_EMPTY;
                u->control &= ~UART_CTRL_RESET;
            }
            uart_update_irq(u);
            break;
        default:
            break;
    }

    pthread_mutex_unlock(&u->mutex);
}
