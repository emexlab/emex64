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

#ifndef E64MACHINE_H
#define E64MACHINE_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETVirtualMachine/E64MachineOptions.h>
#include <EmexToolchain/ETVirtualMachine/E64Core.h>
#include <EmexToolchain/ETVirtualMachine/E64Memory.h>
#include <EmexToolchain/ETVirtualMachine/E64MMIOBus.h>
#include <EmexToolchain/ETVirtualMachine/device/base.h>
#include <EmexToolchain/Support/keyboard.h>
#include <EmexToolchain/ETVirtualMachine/device/internal/controller/E64IC.h>
#ifdef ET_PRIVATE
#include <EmexToolchain/ETVirtualMachine/__E64Machine.h>
#endif /* ET_PRIVATE */

typedef struct __E64Machine *E64MachineRef;

EFTypeID E64MachineGetTypeID(void);

E64MachineRef E64MachineCreate(EFAllocatorRef allocatorRef);
E64MachineRef E64MachineCreateWithOptions(EFAllocatorRef allocatorRef, E64MachineOptions options);

UInt8 E64MachineGetMaxCoreCount(void);

UInt8 E64MachineGetCoreCount(E64MachineRef machineRef);
E64CoreRef E64MachineGetCore(E64MachineRef machineRef);
E64CoreRef E64MachineGetCoreWithIdentifier(E64MachineRef machineRef, UInt8 identifier);
E64MemoryRef E64MachineGetMemory(E64MachineRef machineRef);
E64MMIOBusRef E64MachineGetMMIOBus(E64MachineRef machineRef);

E64ICRef E64MachineGetIC(E64MachineRef machineRef);

E64Exception E64MachineExecute(E64MachineRef machineRef);
void E64MachineTerminate(E64MachineRef machineRef);

void E64MachineSendUARTInput(E64MachineRef machineRef, const UInt8 *bytes, EFSize length);

void E64MachineSendKey(E64MachineRef machineRef, kEmexKeyPhys key, Boolean down);
void E64MachineSendMouse(E64MachineRef machineRef, SInt32 dx, SInt32 dy, UInt8 buttons);

SInt32 E64MachinePlugUSBDisk(E64MachineRef machineRef, const char *path, Boolean readOnly);
Boolean E64MachineUnplugUSBDisk(E64MachineRef machineRef, UInt8 port);

typedef struct {
    Boolean active;
    const UInt8 *pixels;
    UInt32 width, height;
    UInt32 stride;
    UInt32 format;
    UInt32 paletteGeneration;
    UInt8 palette[256 * 3];
    UInt32 dirtyTop, dirtyBottom;
} E64DisplayFrame;

Boolean E64MachineGetDisplayFrame(E64MachineRef machineRef, E64DisplayFrame *frame);

#endif /* E64MACHINE_H */
