/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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

#include <emex64vm/machine.h>
#include <emex64vm/device/uart.h>
#include <emex64vm/device/interrupt.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <pthread.h>
#include <unistd.h>

static struct termios uart_orig_termios;

static void uart_set_raw_mode(void)
{
    /* TODO: make this rawer, or make a GUI terminal */
    struct termios raw;
    tcgetattr(STDIN_FILENO, &uart_orig_termios);
    raw = uart_orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void uart_restore_mode(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &uart_orig_termios);
}

static void uart_update_irq(la64_uart_t *u)
{    
    int level = ((u->control & UART_CTRL_RX_IRQ_EN) && (u->status & UART_STATUS_RX_READY)) || ((u->control & UART_CTRL_TX_IRQ_EN) && (u->status & UART_STATUS_TX_EMPTY));

    /* updating interrupt */
    if(level)
    {
        la64_raise_interrupt(u->machine, LA64_IRQ_UART);
    }
    else
    {
        la64_clear_interrupt(u->machine, LA64_IRQ_UART);
    }
}

static void *uart_input_thread(void *arg)
{
    la64_uart_t *u = (la64_uart_t *)arg;

    uint8_t ch;
    
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
        
        uint32_t next = (u->rx_tail + 1) % UART_BUF_SIZE;

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

static inline void la64_uart_start(la64_uart_t *u)
{
    if(u->running)
    {
        return;
    }
    
    atomic_store(&u->running, true);
    uart_set_raw_mode();
    pthread_create(&u->thread, NULL, uart_input_thread, u);
}

static inline void la64_uart_stop(la64_uart_t *u)
{
    if(!u->running)
    {
        return;
    }
    
    atomic_store(&u->running, false);
    pthread_join(u->thread, NULL);
    uart_restore_mode();
}

la64_uart_t *la64_uart_alloc(la64_machine_t *machine)
{
    /* allocate uart */
    la64_uart_t *u = malloc(sizeof(la64_uart_t));

    /* null pointer check */
    if(u == NULL)
    {
        return NULL;
    }

    /* registering uart on machine */
    if(!la64_mmio_register(machine->mmio_bus, LA64_UART_BASE, LA64_UART_SIZE, u, la64_uart_read, la64_uart_write))
    {
        free(u);
        return NULL;
    }

    /* setting up uart */
    u->machine = machine;
    u->status = UART_STATUS_TX_EMPTY;
    
    pthread_mutex_init(&u->mutex, NULL);
    atomic_store(&u->running, false);

    la64_uart_start(u);

    return u;
}

void la64_uart_dealloc(la64_uart_t *u)
{
    la64_uart_stop(u);
    pthread_mutex_destroy(&u->mutex);
    free(u);
}

uint64_t la64_uart_read(la64_core_t *core, void *device, uint64_t offset, int size)
{
    /* getting uart */
    la64_uart_t *u = (la64_uart_t *)device;

    pthread_mutex_lock(&u->mutex);
    uint64_t result = 0;

    /* perform read */
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

void la64_uart_write(la64_core_t *core, void *device, uint64_t offset, uint64_t value, int size)
{
    /* getting uart */
    la64_uart_t *u = (la64_uart_t *)device;

    pthread_mutex_lock(&u->mutex);

    /* perform write */
    switch(offset)
    {
        case UART_REG_DATA:
            putchar((char)(value & 0xFF));
            fflush(stdout);
            u->status |= UART_STATUS_TX_EMPTY;
            uart_update_irq(u);
            break;
        case UART_REG_CONTROL:
            u->control = (uint32_t)value;
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
