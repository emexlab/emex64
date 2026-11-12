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

#include <pthread.h>
#include <string.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/device/internal/controller/E64IC.h>
#include <EmexToolchain/VM/device/internal/controller/mem.h>
#include <EmexToolchain/VM/device/board/controller/power.h>
#include <EmexToolchain/VM/device/board/rtc.h>
#include <EmexToolchain/VM/device/internal/cpuid.h>
#include <EmexToolchain/VM/device/internal/timer.h>
#include <EmexToolchain/VM/device/board/uart.h>
#include <EmexToolchain/VM/device/board/controller/8042.h>
#include <EmexToolchain/VM/device/board/display.h>

static void __E64MachineDeinit(EFObjectRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;

    if(machine->net != NULL)
    {
        emex64_net_dealloc(machine->net);
    }

    for(UInt8 i = 0; i < machine->core_count; i++)
    {
        EFReleaseTry(machine->cores[i]);
    }
    machine->core = NULL;
    pthread_mutex_destroy(&machine->mmio_lock);
    pthread_mutex_destroy(&machine->reset_lock);
    pthread_cond_destroy(&machine->reset_cond);
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
    emex64_display_stop(machine->display);
    if(machine->gpu != NULL)
    {
        emex64_gpu_dealloc(machine->gpu);
    }
    if(machine->display != NULL)
    {
        emex64_display_dealloc(machine->display);
    }
    if(machine->emex8042 != NULL)
    {
        emex64_8042_dealloc(machine->emex8042);
    }
    if(machine->nvram != NULL)
    {
        emex64_nvram_dealloc(machine->nvram);
    }

    if(machine->sata != NULL)
    {
        emex64_sata_dealloc(machine->sata);
    }
    if(machine->usb != NULL)
    {
        emex64_usb_dealloc(machine->usb);
    }
    if(machine->pcie != NULL)
    {
        emex64_pcie_dealloc(machine->pcie);
    }
}

static EFClassDefinitionV4 E64MachineClass = {
    .header = {
        .version = 4,
        .typeID = kEFTypeIDNone,
        .name = EFSTR_FILESCOPE("E64Machine"),
    },
    .init = NULL,
    .deinit = __E64MachineDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = NULL,
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

static void __E64MachineWakeCore(void *context,
                                 UInt8 core)
{
    __E64Machine machine = (__E64Machine)context;
    if(core < machine->core_count)
    {
        __E64CoreKick((__E64Core)machine->cores[core]);
    }
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

    if(options.host != NULL)
    {
        machine->embedded = true;
        machine->host = *options.host;
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

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&machine->mmio_lock, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_mutex_init(&machine->reset_lock, NULL);
    pthread_cond_init(&machine->reset_cond, NULL);

    UInt8 cores = options.coreCount == 0 ? 1 : options.coreCount;
    if(cores > E64_MACHINE_MAX_CORES)
    {
        cores = E64_MACHINE_MAX_CORES;
    }
    machine->core = E64CoreCreateWithMachine(allocatorRef, (E64MachineRef)machine);
    if(machine->core == NULL)
    {
        return NULL;
    }
    machine->cores[0] = machine->core;
    machine->core_count = 1;
    for(UInt8 i = 1; i < cores; i++)
    {
        machine->cores[i] = __E64CoreCreateWithID(allocatorRef, (E64MachineRef)machine, i);
        if(machine->cores[i] == NULL)
        {
            return NULL;
        }
        machine->core_count++;
    }

    machine->intc = E64ICCreate(allocatorRef);
    if(machine->intc == NULL || !E64ICRegisterOnMMIOBus(machine->intc, machine->mmio_bus))
    {
        return NULL;
    }
    E64ICSetWakeHandler(machine->intc, __E64MachineWakeCore, machine);

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

    machine->pcie = emex64_pcie_alloc((E64MachineRef)machine);
    if(machine->pcie == NULL)
    {
        return NULL;
    }

    machine->sata = emex64_sata_alloc(machine->pcie, 1);
    if(machine->sata == NULL)
    {
        return NULL;
    }

    for(UInt8 i = 0; i < options.sataDiskCount && i < E64_MACHINE_MAX_SATA_DISKS; i++)
    {
        if(!emex64_sata_attach_disk(machine->sata, i, options.sataDisks[i].path, options.sataDisks[i].readOnly))
        {
            return NULL;
        }
    }

    machine->usb = emex64_usb_alloc(machine->pcie, 2);
    if(machine->usb == NULL)
    {
        return NULL;
    }

    for(UInt8 i = 0; i < options.usbDiskCount && i < E64_MACHINE_MAX_USB_DISKS; i++)
    {
        emex64_usb_device_t *disk = emex64_usb_msc_alloc(options.usbDisks[i].path, options.usbDisks[i].readOnly, i);
        if(disk == NULL)
        {
            return NULL;
        }
        if(!emex64_usb_attach(machine->usb, i, disk))
        {
            disk->ops->free(disk);
            return NULL;
        }
    }

    if(options.displayOptions.enabled)
    {
        machine->gpu = emex64_gpu_alloc(machine->pcie, 3, machine->display);
        if(machine->gpu == NULL)
        {
            return NULL;
        }
    }

    if(options.networkOptions.enabled)
    {
        machine->net = emex64_net_alloc(machine->pcie, 4, &options);
        if(machine->net == NULL)
        {
            return NULL;
        }
    }

    EFAUTOREL E64MMIORegionRef CPUIDRegion = E64MMIORegionCreate(NULL, EMEX64_CPUID_BASE, EMEX64_CPUID_SIZE, NULL, emex64_cpuid_read, NULL);
    if(CPUIDRegion == NULL || !E64MMIOBusRegisterRegion(machine->mmio_bus, CPUIDRegion))
    {
        return NULL;
    }

    EFAUTOREL E64MMIORegionRef RTCMMIORegion = E64MMIORegionCreate(NULL, EMEX64_RTC_BASE, EMEX64_RTC_SIZE, NULL, emex64_rtc_read, NULL);
    if(RTCMMIORegion == NULL || !E64MMIOBusRegisterRegion(machine->mmio_bus, RTCMMIORegion))
    {
        return NULL;
    }

    machine->nvram = emex64_nvram_alloc(options.nvramPath);
    if(machine->nvram == NULL)
    {
        return NULL;
    }

    EFAUTOREL E64MMIORegionRef NVRAMRegion = E64MMIORegionCreate(NULL, EMEX64_NVRAM_BASE, EMEX64_NVRAM_SIZE, machine->nvram, emex64_nvram_read, emex64_nvram_write);
    if(NVRAMRegion == NULL || !E64MMIOBusRegisterRegion(machine->mmio_bus, NVRAMRegion))
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

void __E64MachineResetHardware(__E64Machine machine)
{
    /* resetting devices */
    emex64_net_reset(machine->net);
    emex64_pcie_reset(machine->pcie);
    __E64ICReset(machine->intc);
    emex64_timer_reset(machine->timer);
    emex64_uart_reset(machine->uart);
    emex64_8042_reset(machine->emex8042);
    emex64_sata_reset(machine->sata);
    emex64_usb_reset(machine->usb);
    emex64_gpu_reset(machine->gpu);

    /* resetting memory */
    E64MemoryReset(machine->memory);

    /* resetting all cores */
    for(UInt8 i = 0; i < machine->core_count; i++)
    {
        __E64CoreResetForPowerOn((__E64Core)machine->cores[i]);
    }
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

void E64MachineStop(E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine != NULL && machine->core != NULL)
    {
        E64CoreTerminate(machine->core);
    }
}

void E64MachineSendUARTInput(E64MachineRef machineRef,
                             const UInt8 *bytes,
                             EFSize length)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL || machine->uart == NULL)
    {
        return;
    }
    for(EFSize i = 0; i < length; i++)
    {
        emex64_uart_receive(machine->uart, bytes[i]);
    }
}

void E64MachineSendKey(E64MachineRef machineRef,
                       kEmexKeyPhys key,
                       Boolean down)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL || machine->emex8042 == NULL)
    {
        return;
    }
    if(down)
    {
        emex64_8042_send_keyboard_make(machine->emex8042, key);
    }
    else
    {
        emex64_8042_send_keyboard_break(machine->emex8042, key);
    }
}

Boolean E64MachineGetDisplayFrame(E64MachineRef machineRef,
                                  E64DisplayFrame *frame)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL || machine->gpu == NULL || frame == NULL)
    {
        return false;
    }

    emex64_gpu_frame_t f;
    f.palette_gen = frame->paletteGeneration;
    emex64_gpu_get_frame(machine->gpu, &f);
    frame->active = f.active;
    frame->pixels = f.pixels;
    frame->width = f.width;
    frame->height = f.height;
    frame->stride = f.stride;
    frame->format = f.format;
    frame->dirtyTop = f.dirty_top;
    frame->dirtyBottom = f.dirty_bottom;
    if(f.palette_gen != frame->paletteGeneration)
    {
        memcpy(frame->palette, f.palette, sizeof(frame->palette));
        frame->paletteGeneration = f.palette_gen;
    }

    return true;
}
