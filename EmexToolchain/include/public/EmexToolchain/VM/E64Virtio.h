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

#ifndef E64VIRTIO_H
#define E64VIRTIO_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64PCIe.h>

#define E64_VIRTIO_PCIE_DEVICE      0x1AF4
#define E64_VIRTIO_MAX_QUEUES       8
#define E64_VIRTIO_MAX_CONFIG       256

#define E64_VIRTIO_ID_NET           1
#define E64_VIRTIO_ID_BLOCK         2
#define E64_VIRTIO_ID_CONSOLE       3
#define E64_VIRTIO_ID_RNG           4
#define E64_VIRTIO_ID_GPU           16
#define E64_VIRTIO_ID_INPUT         18

typedef struct E64VirtioDevice *E64VirtioDeviceRef;

typedef struct {
    UInt32 deviceID;
    UInt64 features;
    UInt8 queueCount;
    UInt16 queueSize;
    void *context;
    void (*notify)(void *context, E64VirtioDeviceRef deviceRef, UInt8 queue);
    void (*reset)(void *context, E64VirtioDeviceRef deviceRef);
    void (*free)(void *context);
} E64VirtioDeviceDescriptor;

E64VirtioDeviceRef E64MachineAttachVirtioDevice(E64MachineRef machineRef, const E64VirtioDeviceDescriptor *descriptor);

Boolean E64VirtioDeviceIsReady(E64VirtioDeviceRef deviceRef);
UInt64 E64VirtioDeviceGetFeatures(E64VirtioDeviceRef deviceRef);

void E64VirtioDeviceSetConfig(E64VirtioDeviceRef deviceRef, const void *bytes, UInt32 length);

SInt64 E64VirtioQueueReceive(E64VirtioDeviceRef deviceRef, UInt8 queue, void *buffer, UInt64 capacity);
UInt64 E64VirtioQueueSend(E64VirtioDeviceRef deviceRef, UInt8 queue, const void *bytes, UInt64 length);

#endif /* E64VIRTIO_H */
