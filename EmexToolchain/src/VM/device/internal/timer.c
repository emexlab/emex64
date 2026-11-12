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

#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/likely.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/device/internal/timer.h>
#include <EmexToolchain/VM/device/internal/controller/E64IC.h>

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

static UInt64 host_freq_value;

static void host_freq_once(void)
{
    host_freq_value = detect_host_freq();
}

UInt64 emex64_host_freq(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, host_freq_once);
    return host_freq_value;
}

static inline UInt64 emex64_vtimer_ticks(UInt64 elapsed,
                                         UInt64 remainder,
                                         UInt64 host_freq,
                                         UInt64 *remainder_out)
{
    if(likely(elapsed <= (UINT64_MAX / 2) / TIMER_VIRTUAL_FREQ && remainder <= UINT64_MAX / 2))
    {
        UInt64 numerator = elapsed * TIMER_VIRTUAL_FREQ + remainder;
        if(remainder_out != NULL)
        {
            *remainder_out = numerator % host_freq;
        }
        return numerator / host_freq;
    }
    __uint128_t numerator = (__uint128_t)elapsed * TIMER_VIRTUAL_FREQ + remainder;
    if(remainder_out != NULL)
    {
        *remainder_out = (UInt64)(numerator % host_freq);
    }
    return (UInt64)(numerator / host_freq);
}

#define VTIMER_MAX_LOOKAHEAD    TIMER_VIRTUAL_FREQ

void emex64_vtimer_init(emex64_vtimer_t *t)
{
    t->ctrl = 0;
    t->count = 0;
    t->compare = UINT64_MAX;
    t->status = 0;
    t->tick_remainder = 0;
    t->min_elapsed = 0;
    t->last_host_cycles = emex64_get_host_cycles();
}

Boolean emex64_vtimer_advance(emex64_vtimer_t *t,
                              UInt64 host_cycles)
{
    UInt64 host_freq = emex64_host_freq();

    if(!(t->ctrl & TIMER_CTRL_ENABLE))
    {
        t->last_host_cycles = host_cycles;
        t->min_elapsed = host_freq / 100;
        return false;
    }

    if((SInt64)(host_cycles - t->last_host_cycles) <= 0)
    {
        return false;
    }

    UInt64 elapsed_host = host_cycles - t->last_host_cycles;
    t->last_host_cycles = host_cycles;

    UInt64 virtual_ticks = emex64_vtimer_ticks(elapsed_host, t->tick_remainder, host_freq, &t->tick_remainder);

    Boolean fired = false;
    if(virtual_ticks != 0)
    {
        /* updating timer */
        UInt64 old_count = t->count;
        t->count += virtual_ticks;

        Boolean periodic = (t->ctrl & TIMER_CTRL_PERIODIC) && t->compare != 0;
        if(t->count >= t->compare && (periodic || old_count < t->compare))
        {
            t->status |= TIMER_STATUS_IRQ;
            if(periodic)
            {
                t->count %= t->compare;
            }
            else
            {
                t->ctrl &= ~TIMER_CTRL_ENABLE;
            }
            fired = (t->ctrl & TIMER_CTRL_IRQ_EN) != 0;
        }
    }

    UInt64 ticks = VTIMER_MAX_LOOKAHEAD;
    if(t->ctrl & TIMER_CTRL_ENABLE)
    {
        if(t->compare > t->count && t->compare - t->count < ticks)
        {
            ticks = t->compare - t->count;
        }
        else if(t->compare <= t->count && (t->ctrl & TIMER_CTRL_PERIODIC) && t->compare != 0 && t->compare < ticks)
        {
            ticks = t->compare;
        }
    }
    __uint128_t need = (__uint128_t)ticks * host_freq;
    t->min_elapsed = need > t->tick_remainder ? (UInt64)((need - t->tick_remainder) / TIMER_VIRTUAL_FREQ) : 0;

    return fired;
}

UInt64 emex64_vtimer_read(emex64_vtimer_t *t,
                          UInt64 offset)
{
    switch(offset)
    {
        case TIMER_REG_CTRL:
            return t->ctrl;
        case TIMER_REG_COUNT:
            emex64_vtimer_advance(t, emex64_get_host_cycles());
            return t->count;
        case TIMER_REG_COMPARE:
            return t->compare;
        case TIMER_REG_STATUS:
            return t->status;
        case TIMER_REG_FREQ:
            return TIMER_VIRTUAL_FREQ;
        default:
            return 0;
    }
}

void emex64_vtimer_write(emex64_vtimer_t *t,
                         UInt64 offset,
                         UInt64 value)
{
    emex64_vtimer_advance(t, emex64_get_host_cycles());

    switch(offset)
    {
        case TIMER_REG_CTRL:
            t->ctrl = value;
            if(value & TIMER_CTRL_ENABLE)
            {
                t->last_host_cycles = emex64_get_host_cycles();
            }
            break;
        case TIMER_REG_COUNT:
            t->count = value;
            break;
        case TIMER_REG_COMPARE:
            t->compare = value;
            break;
        case TIMER_REG_STATUS:
            t->status &= ~value;
            break;
        default:
            break;
    }

    t->min_elapsed = 0;
}

static void emex64_timer_publish(emex64_timer_t *timer);

emex64_timer_t *emex64_timer_alloc(E64MachineRef machine)
{
    /* allocate timer */
    emex64_timer_t *timer = calloc(1, sizeof(emex64_timer_t));
    if(timer == NULL)
    {
        return NULL;
    }

    /* register timer MMIO */
    EFAUTOREL E64MMIORegionRef TimerRegion = E64MMIORegionCreate(kEFAllocatorDefault, EMEX64_TIMER_BASE, EMEX64_TIMER_SIZE, timer, emex64_timer_read, emex64_timer_write);
    if(TimerRegion == NULL || !E64MMIOBusRegisterRegion(machine->mmio_bus, TimerRegion))
    {
        free(timer);
        return NULL;
    }

    /* setting up timer */
    timer->machine = machine;
    pthread_mutex_init(&timer->lock, NULL);
    emex64_vtimer_init(&timer->vt);
    emex64_timer_publish(timer);

    return timer;
}

void emex64_timer_reset(emex64_timer_t *timer)
{
    pthread_mutex_lock(&timer->lock);
    emex64_vtimer_init(&timer->vt);
    emex64_timer_publish(timer);
    pthread_mutex_unlock(&timer->lock);
}

void emex64_timer_dealloc(emex64_timer_t *timer)
{
    if(timer != NULL)
    {
        pthread_mutex_destroy(&timer->lock);
    }
    free(timer);
}

static void emex64_timer_publish(emex64_timer_t *timer)
{
    const emex64_vtimer_t *t = &timer->vt;
    UInt64 mode = EMEX64_TIMER_SNAP_SLOW;
    if(!(t->ctrl & TIMER_CTRL_ENABLE))
    {
        mode = EMEX64_TIMER_SNAP_STOPPED;
    }
    else if(!(t->ctrl & TIMER_CTRL_PERIODIC) || t->compare == 0)
    {
        mode = EMEX64_TIMER_SNAP_COUNTING;
    }

    UInt64 seq = timer->seq;
    __atomic_store_n(&timer->seq, seq + 1, __ATOMIC_RELAXED);
    __atomic_thread_fence(__ATOMIC_RELEASE);
    __atomic_store_n(&timer->snap_count, t->count, __ATOMIC_RELAXED);
    __atomic_store_n(&timer->snap_host, t->last_host_cycles, __ATOMIC_RELAXED);
    __atomic_store_n(&timer->snap_remainder, t->tick_remainder, __ATOMIC_RELAXED);
    __atomic_store_n(&timer->snap_compare, t->compare, __ATOMIC_RELAXED);
    __atomic_store_n(&timer->snap_mode, mode, __ATOMIC_RELAXED);
    __atomic_store_n(&timer->seq, seq + 2, __ATOMIC_RELEASE);
}

Boolean emex64_timer_read_count(emex64_timer_t *timer,
                                UInt64 *value)
{
    UInt64 seq, count, host, remainder, compare, mode;
    for(;;)
    {
        seq = __atomic_load_n(&timer->seq, __ATOMIC_ACQUIRE);
        if(unlikely(seq & 1))
        {
            return false;
        }
        count = __atomic_load_n(&timer->snap_count, __ATOMIC_RELAXED);
        host = __atomic_load_n(&timer->snap_host, __ATOMIC_RELAXED);
        remainder = __atomic_load_n(&timer->snap_remainder, __ATOMIC_RELAXED);
        compare = __atomic_load_n(&timer->snap_compare, __ATOMIC_RELAXED);
        mode = __atomic_load_n(&timer->snap_mode, __ATOMIC_RELAXED);
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        if(likely(__atomic_load_n(&timer->seq, __ATOMIC_RELAXED) == seq))
        {
            break;
        }
    }

    switch(mode)
    {
        case EMEX64_TIMER_SNAP_STOPPED:
            *value = count;
            return true;
        case EMEX64_TIMER_SNAP_COUNTING:
        {
            /* exactly what advancing to now would count */
            UInt64 now = emex64_get_host_cycles();
            UInt64 v = count;
            if((SInt64)(now - host) > 0)
            {
                v += emex64_vtimer_ticks(now - host, remainder, emex64_host_freq(), NULL);
            }

            /* crossing compare stops a one shot timer there */
            if(unlikely(count < compare && v >= compare))
            {
                return false;
            }
            *value = v;
            return true;
        }
        default:
            return false;
    }
}

void emex64_timer_tick(emex64_timer_t *timer,
                       UInt64 host_cycles)
{
    if(likely(host_cycles - timer->vt.last_host_cycles < timer->vt.min_elapsed))
    {
        return;
    }

    pthread_mutex_lock(&timer->lock);
    Boolean fired = emex64_vtimer_advance(&timer->vt, host_cycles);
    emex64_timer_publish(timer);
    pthread_mutex_unlock(&timer->lock);

    if(fired)
    {
        E64ICRaiseInterrupt(timer->machine->intc, EMEX64_IRQ_TIMER);
    }
}

UInt64 emex64_timer_read(E64CoreRef core,
                         void *device,
                         UInt64 offset,
                         EFSize size)
{
    emex64_timer_t *timer = (emex64_timer_t *)device;
    pthread_mutex_lock(&timer->lock);
    UInt64 value = emex64_vtimer_read(&timer->vt, offset);
    emex64_timer_publish(timer);
    pthread_mutex_unlock(&timer->lock);
    return value;
}

void emex64_timer_write(E64CoreRef core,
                        void *device,
                        UInt64 offset,
                        UInt64 value,
                        EFSize size)
{
    emex64_timer_t *timer = (emex64_timer_t *)device;
    pthread_mutex_lock(&timer->lock);
    emex64_vtimer_write(&timer->vt, offset, value);
    emex64_timer_publish(timer);
    pthread_mutex_unlock(&timer->lock);
}
