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

#ifndef E64PCIE_H
#define E64PCIE_H

#include <EmexFoundation/EmexFoundation.h>

typedef struct __E64Machine *E64MachineRef;
typedef struct E64PCIeDevice *E64PCIeDeviceRef;

#define E64_PCIE_BARS               6
#define E64_PCIE_VENDOR_EMEXLABS    0xE640

typedef struct {
    UInt16 vendor;
    UInt16 device;
    UInt32 classCode;
    UInt8 revision;
    UInt64 barSize[E64_PCIE_BARS];
    void *context;
    UInt64 (*read)(void *context, UInt8 bar, UInt64 offset);
    void (*write)(void *context, UInt8 bar, UInt64 offset, UInt64 value);
    void (*free)(void *context);
} E64PCIeDeviceDescriptor;

E64PCIeDeviceRef E64MachineAttachPCIeDevice(E64MachineRef machineRef, const E64PCIeDeviceDescriptor *descriptor);
void E64PCIeDeviceRaiseInterrupt(E64PCIeDeviceRef deviceRef);
void *E64PCIeDeviceMapMemory(E64PCIeDeviceRef deviceRef, UInt64 paddr, UInt64 length, Boolean forWrite);

#endif /* E64PCIE_H */
