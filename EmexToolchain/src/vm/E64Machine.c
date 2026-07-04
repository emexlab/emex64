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
#include <EmexToolchain/vm/E64Machine.h>
#include <EmexToolchain/vm/device/internal/controller/mem.h>
#include <EmexToolchain/vm/device/board/controller/power.h>
#include <EmexToolchain/vm/device/board/rtc.h>
#include <EmexToolchain/vm/device/internal/timer.h>
#include <EmexToolchain/vm/device/internal/controller/ic.h>
#include <EmexToolchain/vm/device/board/uart.h>
#include <EmexToolchain/vm/device/board/controller/8042.h>
#include <EmexToolchain/vm/device/board/display.h>

static void __E64MachineDeinit(EFObjectRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine->core != NULL)
    {
        EFRelease(machine->core);
    }
    if(machine->memory != NULL)
    {
        EFRelease(machine->memory);
    }
    if(machine->mmio_bus != NULL)
    {
        EFRelease(machine->mmio_bus);
    }
    if(machine->intc != NULL)
    {
        emex64_intc_dealloc(machine->intc);
    }
    if(machine->timer != NULL)
    {
        emex64_timer_dealloc(machine->timer);
    }
    if(machine->uart != NULL)
    {
        emex64_uart_dealloc(machine->uart);
    }
    if(machine->display != NULL)
    {
        emex64_display_dealloc(machine->display);
    }
    if(machine->emex8042 != NULL)
    {
        emex64_8042_dealloc(machine->emex8042);
    }
}

static EFClass E64MachineClass = {
    .name = "E64Machine",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __E64MachineDeinit,
    .equal = NULL,
    .copyDescription = NULL,
};

static void E64MachineRegisterClass(void)
{
    EFClassRegister(&E64MachineClass);
}

EFTypeID E64MachineGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, E64MachineRegisterClass);
    return E64MachineClass.typeID;
}

E64MachineRef E64MachineCreate(EFAllocatorRef allocatorRef)
{
    return E64MachineCreateWithOptions(allocatorRef, E64MachineOptionsGetDefault());
}

E64MachineRef E64MachineCreateWithOptions(EFAllocatorRef allocatorRef,
                                          E64MachineOptions options)
{
    __E64Machine machine = (__E64Machine)EFObjectAlloc(allocatorRef, E64MachineGetTypeID(), sizeof(struct __E64Machine));
    if(machine == NULL)
    {
        return NULL;
    }

    machine->memory = E64MemoryCreate(allocatorRef, options.memoryLength);
    if(machine->memory == NULL)
    {
        EFRelease(machine);
    }

    machine->mmio_bus = E64MMIOBusCreate(allocatorRef);
    if(machine->mmio_bus == NULL)
    {
        EFRelease(machine);
    }

    machine->core = E64CoreCreate(allocatorRef);
    if(machine->core == NULL)
    {
        EFRelease(machine);
    }
    /* machine->core->machine = EFRetain(machine); FIXME: retain cycle */
    machine->core->machine = machine;

    machine->intc = emex64_intc_alloc(machine);
    if(machine->intc == NULL)
    {
        EFRelease(machine);
    }

    machine->timer = emex64_timer_alloc(machine);
    if(machine->timer == NULL)
    {
        EFRelease(machine);
    }

    machine->uart = emex64_uart_alloc(machine);
    if(machine->uart == NULL)
    {
        EFRelease(machine);
    }

    machine->emex8042 = emex64_8042_alloc(machine, options.keyboardPeripheralMode == kE64PeripheralMode8042, options.mousePeripheralMode == kE64PeripheralMode8042);
    if(machine->emex8042 == NULL)
    {
        EFRelease(machine);
    }

    machine->display = emex64_display_alloc(machine, options.displayOptions.enabled, options.displayOptions.width, options.displayOptions.height);
    if(machine->display == NULL)
    {
        EFRelease(machine);
    }

    E64MMIORegionRef RTCMMIORegion = E64MMIORegionCreate(NULL, EMEX64_RTC_BASE, EMEX64_RTC_SIZE, NULL, emex64_rtc_read, NULL);
    if(RTCMMIORegion == NULL)
    {
        EFRelease(machine);
    }

    Boolean success = E64MMIOBusRegisterRegion(machine->mmio_bus, RTCMMIORegion);
    EFRelease(RTCMMIORegion);
    if(!success)
    {
        EFRelease(machine);
    }

    E64MMIORegionRef MCRegion = E64MMIORegionCreate(NULL, EMEX64_MC_BASE, EMEX64_MC_SIZE, NULL, emex64_mc_read, emex64_mc_write);
    if(MCRegion == NULL)
    {
        EFRelease(machine);
    }
    
    success = E64MMIOBusRegisterRegion(machine->mmio_bus, MCRegion);
    EFRelease(MCRegion);
    if(!success)
    {
        EFRelease(machine);
    }

    E64MMIORegionRef PlatformRegion = E64MMIORegionCreate(NULL, EMEX64_PLATFORM_BASE, EMEX64_PLATFORM_SIZE, NULL, emex64_platform_read, emex64_platform_write);
    if(PlatformRegion == NULL)
    {
        EFRelease(machine);
    }
    
    success = E64MMIOBusRegisterRegion(machine->mmio_bus, PlatformRegion);
    EFRelease(PlatformRegion);
    if(!success)
    {
        EFRelease(machine);
    }

    return machine;
}

E64CoreRef E64MachineGetCore(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL)
    {
        return NULL;
    }

    return machine->core;
}

E64MemoryRef E64MachineGetMemory(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL)
    {
        return NULL;
    }

    return machine->memory;
}

E64MMIOBusRef E64MachineGetMMIOBus(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL)
    {
        return NULL;
    }

    return machine->mmio_bus;
}

emex64_intc_t *E64MachineGetIC(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL)
    {
        return NULL;
    }

    return machine->intc;
}

emex64_timer_t *E64MachineGetTimer(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL)
    {
        return NULL;
    }

    return machine->timer;
}

emex64_uart_t *E64MachineGetUART(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL)
    {
        return NULL;
    }

    return machine->uart;
}

emex64_display_t *E64MachineGetDisplay(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL)
    {
        return NULL;
    }

    return machine->display;
}

emex64_8042_t *E64MachineGet8042(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL)
    {
        return NULL;
    }

    return machine->emex8042;
}
