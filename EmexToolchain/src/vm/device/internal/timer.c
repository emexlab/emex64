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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <EmexToolchain/vm/E64Machine.h>
#include <EmexToolchain/vm/device/internal/timer.h>
#include <EmexToolchain/vm/device/internal/controller/ic.h>

UInt64 emex64_get_host_cycles(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    UInt32 lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((UInt64)hi << 32) | lo;
#elif defined(__aarch64__)
    UInt64 val;
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r"(val));
    return val;
#elif defined(__loongarch64)
    UInt64 val;
    __asm__ volatile ("rdtime.d %0, $zero" : "=r"(val));
    return val;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (UInt64)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

static UInt64 detect_host_freq(void)
{
#if defined(__aarch64__)
    UInt64 freq;
    __asm__ volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    UInt32 eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    UInt32 max_level = eax;
    if(max_level >= 0x15)
    {
        __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x15), "c"(0));
        if(eax != 0 && ebx != 0 && ecx != 0)
        {
            return ((UInt64)ecx * ebx) / eax;
        }
    }
    if(max_level >= 0x16)
    {
        __asm__ volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x16), "c"(0));
        if((eax & 0xFFFF) != 0)
        {
            return (UInt64)(eax & 0xFFFF) * 1000000ULL;
        }
    }
    struct timespec start_ts, end_ts;
    UInt32 lo, hi;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    UInt64 start_cycles = ((UInt64)hi << 32) | lo;
    usleep(100000);
    clock_gettime(CLOCK_MONOTONIC, &end_ts);
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    UInt64 end_cycles = ((UInt64)hi << 32) | lo;
    UInt64 elapsed_ns = (end_ts.tv_sec - start_ts.tv_sec) * 1000000000ULL +  (end_ts.tv_nsec - start_ts.tv_nsec);
    UInt64 elapsed_cycles = end_cycles - start_cycles;
    return (elapsed_cycles * 1000000000ULL) / elapsed_ns;
#else
    return 1000000000ULL;
#endif
}

emex64_timer_t *emex64_timer_alloc(E64MachineRef machine)
{
    /* allocate timer */
    emex64_timer_t *timer = malloc(sizeof(emex64_timer_t));

    if(timer == NULL)
    {
        return NULL;
    }

    /* register timer MMIO */
    E64MMIORegionRef TimerRegion = E64MMIORegionCreate(kEFAllocatorDefault, EMEX64_TIMER_BASE, EMEX64_TIMER_SIZE, timer, emex64_timer_read, emex64_timer_write);
    if(TimerRegion == NULL)
    {
        free(timer);
        return NULL;
    }

    Boolean success = E64MMIOBusRegisterRegion(machine->mmio_bus, TimerRegion);
    EFRelease(TimerRegion);
    if(!success)
    {
        free(timer);
        return NULL;
    }

    /* setting up timer */
    timer->machine = machine;
    timer->compare = UINT64_MAX;
    
    timer->host_freq = detect_host_freq();
    timer->last_host_cycles = emex64_get_host_cycles();
    
    return timer;
}

void emex64_timer_dealloc(emex64_timer_t *timer)
{
    free(timer);
}

void emex64_timer_tick(emex64_timer_t *timer,
                       UInt64 host_cycles)
{
    /* checking if timer is not enabled */
    if(!(timer->ctrl & TIMER_CTRL_ENABLE))
    {
        /* if it is then we simply forget about it!!! */
        timer->last_host_cycles = host_cycles;
        return;
    }
    
    /* calculate elappsed cycles */
    UInt64 elapsed_host = host_cycles - timer->last_host_cycles;
    timer->last_host_cycles = host_cycles;
    if(elapsed_host == 0)
    {
        return;
    }

    /*  calculating using virtual frequency the actual timer count */
    __uint128_t numerator = (__uint128_t)elapsed_host * TIMER_VIRTUAL_FREQ + timer->tick_remainder;
    UInt64 virtual_ticks = (UInt64)(numerator / timer->host_freq);
    timer->tick_remainder  = (UInt64)(numerator % timer->host_freq);
    if(virtual_ticks == 0)
    {
        return;
    }
    
    /* updating timer */
    UInt64 old_count = timer->count;
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
            emex64_raise_interrupt(timer->machine, EMEX64_IRQ_TIMER);
        }
    }
}

UInt64 emex64_timer_read(emex64_core_t *core,
                         void *device,
                         UInt64 offset,
                         int size)
{
    emex64_timer_t *timer = (emex64_timer_t *)device;

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
            return TIMER_VIRTUAL_FREQ;
        default:
            return 0;
    }
}

void emex64_timer_write(emex64_core_t *core,
                      void *device,
                      UInt64 offset,
                      UInt64 value,
                      int size)
{
    emex64_timer_t *timer = (emex64_timer_t *)device;

    switch(offset)
    {
        case TIMER_REG_CTRL:
            timer->ctrl = value;
            if(value & TIMER_CTRL_ENABLE)
            {
                timer->last_host_cycles = emex64_get_host_cycles();
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
        default:
            break;
    }
}
