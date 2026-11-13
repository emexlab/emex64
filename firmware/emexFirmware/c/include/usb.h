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

/*
 * usb.h - driver for the emex64 USB host controller (docs/devices/usb.md):
 * resets the root ports, enumerates the devices behind them (address,
 * descriptors, configuration) and hands mass storage devices to usbms.c
 */

#ifndef FW_USB_H
#define FW_USB_H

#include <fw.h>
#include <pcie.h>

#define USB_PCI_CLASS               0x0C03E6

#define USB_REG_CAP                 0x00
#define USB_REG_CMD                 0x08
#define USB_REG_STS                 0x10
#define USB_REG_TDLIST              0x18
#define USB_REG_DOORBELL            0x20
#define USB_REG_ERROR               0x28
#define USB_CMD_RUN                 (1UL << 0)
#define USB_CMD_RESET               (1UL << 1)

#define USB_PORT_BASE(p)            (0x100 + (p) * 0x20)
#define USB_PORTSC                  0x00
#define USB_PORTCTL                 0x08
#define USB_PORTSC_CONNECTED        (1UL << 0)
#define USB_PORTSC_ENABLED          (1UL << 1)
#define USB_PORTCTL_RESET           (1UL << 0)
#define USB_PORTCTL_DISABLE         (1UL << 1)
#define USB_PORTCTL_CLEAR_CHANGE    (1UL << 2)

#define USB_TD_CONTROL              1
#define USB_TD_BULK_IN              2
#define USB_TD_BULK_OUT             3
#define USB_TD_STATUS_DONE          (1UL << 0)
#define USB_TD_STATUS_ERROR         (1UL << 1)

/* completion codes */
#define USB_OK                      0
#define USB_ERR_STALL               1
#define USB_ERR_NO_RESPONSE         2
#define USB_ERR_BAD_TD              3
#define USB_ERR_DMA                 4
#define USB_ERR_BABBLE              5
#define USB_ERR_NAK                 6
#define USB_ERR_TIMEOUT             7       /* driver side: TD never completed */

/* standard requests and descriptors */
#define USB_DIR_IN                  0x80
#define USB_REQ_CLEAR_FEATURE       0x01
#define USB_REQ_SET_ADDRESS         0x05
#define USB_REQ_GET_DESCRIPTOR      0x06
#define USB_REQ_SET_CONFIGURATION   0x09
#define USB_DT_DEVICE               1
#define USB_DT_CONFIG               2
#define USB_DT_STRING               3
#define USB_DT_INTERFACE            4
#define USB_DT_ENDPOINT             5

#define USB_MAX_DEVICES             16

/* one transfer descriptor (64 bytes) */
struct usb_td {
    u64 type;
    u64 target;                 /* address | endpoint << 8 */
    u64 setup;
    u64 buffer;
    u64 length;
    volatile u64 status;
    volatile u64 actual;
    u64 reserved;
};

struct usb_hc {
    struct pcie_dev *pci;
    struct usb_td *tds;         /* 32 TDs, 64 byte aligned; the driver uses TD 0 */
    int ports;
};

struct usb_device {
    struct usb_hc *hc;
    int port;
    u8 address;
    u8 speed;
    u16 vendor, product;
    u8 config_value;
    char name[64];              /* product string, ASCII */

    /* the interface a class driver binds to */
    u8 if_number, if_class, if_subclass, if_protocol;
    u8 ep_in, ep_out;           /* bulk endpoint numbers, 0 = none */
};

/* finds the controllers and enumerates their ports, returns the number of devices */
int usb_init(void);

/* return USB_OK or a USB_ERR_* code; *actual (may be NULL) gets the bytes moved */
int usb_control(struct usb_device *dev, u8 type, u8 request, u16 value, u16 index,
                u16 length, void *data, u64 *actual);
int usb_bulk(struct usb_device *dev, u8 ep, int in, void *data, u64 length, u64 *actual);
int usb_clear_halt(struct usb_device *dev, u8 ep_address);
const char *usb_strerror(int err);

#endif /* FW_USB_H */
