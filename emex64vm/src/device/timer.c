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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <emex64vm/machine.h>

#include <emex64vm/device/timer.h>
#include <emex64vm/device/interrupt.h>

uint64_t la64_get_host_cycles(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__)
    uint64_t val;
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#elif defined(__loongarch64)
    uint64_t val;
    __asm__ volatile ("rdtime.d %0, $zero" : "=r"(val));
    return val;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

static uint64_t detect_host_freq(void)
{
#if defined(__aarch64__)
    uint64_t freq;
    __asm__ volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    uint32_t max_level = eax;
    if(max_level >= 0x15)
    {
        __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x15), "c"(0));
        if(eax != 0 && ebx != 0 && ecx != 0)
        {
            return ((uint64_t)ecx * ebx) / eax;
        }
    }
    if(max_level >= 0x16)
    {
        __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x16), "c"(0));
        if((eax & 0xFFFF) != 0)
        {
            return (uint64_t)(eax & 0xFFFF) * 1000000ULL;
        }
    }
    struct timespec start_ts, end_ts;
    uint32_t lo, hi;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    uint64_t start_cycles = ((uint64_t)hi << 32) | lo;
    usleep(100000);
    clock_gettime(CLOCK_MONOTONIC, &end_ts);
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    uint64_t end_cycles = ((uint64_t)hi << 32) | lo;
    uint64_t elapsed_ns = (end_ts.tv_sec - start_ts.tv_sec) * 1000000000ULL +  (end_ts.tv_nsec - start_ts.tv_nsec);
    uint64_t elapsed_cycles = end_cycles - start_cycles;
    return (elapsed_cycles * 1000000000ULL) / elapsed_ns;
#else
    return 1000000000ULL;
#endif
}

la64_timer_t *la64_timer_alloc(la64_machine_t *machine)
{
    /* allocate timer */
    la64_timer_t *timer = malloc(sizeof(la64_timer_t));

    if(timer == NULL)
    {
        return NULL;
    }

    /* register timer MMIO */
    if(!la64_mmio_register(machine->mmio_bus, LA64_TIMER_BASE, LA64_TIMER_SIZE, timer, la64_timer_read, la64_timer_write))
    {
        free(timer);
        return NULL;
    }

    /* setting up timer */
    timer->machine = machine;
    timer->compare = UINT64_MAX;
    
    timer->host_freq = detect_host_freq();
    timer->last_host_cycles = la64_get_host_cycles();
    
    return timer;
}

void la64_timer_dealloc(la64_timer_t *timer)
{
    free(timer);
}

void la64_timer_tick(la64_timer_t *timer,
                     uint64_t host_cycles)
{
    /* checking if timer is not enabled */
    if(!(timer->ctrl & TIMER_CTRL_ENABLE))
    {
        /* if it is then we simply forget about it!!! */
        timer->last_host_cycles = host_cycles;
        return;
    }
    
    /* calculate elappsed cycles */
    uint64_t elapsed_host = host_cycles - timer->last_host_cycles;
    timer->last_host_cycles = host_cycles;

    if (elapsed_host == 0)
    {
        return;
    }

    /*  calculating using virtual frequency the actual timer count */
    uint64_t virtual_ticks = elapsed_host;
    
    /* null check */
    if(virtual_ticks == 0)
    {
        return;
    }
    
    /* updating timer */
    uint64_t old_count = timer->count;
    timer->count += virtual_ticks;
    
    /* compare match */
    if(old_count < timer->compare && timer->count >= timer->compare)
    {
        timer->status |= TIMER_STATUS_IRQ;
        
        if(timer->ctrl & TIMER_CTRL_PERIODIC)
        {
            timer->count -= timer->compare;
        }
        else
        {
            timer->ctrl &= ~TIMER_CTRL_ENABLE;
        }
        
        if(timer->ctrl & TIMER_CTRL_IRQ_EN)
        {
            la64_raise_interrupt(timer->machine, LA64_IRQ_TIMER);
        }
    }
}

uint64_t la64_timer_read(la64_core_t *core,
                         void *device,
                         uint64_t offset,
                         int size)
{
    /* getting timer */
    la64_timer_t *timer = (la64_timer_t *)device;

    /* perform read */
    switch(offset)
    {
        case TIMER_REG_CTRL:
            return timer->ctrl;
        case TIMER_REG_COUNT:
            return timer->count;
        case TIMER_REG_COMPARE:
            return timer->compare;
        case TIMER_REG_STATUS:
            return timer->status;
        case TIMER_REG_FREQ:
            return timer->host_freq;
        default:
            return 0;
    }
}

void la64_timer_write(la64_core_t *core,
                      void *device,
                      uint64_t offset,
                      uint64_t value,
                      int size)
{
    /* getting timer */
    la64_timer_t *timer = (la64_timer_t *)device;

    /* perform write */
    switch(offset)
    {
        case TIMER_REG_CTRL:
            timer->ctrl = value;
            if(value & TIMER_CTRL_ENABLE)
            {
                timer->last_host_cycles = la64_get_host_cycles();
            }
            break;
        case TIMER_REG_COUNT:
            timer->count = value;
            break;
        case TIMER_REG_COMPARE:
            timer->compare = value;
            break;
        case TIMER_REG_STATUS:
            timer->status &= ~value;
            break;
        case TIMER_REG_FREQ:
            /* Read-only */
            break;
        default:
            break;
    }
}
