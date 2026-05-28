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

#include <stdlib.h>
#include <emex64vm/machine.h>

#include <emex64vm/device/rtc.h>
#include <emex64vm/device/platform.h>
#include <emex64vm/device/mc.h>

la64_machine_t *la64_machine_alloc(uint64_t memory_size)
{
    /* allocating brand new machine */
    la64_machine_t *machine = calloc(1, sizeof(la64_machine_t));
    if(machine == NULL)
    {
        return NULL;
    }

    /* allocate random access memory */
    machine->memory = la64_memory_alloc(memory_size);
    if(machine->memory == NULL)
    {
        goto out_release_machine;
    }

    /* allocating mmio controller */
    machine->mmio_bus = la64_mmio_alloc();
    if(machine->mmio_bus == NULL)
    {
        goto out_release_memory;
    }

    /* allocating main core */
    machine->core = la64_core_alloc();
    if(machine->core == NULL)
    {
        goto out_release_mmio;
    }
    machine->core->machine = machine;

    /* allocating devices*/
    machine->intc = la64_intc_alloc(machine);
    if(machine->intc == NULL)
    {
        goto out_release_core;
    }

    machine->timer = la64_timer_alloc(machine);
    if(machine->timer == NULL)
    {
        goto out_release_intc;
    }

    machine->uart = la64_uart_alloc(machine);
    if(machine->uart == NULL)
    {
        goto out_release_timer;
    }

    /* register device less(means without allocation) devices */
    if(!la64_mmio_register(machine->mmio_bus, LA64_RTC_BASE, LA64_RTC_SIZE, NULL, la64_rtc_read, NULL))
    {
        goto out_release_uart;
    }

    if(!la64_mmio_register(machine->mmio_bus, LA64_MC_BASE, LA64_MC_SIZE, NULL, la64_mc_read, NULL))
    {
        goto out_release_uart;
    }

    if(!la64_mmio_register(machine->mmio_bus, LA64_PLATFORM_BASE, LA64_PLATFORM_SIZE, NULL, la64_platform_read, la64_platform_write))
    {
        goto out_release_uart;
    }

    /* apple and linux have support for the LA64 LCD display */
#if defined(__linux__) || defined(__APPLE__)
    machine->display = la64_display_alloc(machine);

    if(machine->display == NULL)
    {
        goto out_release_uart;
    }
#endif /* __linux__ */

    return machine;

    /* much more compact error handling */
#if defined(__linux__)  || defined(__APPLE__)
out_release_display:
    la64_display_dealloc(machine->display);
#endif /* __linux__ */
out_release_uart:
    la64_uart_dealloc(machine->uart);
out_release_timer:
    la64_timer_dealloc(machine->timer);
out_release_intc:
    la64_intc_dealloc(machine->intc);
out_release_core:
    la64_core_dealloc(machine->core);
out_release_mmio:
    la64_mmio_dealloc(machine->mmio_bus);
out_release_memory:
    la64_memory_dealloc(machine->memory);
out_release_machine:
    free(machine);
    return NULL;
}

void la64_machine_dealloc(la64_machine_t *machine)
{
    /* release devices */
#if defined(__linux__)  || defined(__APPLE__)
    if(machine->display)
    {
        la64_display_dealloc(machine->display);
    }
#endif /* __linux__ */

    if(machine->uart)
    {
        la64_uart_dealloc(machine->uart);
    }

    if(machine->timer)
    {
        la64_timer_dealloc(machine->timer);
    }

    if(machine->intc)
    {
        la64_intc_dealloc(machine->intc);
    }

    /* releasing machine internals */
    la64_core_dealloc(machine->core);

    if(machine->mmio_bus)
    {
        la64_mmio_dealloc(machine->mmio_bus);
    }

    la64_memory_dealloc(machine->memory);

    /* release machine it self */
    free(machine);
}
