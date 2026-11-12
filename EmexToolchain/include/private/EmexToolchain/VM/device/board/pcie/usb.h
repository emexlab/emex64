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

#ifndef EMEX64VM_DEVICE_USB_H
#define EMEX64VM_DEVICE_USB_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/board/pcie/pcie.h>

#define USB_PCI_DEVICE_ID           0x0002
#define USB_PCI_CLASS               0x0C03E6
#define USB_BAR0_SIZE               EMEX64_PAGE_SIZE

#define USB_MAX_PORTS               8
#define USB_TD_SLOTS                32
#define USB_TD_MAX_LENGTH           (16ULL * 1024 * 1024)

/* global registers */
#define USB_REG_CAP                 0x00
#define USB_REG_CMD                 0x08
#define USB_REG_STS                 0x10
#define USB_REG_TDLIST              0x18
#define USB_REG_DOORBELL            0x20
#define USB_REG_ERROR               0x28

#define USB_CMD_RUN                 (1ULL << 0)
#define USB_CMD_RESET               (1ULL << 1)
#define USB_CMD_IE                  (1ULL << 2)

#define USB_STS_TD_DONE             (1ULL << 0)
#define USB_STS_TD_ERROR            (1ULL << 1)
#define USB_STS_PORT_CHANGE         (1ULL << 2)

/* root port registers */
#define USB_PORT_BASE(p)            (0x100 + (p) * 0x20)
#define USB_PORTSC                  0x00
#define USB_PORTCTL                 0x08

#define USB_PORTSC_CONNECTED        (1ULL << 0)
#define USB_PORTSC_ENABLED          (1ULL << 1)
#define USB_PORTSC_CHANGE           (1ULL << 2)
#define USB_PORTSC_SPEED(s)         (((s) >> 8) & 0xF)

#define USB_PORTCTL_RESET           (1ULL << 0)
#define USB_PORTCTL_DISABLE         (1ULL << 1)
#define USB_PORTCTL_CLEAR_CHANGE    (1ULL << 2)

#define USB_SPEED_LOW               1
#define USB_SPEED_FULL              2
#define USB_SPEED_HIGH              3
#define USB_SPEED_SUPER             4

/* transfer descriptor */
#define USB_TD_SIZE                 64
#define USB_TD_TYPE                 0x00
#define USB_TD_TARGET               0x08
#define USB_TD_SETUP                0x10
#define USB_TD_BUFFER               0x18
#define USB_TD_LENGTH               0x20
#define USB_TD_STATUS               0x28
#define USB_TD_ACTUAL               0x30

#define USB_TD_TYPE_CONTROL         1
#define USB_TD_TYPE_BULK_IN         2
#define USB_TD_TYPE_BULK_OUT        3

#define USB_TD_STATUS_DONE          (1ULL << 0)
#define USB_TD_STATUS_ERROR         (1ULL << 1)
#define USB_TD_STATUS_CODE(s)       (((s) >> 8) & 0xFF)

/* completion codes */
#define USB_OK                      0
#define USB_ERR_STALL               1
#define USB_ERR_NO_RESPONSE         2
#define USB_ERR_BAD_TD              3
#define USB_ERR_DMA                 4
#define USB_ERR_BABBLE              5
#define USB_ERR_NAK                 6

#define USB_DIR_IN                  0x80
#define USB_TYPE_MASK               0x60
#define USB_TYPE_STANDARD           0x00
#define USB_TYPE_CLASS              0x20
#define USB_TYPE_VENDOR             0x40
#define USB_RECIP_MASK              0x1F
#define USB_RECIP_DEVICE            0x00
#define USB_RECIP_INTERFACE         0x01
#define USB_RECIP_ENDPOINT          0x02

#define USB_REQ_GET_STATUS          0x00
#define USB_REQ_CLEAR_FEATURE       0x01
#define USB_REQ_SET_FEATURE         0x03
#define USB_REQ_SET_ADDRESS         0x05
#define USB_REQ_GET_DESCRIPTOR      0x06
#define USB_REQ_GET_CONFIGURATION   0x08
#define USB_REQ_SET_CONFIGURATION   0x09
#define USB_REQ_GET_INTERFACE       0x0A
#define USB_REQ_SET_INTERFACE       0x0B

#define USB_FEATURE_ENDPOINT_HALT   0

#define USB_DT_DEVICE               1
#define USB_DT_CONFIG               2
#define USB_DT_STRING               3
#define USB_DT_INTERFACE            4
#define USB_DT_ENDPOINT             5

#define USB_EP_BULK                 2

typedef struct emex64_usb_device emex64_usb_device_t;

/* god I wanna kms */
typedef struct {
    UInt8 (*control)(emex64_usb_device_t *dev, const UInt8 setup[8], UInt8 *data, UInt32 len, UInt32 *actual);
    UInt8 (*bulk_in)(emex64_usb_device_t *dev, UInt8 ep, UInt8 *data, UInt64 len, UInt64 *actual);
    UInt8 (*bulk_out)(emex64_usb_device_t *dev, UInt8 ep, const UInt8 *data, UInt64 len, UInt64 *actual);
    void (*configured)(emex64_usb_device_t *dev, UInt8 configuration);
    Boolean (*clear_halt)(emex64_usb_device_t *dev, UInt8 ep_address);
    void (*free)(emex64_usb_device_t *dev);
} emex64_usb_device_ops_t;

struct emex64_usb_device {
    const emex64_usb_device_ops_t *ops;
    void *context;
    UInt8 speed;
    const UInt8 *device_descriptor;
    const UInt8 *config_descriptor;
    UInt16 config_length;
    const char *strings[8];
    UInt8 address;
    UInt8 configuration;
    UInt16 ep_in;
    UInt16 ep_out;
    UInt16 halted_in;
    UInt16 halted_out;
};

emex64_usb_device_t *emex64_usb_msc_alloc(const char *path, Boolean readonly, UInt8 index);

/* finally */

typedef struct {
    emex64_usb_device_t *device;
    Boolean enabled;
    Boolean change;
} emex64_usb_port_t;

typedef struct {
    emex64_pcie_function_t function;
    emex64_usb_port_t ports[USB_MAX_PORTS];
    UInt64 cmd, sts, tdlist, error;
} emex64_usb_t;

emex64_usb_t *emex64_usb_alloc(emex64_pcie_t *pcie, UInt8 dev);
Boolean emex64_usb_attach(emex64_usb_t *usb, UInt8 port, emex64_usb_device_t *device);  /* please hand me a sharp knife */
void emex64_usb_dealloc(emex64_usb_t *usb);
void emex64_usb_reset(emex64_usb_t *usb);

#endif /* EMEX64VM_DEVICE_USB_H */
