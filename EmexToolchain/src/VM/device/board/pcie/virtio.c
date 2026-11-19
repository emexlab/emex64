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
#include <stdlib.h>
#include <string.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Virtio.h>

#define VIRTIO_REG_MAGIC            0x00
#define VIRTIO_REG_DEVICE           0x08
#define VIRTIO_REG_DEVICE_FEATURES  0x10
#define VIRTIO_REG_DRIVER_FEATURES  0x18
#define VIRTIO_REG_QUEUE_SEL        0x20
#define VIRTIO_REG_QUEUE_NUM_MAX    0x28
#define VIRTIO_REG_QUEUE_NUM        0x30
#define VIRTIO_REG_QUEUE_READY      0x38
#define VIRTIO_REG_QUEUE_NOTIFY     0x40
#define VIRTIO_REG_INT_STATUS       0x48
#define VIRTIO_REG_INT_ACK          0x50
#define VIRTIO_REG_STATUS           0x58
#define VIRTIO_REG_QUEUE_DESC       0x60
#define VIRTIO_REG_QUEUE_DRIVER     0x68
#define VIRTIO_REG_QUEUE_DEVICE     0x70
#define VIRTIO_REG_CONFIG_GEN       0x78
#define VIRTIO_REG_CONFIG           0x100

#define VIRTIO_BAR_SIZE             0x2000
#define VIRTIO_MAGIC                0x74726976ULL
#define VIRTIO_VERSION              2ULL
#define VIRTIO_F_VERSION_1          (1ULL << 32)
#define VIRTIO_DEVICE_FEATURE_MASK  0xFFFFFFULL
#define VIRTIO_STATUS_DRIVER_OK     4
#define VIRTIO_INT_QUEUE            1
#define VIRTIO_INT_CONFIG           2
#define VIRTQ_DESC_F_NEXT           1
#define VIRTQ_DESC_F_WRITE          2
#define VIRTQ_AVAIL_F_NO_INTERRUPT  1
#define VIRTIO_PCIE_CLASS           0xFF0000

typedef struct {
    UInt16 num;
    Boolean ready;
    UInt64 desc, driver, device;
    UInt16 last;
} virtio_queue_t;

struct E64VirtioDevice {
    E64PCIeDeviceRef pcie;
    E64VirtioDeviceDescriptor descriptor;
    pthread_mutex_t lock;
    UInt64 driver_features;
    UInt8 queue_sel;
    UInt64 status;
    UInt64 interrupts;
    UInt64 generation;
    UInt16 queue_max;
    virtio_queue_t queues[E64_VIRTIO_MAX_QUEUES];
    UInt8 config[E64_VIRTIO_MAX_CONFIG];
};

static UInt16 rd16(const UInt8 *p)
{
    return (UInt16)(p[0] | (p[1] << 8));
}

static UInt32 rd32(const UInt8 *p)
{
    return (UInt32)p[0] | ((UInt32)p[1] << 8) | ((UInt32)p[2] << 16) | ((UInt32)p[3] << 24);
}

static UInt64 rd64(const UInt8 *p)
{
    return (UInt64)rd32(p) | ((UInt64)rd32(p + 4) << 32);
}

static void wr16(UInt8 *p,
                 UInt16 v)
{
    p[0] = (UInt8)v;
    p[1] = (UInt8)(v >> 8);
}

static void wr32(UInt8 *p,
                 UInt32 v)
{
    p[0] = (UInt8)v;
    p[1] = (UInt8)(v >> 8);
    p[2] = (UInt8)(v >> 16);
    p[3] = (UInt8)(v >> 24);
}

static void virtio_interrupt(E64VirtioDeviceRef dev,
                             UInt64 what)
{
    dev->interrupts |= what;
    E64PCIeDeviceRaiseInterrupt(dev->pcie);
}

static void virtio_reset(E64VirtioDeviceRef dev)
{
    dev->driver_features = 0;
    dev->queue_sel = 0;
    dev->status = 0;
    dev->interrupts = 0;
    memset(dev->queues, 0, sizeof(dev->queues));
}

static virtio_queue_t *virtio_queue(E64VirtioDeviceRef dev,
                                    UInt8 queue)
{
    if(queue >= dev->descriptor.queueCount || queue >= E64_VIRTIO_MAX_QUEUES)
    {
        return NULL;
    }
    virtio_queue_t *q = &dev->queues[queue];
    if(!q->ready || q->num == 0 || !(dev->status & VIRTIO_STATUS_DRIVER_OK))
    {
        return NULL;
    }
    return q;
}

static Boolean virtio_pop(E64VirtioDeviceRef dev,
                          virtio_queue_t *q,
                          UInt16 *head)
{
    UInt8 *ring = E64PCIeDeviceMapMemory(dev->pcie, q->driver, 4 + 2 * (UInt64)q->num, false);
    if(ring == NULL || q->last == rd16(ring + 2))
    {
        return false;
    }
    *head = rd16(ring + 4 + 2 * (q->last % q->num));
    q->last++;
    return *head < q->num;
}

static void virtio_push(E64VirtioDeviceRef dev,
                        virtio_queue_t *q,
                        UInt16 head,
                        UInt32 written)
{
    UInt8 *ring = E64PCIeDeviceMapMemory(dev->pcie, q->device, 4 + 8 * (UInt64)q->num, true);
    if(ring == NULL)
    {
        return;
    }
    UInt16 index = rd16(ring + 2);
    UInt8 *entry = ring + 4 + 8 * (index % q->num);
    wr32(entry, head);
    wr32(entry + 4, written);
    wr16(ring + 2, (UInt16)(index + 1));

    const UInt8 *avail = E64PCIeDeviceMapMemory(dev->pcie, q->driver, 2, false);
    if(avail != NULL && (rd16(avail) & VIRTQ_AVAIL_F_NO_INTERRUPT))
    {
        return;
    }
    virtio_interrupt(dev, VIRTIO_INT_QUEUE);
}

static UInt64 virtio_copy(E64VirtioDeviceRef dev,
                          virtio_queue_t *q,
                          UInt16 head,
                          Boolean out,
                          UInt8 *data,
                          UInt64 length)
{
    UInt64 done = 0;
    UInt16 index = head;
    for(UInt16 hops = 0; hops < q->num; hops++)
    {
        UInt8 *d = E64PCIeDeviceMapMemory(dev->pcie, q->desc + 16 * (UInt64)index, 16, false);
        if(d == NULL)
        {
            break;
        }
        UInt64 addr = rd64(d);
        UInt32 len = rd32(d + 8);
        UInt16 flags = rd16(d + 12);
        UInt16 next = rd16(d + 14);

        if(((flags & VIRTQ_DESC_F_WRITE) != 0) == (out != false) && done < length && len != 0)
        {
            UInt64 n = length - done < len ? length - done : len;
            UInt8 *p = E64PCIeDeviceMapMemory(dev->pcie, addr, n, out);
            if(p != NULL)
            {
                if(out)
                {
                    memcpy(p, data + done, n);
                }
                else
                {
                    memcpy(data + done, p, n);
                }
                done += n;
            }
        }

        if(!(flags & VIRTQ_DESC_F_NEXT) || next >= q->num)
        {
            break;
        }
        index = next;
    }
    return done;
}

static UInt64 virtio_read(void *context,
                          UInt8 bar,
                          UInt64 offset)
{
    E64VirtioDeviceRef dev = context;
    UInt64 value = 0;
    if(bar != 0)
    {
        return 0;
    }

    pthread_mutex_lock(&dev->lock);
    virtio_queue_t *q = &dev->queues[dev->queue_sel];
    switch(offset)
    {
        case VIRTIO_REG_MAGIC:
            value = VIRTIO_MAGIC | (VIRTIO_VERSION << 32);
            break;
        case VIRTIO_REG_DEVICE:
            value = dev->descriptor.deviceID | ((UInt64)E64_PCIE_VENDOR_EMEXLABS << 32);
            break;
        case VIRTIO_REG_DEVICE_FEATURES:
            value = (dev->descriptor.features & VIRTIO_DEVICE_FEATURE_MASK) | VIRTIO_F_VERSION_1;
            break;
        case VIRTIO_REG_QUEUE_NUM_MAX:
            value = dev->queue_sel < dev->descriptor.queueCount ? dev->queue_max : 0;
            break;
        case VIRTIO_REG_QUEUE_READY:
            value = q->ready;
            break;
        case VIRTIO_REG_INT_STATUS:
            value = dev->interrupts;
            break;
        case VIRTIO_REG_STATUS:
            value = dev->status;
            break;
        case VIRTIO_REG_QUEUE_DESC:
            value = q->desc;
            break;
        case VIRTIO_REG_QUEUE_DRIVER:
            value = q->driver;
            break;
        case VIRTIO_REG_QUEUE_DEVICE:
            value = q->device;
            break;
        case VIRTIO_REG_CONFIG_GEN:
            value = dev->generation;
            break;
        default:
            if(offset >= VIRTIO_REG_CONFIG && offset + 8 <= VIRTIO_REG_CONFIG + E64_VIRTIO_MAX_CONFIG)
            {
                value = rd64(dev->config + (offset - VIRTIO_REG_CONFIG));
            }
            break;
    }
    pthread_mutex_unlock(&dev->lock);
    return value;
}

static void virtio_write(void *context,
                         UInt8 bar,
                         UInt64 offset,
                         UInt64 value)
{
    E64VirtioDeviceRef dev = context;
    Boolean notify = false, was_reset = false;
    if(bar != 0)
    {
        return;
    }

    pthread_mutex_lock(&dev->lock);
    virtio_queue_t *q = &dev->queues[dev->queue_sel];
    switch(offset)
    {
        case VIRTIO_REG_DRIVER_FEATURES:
            dev->driver_features = value;
            break;
        case VIRTIO_REG_QUEUE_SEL:
            dev->queue_sel = value < E64_VIRTIO_MAX_QUEUES ? (UInt8)value : 0;
            break;
        case VIRTIO_REG_QUEUE_NUM:
            if(value != 0 && value <= dev->queue_max && (value & (value - 1)) == 0)
            {
                q->num = (UInt16)value;
            }
            break;
        case VIRTIO_REG_QUEUE_READY:
            q->ready = (value & 1) != 0;
            if(!q->ready)
            {
                q->last = 0;
            }
            break;
        case VIRTIO_REG_QUEUE_NOTIFY:
            notify = value < dev->descriptor.queueCount;
            break;
        case VIRTIO_REG_INT_ACK:
            dev->interrupts &= ~value;
            break;
        case VIRTIO_REG_STATUS:
            if(value == 0)
            {
                virtio_reset(dev);
                was_reset = true;
            }
            else
            {
                dev->status = value & 0xFF;
            }
            break;
        case VIRTIO_REG_QUEUE_DESC:
            q->desc = value;
            break;
        case VIRTIO_REG_QUEUE_DRIVER:
            q->driver = value;
            break;
        case VIRTIO_REG_QUEUE_DEVICE:
            q->device = value;
            break;
        default:
            break;
    }
    pthread_mutex_unlock(&dev->lock);

    if(was_reset && dev->descriptor.reset != NULL)
    {
        dev->descriptor.reset(dev->descriptor.context, dev);
    }
    if(notify && dev->descriptor.notify != NULL)
    {
        dev->descriptor.notify(dev->descriptor.context, dev, (UInt8)value);
    }
}

static void virtio_free(void *context)
{
    E64VirtioDeviceRef dev = context;
    if(dev->descriptor.free != NULL)
    {
        dev->descriptor.free(dev->descriptor.context);
    }
    pthread_mutex_destroy(&dev->lock);
    free(dev);
}

E64VirtioDeviceRef E64MachineAttachVirtioDevice(E64MachineRef machineRef,
                                                const E64VirtioDeviceDescriptor *descriptor)
{
    if(machineRef == NULL || descriptor == NULL ||
       descriptor->queueCount == 0 || descriptor->queueCount > E64_VIRTIO_MAX_QUEUES)
    {
        return NULL;
    }

    E64VirtioDeviceRef dev = calloc(1, sizeof(struct E64VirtioDevice));
    if(dev == NULL)
    {
        return NULL;
    }
    dev->descriptor = *descriptor;
    dev->queue_max = descriptor->queueSize != 0 ? descriptor->queueSize : 64;
    if((dev->queue_max & (dev->queue_max - 1)) != 0)
    {
        free(dev);
        return NULL;
    }
    pthread_mutex_init(&dev->lock, NULL);

    E64PCIeDeviceDescriptor card = {
        .vendor = E64_PCIE_VENDOR_EMEXLABS,
        .device = E64_VIRTIO_PCIE_DEVICE,
        .classCode = VIRTIO_PCIE_CLASS,
        .revision = 1,
        .barSize = { VIRTIO_BAR_SIZE },
        .context = dev,
        .read = virtio_read,
        .write = virtio_write,
        .free = virtio_free,
    };
    dev->pcie = E64MachineAttachPCIeDevice(machineRef, &card);
    if(dev->pcie == NULL)
    {
        pthread_mutex_destroy(&dev->lock);
        free(dev);
        return NULL;
    }
    return dev;
}

Boolean E64VirtioDeviceIsReady(E64VirtioDeviceRef dev)
{
    if(dev == NULL)
    {
        return false;
    }
    pthread_mutex_lock(&dev->lock);
    Boolean ready = (dev->status & VIRTIO_STATUS_DRIVER_OK) != 0;
    pthread_mutex_unlock(&dev->lock);
    return ready;
}

UInt64 E64VirtioDeviceGetFeatures(E64VirtioDeviceRef dev)
{
    if(dev == NULL)
    {
        return 0;
    }
    pthread_mutex_lock(&dev->lock);
    UInt64 features = dev->driver_features;
    pthread_mutex_unlock(&dev->lock);
    return features;
}

void E64VirtioDeviceSetConfig(E64VirtioDeviceRef dev,
                              const void *bytes,
                              UInt32 length)
{
    if(dev == NULL || bytes == NULL)
    {
        return;
    }
    if(length > E64_VIRTIO_MAX_CONFIG)
    {
        length = E64_VIRTIO_MAX_CONFIG;
    }
    pthread_mutex_lock(&dev->lock);
    if(memcmp(dev->config, bytes, length) != 0)
    {
        memcpy(dev->config, bytes, length);
        dev->generation++;
        if(dev->status & VIRTIO_STATUS_DRIVER_OK)
        {
            virtio_interrupt(dev, VIRTIO_INT_CONFIG);
        }
    }
    pthread_mutex_unlock(&dev->lock);
}

SInt64 E64VirtioQueueReceive(E64VirtioDeviceRef dev,
                             UInt8 queue,
                             void *buffer,
                             UInt64 capacity)
{
    SInt64 result = -1;
    if(dev == NULL || (buffer == NULL && capacity != 0))
    {
        return -1;
    }
    pthread_mutex_lock(&dev->lock);
    virtio_queue_t *q = virtio_queue(dev, queue);
    UInt16 head = 0;
    if(q != NULL && virtio_pop(dev, q, &head))
    {
        result = (SInt64)virtio_copy(dev, q, head, false, buffer, capacity);
        virtio_push(dev, q, head, 0);
    }
    pthread_mutex_unlock(&dev->lock);
    return result;
}

UInt64 E64VirtioQueueSend(E64VirtioDeviceRef dev,
                          UInt8 queue,
                          const void *bytes,
                          UInt64 length)
{
    UInt64 sent = 0;
    if(dev == NULL || bytes == NULL || length == 0)
    {
        return 0;
    }
    pthread_mutex_lock(&dev->lock);
    virtio_queue_t *q = virtio_queue(dev, queue);
    UInt16 head = 0;
    if(q != NULL && virtio_pop(dev, q, &head))
    {
        sent = virtio_copy(dev, q, head, true, (UInt8 *)bytes, length);
        virtio_push(dev, q, head, (UInt32)sent);
    }
    pthread_mutex_unlock(&dev->lock);
    return sent;
}
