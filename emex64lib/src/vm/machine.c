/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
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
#include <emex64lib/vm/machine.h>

#include <emex64lib/vm/device/rtc.h>
#include <emex64lib/vm/device/platform.h>
#include <emex64lib/vm/device/mc.h>
#include <emex64lib/vm/device/ac97.h>

emex64_machine_t *emex64_machine_alloc(emex64_machine_options_t options)
{
    emex64_machine_t *machine = calloc(1, sizeof(emex64_machine_t));
    if(machine == NULL)
    {
        return NULL;
    }

    machine->memory = emex64_memory_alloc(options.memory_size);
    if(machine->memory == NULL)
    {
        goto out_release_machine;
    }

    machine->mmio_bus = emex64_mmio_alloc();
    if(machine->mmio_bus == NULL)
    {
        goto out_release_memory;
    }

    machine->core = emex64_core_alloc();
    if(machine->core == NULL)
    {
        goto out_release_mmio;
    }
    machine->core->machine = machine;

    machine->intc = emex64_intc_alloc(machine);
    if(machine->intc == NULL)
    {
        goto out_release_core;
    }

    machine->timer = emex64_timer_alloc(machine);
    if(machine->timer == NULL)
    {
        goto out_release_intc;
    }

    machine->uart = emex64_uart_alloc(machine);
    if(machine->uart == NULL)
    {
        goto out_release_timer;
    }

    machine->emex8042 = emex64_8042_alloc(machine);
    if(machine->emex8042 == NULL)
    {
        goto out_release_uart;
    }

    machine->display = emex64_display_alloc(machine, options.display.enabled, options.display.width, options.display.height);
    if(machine->display == NULL)
    {
        goto out_release_8042;
    }

    machine->ac97 = emex64_ac97_alloc(machine, options.audio);
    if(machine->ac97 == NULL)
    {
        goto out_release_display;
    }

    if(!emex64_mmio_register(machine->mmio_bus, EMEX64_RTC_BASE, EMEX64_RTC_SIZE, NULL, emex64_rtc_read, NULL) ||
       !emex64_mmio_register(machine->mmio_bus, EMEX64_MC_BASE, EMEX64_MC_SIZE, NULL, emex64_mc_read, emex64_mc_write) ||
       !emex64_mmio_register(machine->mmio_bus, EMEX64_PLATFORM_BASE, EMEX64_PLATFORM_SIZE, NULL, emex64_platform_read, emex64_platform_write))
    {
        goto out_release_ac97;
    }

    return machine;

out_release_ac97:
    emex64_ac97_dealloc(machine->ac97);
out_release_display:
    emex64_display_dealloc(machine->display);
out_release_8042:
    emex64_8042_dealloc(machine->emex8042);
out_release_uart:
    emex64_uart_dealloc(machine->uart);
out_release_timer:
    emex64_timer_dealloc(machine->timer);
out_release_intc:
    emex64_intc_dealloc(machine->intc);
out_release_core:
    emex64_core_dealloc(machine->core);
out_release_mmio:
    emex64_mmio_dealloc(machine->mmio_bus);
out_release_memory:
    emex64_memory_dealloc(machine->memory);
out_release_machine:
    free(machine);
    return NULL;
}

void emex64_machine_dealloc(emex64_machine_t *machine)
{
    emex64_ac97_dealloc(machine->ac97);
    emex64_8042_dealloc(machine->emex8042);
    emex64_display_dealloc(machine->display);
    emex64_uart_dealloc(machine->uart);
    emex64_timer_dealloc(machine->timer);
    emex64_intc_dealloc(machine->intc);
    emex64_core_dealloc(machine->core);
    emex64_mmio_dealloc(machine->mmio_bus);
    emex64_memory_dealloc(machine->memory);
    free(machine);
}

emex64_machine_support_t emex64_machine_support_get(void)
{
    emex64_machine_support_t support;
    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    support.display = true;
    #else
    support.display = false;
    #endif /* EMEX64VM_DEVICE_DISPLAY */
    #if EMEX64VM_DEVICE_AUDIO && (defined(__linux__) || defined(__APPLE__))
    support.audio = true;
    #else
    support.audio = false;
    #endif /* EMEX64VM_DEVICE_AUDIO */
    return support;
}

emex64_machine_options_t emex64_machine_options_default(void)
{
    emex64_machine_options_t options;
    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    options.display.enabled = true;
    options.display.width = 640;
    options.display.height = 480;
    #else
    options.display.enabled = false;
    #endif /* EMEX64VM_DEVICE_DISPLAY */
    #if EMEX64VM_DEVICE_AUDIO && (defined(__linux__) || defined(__APPLE__))
    options.audio = true;
    #else
    options.audio = false;
    #endif /* EMEX64VM_DEVICE_AUDIO */
    options.memory_size = 100 * 1024 * 1024;
    return options;
}
