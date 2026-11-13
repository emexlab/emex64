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

#include <fw.h>
#include <pcie.h>
#include <usb.h>
#include <usbms.h>

#define MAX_CONTROLLERS     4
#define POLL_LIMIT          100000000UL

static struct usb_hc controllers[MAX_CONTROLLERS];
static int controller_count;
static struct usb_device devices[USB_MAX_DEVICES];
static int device_count;
static u8 next_address = 1;

static u8 td_lists[MAX_CONTROLLERS * 32 * sizeof(struct usb_td) + 64];
static u8 desc_buf[256];

const char *usb_strerror(int err)
{
    switch(err)
    {
        case USB_OK: return "ok";
        case USB_ERR_STALL: return "stall";
        case USB_ERR_NO_RESPONSE: return "no response";
        case USB_ERR_BAD_TD: return "bad transfer descriptor";
        case USB_ERR_DMA: return "DMA fault";
        case USB_ERR_BABBLE: return "babble";
        case USB_ERR_NAK: return "NAK";
        case USB_ERR_TIMEOUT: return "timeout";
    }
    return "unknown error";
}

static const char *speed_name(int speed)
{
    switch(speed)
    {
        case 1: return "low speed";
        case 2: return "full speed";
        case 3: return "high speed";
        case 4: return "SuperSpeed";
    }
    return "unknown speed";
}

static int run_td(struct usb_hc *hc, u64 *actual)
{
    struct usb_td *t = &hc->tds[0];
    u64 n;

    pcie_bar_write(hc->pci, 0, USB_REG_DOORBELL, 1);
    for(n = 0; n < POLL_LIMIT && !(t->status & USB_TD_STATUS_DONE); n++)
    {
    }
    if(actual)
    {
        *actual = t->actual;
    }
    if(!(t->status & USB_TD_STATUS_DONE))
    {
        return USB_ERR_TIMEOUT;
    }
    if(t->status & USB_TD_STATUS_ERROR)
    {
        return (int)((t->status >> 8) & 0xFF);
    }
    return USB_OK;
}

static int control(struct usb_hc *hc, u8 address, u8 type, u8 request, u16 value, u16 index,
                   u16 length, void *data, u64 *actual)
{
    struct usb_td *t = &hc->tds[0];
    t->type = USB_TD_CONTROL;
    t->target = address;
    t->setup = (u64)type | ((u64)request << 8) | ((u64)value << 16) | ((u64)index << 32) | ((u64)length << 48);
    t->buffer = (u64)data;
    t->length = length;
    t->status = 0;
    t->actual = 0;
    return run_td(hc, actual);
}

int usb_control(struct usb_device *dev, u8 type, u8 request, u16 value, u16 index,
                u16 length, void *data, u64 *actual)
{
    return control(dev->hc, dev->address, type, request, value, index, length, data, actual);
}

int usb_bulk(struct usb_device *dev, u8 ep, int in, void *data, u64 length, u64 *actual)
{
    struct usb_td *t = &dev->hc->tds[0];
    t->type = in ? USB_TD_BULK_IN : USB_TD_BULK_OUT;
    t->target = (u64)dev->address | ((u64)ep << 8);
    t->setup = 0;
    t->buffer = (u64)data;
    t->length = length;
    t->status = 0;
    t->actual = 0;
    return run_td(dev->hc, actual);
}

int usb_clear_halt(struct usb_device *dev, u8 ep_address)
{
    return usb_control(dev, 0x02, USB_REQ_CLEAR_FEATURE, 0, ep_address, 0, NULL, NULL);
}

static void read_string(struct usb_device *dev, u8 index, char *out, int size)
{
    u64 got = 0;
    out[0] = 0;
    if(index == 0)
    {
        return;
    }
    if(usb_control(dev, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, USB_DT_STRING << 8, 0, 255, desc_buf, &got) != USB_OK || got < 4)
    {
        return;
    }
    u16 lang = desc_buf[2] | (desc_buf[3] << 8);
    if(usb_control(dev, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) | index, lang, 255, desc_buf, &got) != USB_OK || got < 2)
    {
        return;
    }
    int chars = (int)((got < desc_buf[0] ? got : desc_buf[0]) - 2) / 2;
    int n = 0;
    for(int i = 0; i < chars && n < size - 1; i++)
    {
        u16 c = desc_buf[2 + i * 2] | (desc_buf[3 + i * 2] << 8);
        out[n++] = (c >= 0x20 && c < 0x7F) ? (char)c : '?';
    }
    out[n] = 0;
}

struct iface {
    u8 number, cls, sub, proto, in, out;
};

static void consider(struct usb_device *dev, int *best, struct iface *f)
{
    int score = (f->cls == 0x08 && f->sub == 0x06 && f->proto == 0x50 && f->in && f->out) ? 2 : 1;
    if(score > *best)
    {
        *best = score;
        dev->if_number = f->number;
        dev->if_class = f->cls;
        dev->if_subclass = f->sub;
        dev->if_protocol = f->proto;
        dev->ep_in = f->in;
        dev->ep_out = f->out;
    }
}

static void parse_config(struct usb_device *dev, u8 *cfg, u64 len)
{
    struct iface cur;
    int have = 0, best = 0;

    for(u64 off = 0; off + 2 <= len; off += cfg[off])
    {
        u8 *d = cfg + off;
        if(d[0] < 2 || off + d[0] > len)
        {
            break;
        }
        if(d[1] == USB_DT_INTERFACE && d[0] >= 9)
        {
            if(have)
            {
                consider(dev, &best, &cur);
            }
            have = 1;
            cur.number = d[2];
            cur.cls = d[5];
            cur.sub = d[6];
            cur.proto = d[7];
            cur.in = cur.out = 0;
        }
        else if(d[1] == USB_DT_ENDPOINT && d[0] >= 7 && have && (d[3] & 3) == 2)
        {
            if(d[2] & USB_DIR_IN)
            {
                cur.in = d[2] & 0x0F;
            }
            else
            {
                cur.out = d[2] & 0x0F;
            }
        }
    }
    if(have)
    {
        consider(dev, &best, &cur);
    }
}

static void enumerate_port(struct usb_hc *hc, int p)
{
    u64 portsc_reg = USB_PORT_BASE(p) + USB_PORTSC;
    u64 got = 0;
    int err;

    if(!(pcie_bar_read(hc->pci, 0, portsc_reg) & USB_PORTSC_CONNECTED))
    {
        return;
    }
    if(device_count >= USB_MAX_DEVICES || next_address > 127)
    {
        fw_printf("[usb] port %d: too many devices\n", p);
        return;
    }

    pcie_bar_write(hc->pci, 0, USB_PORT_BASE(p) + USB_PORTCTL, USB_PORTCTL_RESET | USB_PORTCTL_CLEAR_CHANGE);
    u64 portsc = pcie_bar_read(hc->pci, 0, portsc_reg);
    if(!(portsc & USB_PORTSC_ENABLED))
    {
        fw_printf("[usb] port %d: reset failed\n", p);
        return;
    }

    struct usb_device *dev = &devices[device_count];
    fw_memset(dev, 0, sizeof(*dev));
    dev->hc = hc;
    dev->port = p;
    dev->speed = (u8)((portsc >> 8) & 0xF);

    err = usb_control(dev, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, USB_DT_DEVICE << 8, 0, 8, desc_buf, &got);
    if(err == USB_OK && got < 8)
    {
        err = USB_ERR_BABBLE;
    }
    if(err == USB_OK)
    {
        err = usb_control(dev, 0x00, USB_REQ_SET_ADDRESS, next_address, 0, 0, NULL, NULL);
    }
    if(err != USB_OK)
    {
        fw_printf("[usb] port %d: device does not respond (%s)\n", p, usb_strerror(err));
        pcie_bar_write(hc->pci, 0, USB_PORT_BASE(p) + USB_PORTCTL, USB_PORTCTL_DISABLE);
        return;
    }
    dev->address = next_address++;

    err = usb_control(dev, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, USB_DT_DEVICE << 8, 0, 18, desc_buf, &got);
    if(err != USB_OK || got < 18 || desc_buf[1] != USB_DT_DEVICE)
    {
        fw_printf("[usb] port %d: bad device descriptor (%s)\n", p, usb_strerror(err));
        return;
    }
    dev->vendor = desc_buf[8] | (desc_buf[9] << 8);
    dev->product = desc_buf[10] | (desc_buf[11] << 8);
    u8 i_product = desc_buf[15];
    u8 class_code = desc_buf[4];

    err = usb_control(dev, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, USB_DT_CONFIG << 8, 0, 9, desc_buf, &got);
    if(err != USB_OK || got < 9)
    {
        fw_printf("[usb] port %d: no configuration descriptor (%s)\n", p, usb_strerror(err));
        return;
    }
    u16 total = desc_buf[2] | (desc_buf[3] << 8);
    if(total > sizeof(desc_buf))
    {
        total = sizeof(desc_buf);
    }
    dev->config_value = desc_buf[5];
    err = usb_control(dev, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, USB_DT_CONFIG << 8, 0, total, desc_buf, &got);
    if(err != USB_OK)
    {
        fw_printf("[usb] port %d: reading the configuration failed (%s)\n", p, usb_strerror(err));
        return;
    }
    parse_config(dev, desc_buf, got);

    read_string(dev, i_product, dev->name, sizeof(dev->name));
    if(dev->name[0] == 0)
    {
        fw_memcpy(dev->name, "unnamed device", 15);
    }

    err = usb_control(dev, 0x00, USB_REQ_SET_CONFIGURATION, dev->config_value, 0, 0, NULL, NULL);
    if(err != USB_OK)
    {
        fw_printf("[usb] port %d: SET_CONFIGURATION failed (%s)\n", p, usb_strerror(err));
        return;
    }

    fw_printf("[usb] port %d: %04x:%04x '%s', %s, address %u, class %02x/%02x/%02x\n",
              p, dev->vendor, dev->product, dev->name, speed_name(dev->speed), dev->address,
              class_code ? class_code : dev->if_class, dev->if_subclass, dev->if_protocol);
    device_count++;

    if(dev->if_class == 0x08)
    {
        if(dev->if_subclass == 0x06 && dev->if_protocol == 0x50 && dev->ep_in && dev->ep_out)
        {
            usbms_probe(dev);
        }
        else
        {
            fw_printf("[usb] port %d: mass storage, but not SCSI over bulk-only: ignored\n", p);
        }
    }
}

static void init_controller(struct pcie_dev *pci)
{
    if(pci->bar_size[0] == 0 || controller_count >= MAX_CONTROLLERS)
    {
        return;
    }

    struct usb_hc *hc = &controllers[controller_count];
    u8 *lists = (u8 *)(((u64)td_lists + 63) & ~63UL);
    hc->pci = pci;
    hc->tds = (struct usb_td *)(lists + (u64)controller_count * 32 * sizeof(struct usb_td));
    controller_count++;

    pcie_enable(pci, PCIE_CONTROL_MEM | PCIE_CONTROL_MASTER);
    pcie_bar_write(pci, 0, USB_REG_CMD, USB_CMD_RESET);
    pcie_bar_write(pci, 0, USB_REG_TDLIST, (u64)hc->tds);
    pcie_bar_write(pci, 0, USB_REG_CMD, USB_CMD_RUN);

    u64 cap = pcie_bar_read(pci, 0, USB_REG_CAP);
    hc->ports = (int)(cap & 0xFF);
    fw_printf("[usb] controller at 00:%02x.%u, %d ports, v%lu\n", pci->dev, pci->fn, hc->ports, cap >> 32);

    for(int p = 0; p < hc->ports; p++)
    {
        enumerate_port(hc, p);
    }
}

int usb_init(void)
{
    struct pcie_dev *pci;

    for(int i = 0; (pci = pcie_find_class(USB_PCI_CLASS, i)) != NULL; i++)
    {
        init_controller(pci);
    }
    if(controller_count != 0 && device_count == 0)
    {
        fw_printf("[usb] no devices attached\n");
    }
    return device_count;
}
