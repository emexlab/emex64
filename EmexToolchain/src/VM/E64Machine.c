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

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/device/internal/controller/E64IC.h>
#include <EmexToolchain/VM/device/internal/controller/mem.h>
#include <EmexToolchain/VM/device/board/controller/power.h>
#include <EmexToolchain/VM/device/board/rtc.h>
#include <EmexToolchain/VM/device/internal/timer.h>
#include <EmexToolchain/VM/device/board/uart.h>
#include <EmexToolchain/VM/device/board/controller/8042.h>
#include <EmexToolchain/VM/device/board/display.h>

static void __E64MachineDeinit(EFObjectRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    EFReleaseTry(machine->core);
    EFReleaseTry(machine->memory);
    EFReleaseTry(machine->mmio_bus);
    EFReleaseTry(machine->intc);
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

static EFClassDefinitionV2 E64MachineClass = {
    .header = {
        .version = 2,
        .typeID = kEFTypeIDNone,
        .name = NULL,
    },
    .name = "E64Machine",
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
    return E64MachineClass.header.typeID;
}

E64MachineRef E64MachineCreate(EFAllocatorRef allocatorRef)
{
    return E64MachineCreateWithOptions(allocatorRef, E64MachineOptionsDefault);
}

E64MachineRef E64MachineCreateWithOptions(EFAllocatorRef allocatorRef,
                                          E64MachineOptions options)
{
    EFAUTOREL __E64Machine machine = (__E64Machine)EFObjectCreate(allocatorRef, E64MachineGetTypeID(), (EFIndex)sizeof(struct __E64Machine));
    if(machine == NULL)
    {
        return NULL;
    }

    machine->memory = E64MemoryCreate(allocatorRef, options.memoryLength);
    if(machine->memory == NULL)
    {
        return NULL;
    }

    machine->mmio_bus = E64MMIOBusCreate(allocatorRef);
    if(machine->mmio_bus == NULL)
    {
        return NULL;
    }

    machine->core = E64CoreCreateWithMachine(allocatorRef, (E64MachineRef)machine);
    if(machine->core == NULL)
    {
        return NULL;
    }

    machine->intc = E64ICCreate(allocatorRef);
    if(machine->intc == NULL || !E64ICRegisterOnMMIOBus(machine->intc, machine->mmio_bus))
    {
        return NULL;
    }

    machine->timer = emex64_timer_alloc(machine);
    if(machine->timer == NULL)
    {
        return NULL;
    }

    machine->uart = emex64_uart_alloc(machine);
    if(machine->uart == NULL)
    {
        return NULL;
    }

    machine->emex8042 = emex64_8042_alloc(machine, options.keyboardPeripheralMode == kE64PeripheralMode8042, options.mousePeripheralMode == kE64PeripheralMode8042);
    if(machine->emex8042 == NULL)
    {
        return NULL;
    }

    machine->display = emex64_display_alloc(machine, options.displayOptions.enabled, options.displayOptions.width, options.displayOptions.height);
    if(machine->display == NULL)
    {
        return NULL;
    }

    EFAUTOREL E64MMIORegionRef RTCMMIORegion = E64MMIORegionCreate(NULL, EMEX64_RTC_BASE, EMEX64_RTC_SIZE, NULL, emex64_rtc_read, NULL);
    if(RTCMMIORegion == NULL || !E64MMIOBusRegisterRegion(machine->mmio_bus, RTCMMIORegion))
    {
        return NULL;
    }

    EFAUTOREL E64MMIORegionRef MCRegion = E64MMIORegionCreate(NULL, EMEX64_MC_BASE, EMEX64_MC_SIZE, NULL, emex64_mc_read, emex64_mc_write);
    if(MCRegion == NULL || !E64MMIOBusRegisterRegion(machine->mmio_bus, MCRegion))
    {
        return NULL;
    }

    EFAUTOREL E64MMIORegionRef PlatformRegion = E64MMIORegionCreate(NULL, EMEX64_PLATFORM_BASE, EMEX64_PLATFORM_SIZE, NULL, emex64_platform_read, emex64_platform_write);
    if(PlatformRegion == NULL || !E64MMIOBusRegisterRegion(machine->mmio_bus, PlatformRegion))
    {
        return NULL;
    }

    return (E64MachineRef)EFAUTOTRANSFER(machine);
}

E64CoreRef E64MachineGetCore(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    return machine != NULL ? machine->core : NULL;
}

E64MemoryRef E64MachineGetMemory(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    return machine != NULL ? machine->memory : NULL;
}

E64MMIOBusRef E64MachineGetMMIOBus(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    return machine != NULL ? machine->mmio_bus : NULL;
}

E64ICRef E64MachineGetIC(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    return machine != NULL ? machine->intc : NULL;
}
