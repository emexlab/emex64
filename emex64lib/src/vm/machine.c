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
#include <emex64lib/vm/machine.h>
#include <emex64lib/vm/device/internal/controller/mem.h>
#include <emex64lib/vm/device/board/controller/power.h>
#include <emex64lib/vm/device/board/rtc.h>

emex64_machine_t *emex64_machine_alloc(emex64_machine_options_t options)
{
    emex64_machine_t *machine = calloc(1, sizeof(emex64_machine_t));
    if(machine == NULL)
    {
        return NULL;
    }
    
    machine->memory = Emex64MemoryCreate(NULL, options.memory_size);
    if(machine->memory == NULL)
    {
        goto out_release_machine;
    }
    
    machine->mmio_bus = Emex64MMIOBusCreate(NULL);
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
    
    machine->emex8042 = emex64_8042_alloc(machine, options.keyboard_mode == kKeyboardMode8042, options.mouse_mode == kMouseMode8042);
    if(machine->emex8042 == NULL)
    {
        goto out_release_uart;
    }
    
    machine->display = emex64_display_alloc(machine, options.display.enabled, options.display.width, options.display.height);
    if(machine->display == NULL)
    {
        goto out_release_8042;
    }
    
    Emex64MMIORegionRef RTCMMIORegion = Emex64MMIORegionCreate(NULL, EMEX64_RTC_BASE, EMEX64_RTC_SIZE, NULL, emex64_rtc_read, NULL);
    if(RTCMMIORegion == NULL)
    {
        goto out_release_display;
    }

    bool success = Emex64MMIOBusRegisterRegion(machine->mmio_bus, RTCMMIORegion);
    EFRelease(RTCMMIORegion);
    if(!success)
    {
        goto out_release_display;
    }

    Emex64MMIORegionRef MCRegion = Emex64MMIORegionCreate(NULL, EMEX64_MC_BASE, EMEX64_MC_SIZE, NULL, emex64_mc_read, emex64_mc_write);
    if(MCRegion == NULL)
    {
        goto out_release_display;
    }
    
    success = Emex64MMIOBusRegisterRegion(machine->mmio_bus, MCRegion);
    EFRelease(MCRegion);
    if(!success)
    {
        goto out_release_display;
    }
    
    Emex64MMIORegionRef PlatformRegion = Emex64MMIORegionCreate(NULL, EMEX64_PLATFORM_BASE, EMEX64_PLATFORM_SIZE, NULL, emex64_platform_read, emex64_platform_write);
    if(PlatformRegion == NULL)
    {
        goto out_release_display;
    }
    
    success = Emex64MMIOBusRegisterRegion(machine->mmio_bus, PlatformRegion);
    EFRelease(PlatformRegion);
    if(!success)
    {
        goto out_release_display;
    }

    return machine;

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
    EFRelease(machine->mmio_bus);
out_release_memory:
    EFRelease(machine->memory);
out_release_machine:
    free(machine);
    return NULL;
}

void emex64_machine_dealloc(emex64_machine_t *machine)
{
    emex64_8042_dealloc(machine->emex8042);
    emex64_display_dealloc(machine->display);
    emex64_uart_dealloc(machine->uart);
    emex64_timer_dealloc(machine->timer);
    emex64_intc_dealloc(machine->intc);
    emex64_core_dealloc(machine->core);
    EFRelease(machine->mmio_bus);
    EFRelease(machine->memory);
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
    return support;
}

emex64_machine_options_t emex64_machine_options_default(void)
{
    emex64_machine_options_t options;
    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    options.display.enabled = true;
    options.keyboard_mode = kKeyboardMode8042;
    options.mouse_mode = kMouseMode8042;
    #else
    options.display.enabled = false;
    options.keyboard_mode = kKeyboardModeOff;
    options.mouse_mode = kMouseModeOff;
    #endif /* EMEX64VM_DEVICE_DISPLAY */
    options.display.width = 640;
    options.display.height = 480;
    options.memory_size = 100 * 1024 * 1024;
    return options;
}
