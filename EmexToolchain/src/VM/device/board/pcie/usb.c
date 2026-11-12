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
#include <string.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/board/pcie/usb.h>

static void usb_wr64(UInt8 *p,
                     UInt64 v)
{
    for(int i = 0; i < 8; i++)
    {
        p[i] = (UInt8)(v >> (8 * i));
    }
}

static UInt64 usb_rd64(const UInt8 *p)
{
    UInt64 v = 0;
    for(int i = 0; i < 8; i++)
    {
        v |= (UInt64)p[i] << (8 * i);
    }
    return v;
}

static void usb_device_set_configuration(emex64_usb_device_t *dev,
                                         UInt8 configuration)
{
    dev->configuration = configuration;
    dev->ep_in = 0;
    dev->ep_out = 0;
    dev->halted_in = 0;
    dev->halted_out = 0;

    if(configuration != 0)
    {
        UInt16 off = 0;
        while(off + 2 <= dev->config_length)
        {
            const UInt8 *d = dev->config_descriptor + off;
            if(d[0] < 2 || off + d[0] > dev->config_length)
            {
                break;
            }
            if(d[1] == USB_DT_ENDPOINT && d[0] >= 7 && (d[3] & 3) == USB_EP_BULK)
            {
                UInt8 num = d[2] & 0x0F;
                if(d[2] & USB_DIR_IN)
                {
                    dev->ep_in |= (UInt16)(1u << num);
                }
                else
                {
                    dev->ep_out |= (UInt16)(1u << num);
                }
            }
            off += d[0];
        }
    }

    if(dev->ops->configured != NULL)
    {
        dev->ops->configured(dev, configuration);
    }
}

static void usb_device_reset(emex64_usb_device_t *dev)
{
    dev->address = 0;
    usb_device_set_configuration(dev, 0);
}

static Boolean usb_endpoint_valid(emex64_usb_device_t *dev,
                                  UInt8 ep_address)
{
    UInt8 num = ep_address & 0x0F;
    if(num == 0)
    {
        return true;
    }
    if(dev->configuration == 0)
    {
        return false;
    }
    UInt16 mask = (ep_address & USB_DIR_IN) ? dev->ep_in : dev->ep_out;
    return (mask >> num) & 1;
}

static UInt16 *usb_halt_mask(emex64_usb_device_t *dev,
                             UInt8 ep_address)
{
    return (ep_address & USB_DIR_IN) ? &dev->halted_in : &dev->halted_out;
}

static UInt8 usb_get_descriptor(emex64_usb_device_t *dev,
                                UInt16 value,
                                UInt16 index,
                                UInt8 *data,
                                UInt32 len,
                                UInt32 *actual)
{
    UInt8 type = value >> 8;
    UInt8 n = value & 0xFF;
    UInt8 tmp[256];
    const UInt8 *src = NULL;
    UInt32 size = 0;

    switch(type)
    {
        case USB_DT_DEVICE:
            src = dev->device_descriptor;
            size = 18;
            break;
        case USB_DT_CONFIG:
            if(n != 0)
            {
                return USB_ERR_STALL;
            }
            src = dev->config_descriptor;
            size = dev->config_length;
            break;
        case USB_DT_STRING:
            if(n == 0)
            {
                tmp[0] = 4;
                tmp[1] = USB_DT_STRING;
                tmp[2] = 0x09;
                tmp[3] = 0x04;
                size = 4;
            }
            else
            {
                if(n >= 8 || dev->strings[n] == NULL)
                {
                    return USB_ERR_STALL;
                }
                const char *s = dev->strings[n];
                UInt32 chars = (UInt32)strlen(s);
                if(chars > 126)
                {
                    chars = 126;
                }
                tmp[0] = (UInt8)(2 + chars * 2);
                tmp[1] = USB_DT_STRING;
                for(UInt32 i = 0; i < chars; i++)
                {
                    tmp[2 + i * 2] = (UInt8)s[i];
                    tmp[3 + i * 2] = 0;
                }
                size = tmp[0];
            }
            src = tmp;
            break;
        default:
            return USB_ERR_STALL;
    }

    *actual = size < len ? size : len;
    memcpy(data, src, *actual);
    return USB_OK;
}

static UInt8 usb_control(emex64_usb_device_t *dev,
                         const UInt8 setup[8],
                         UInt8 *data,
                         UInt32 len,
                         UInt32 *actual,
                         SInt16 *new_address)
{
    UInt8 type = setup[0];
    UInt8 request = setup[1];
    UInt16 value = setup[2] | (setup[3] << 8);
    UInt16 index = setup[4] | (setup[5] << 8);

    *actual = 0;
    if((type & USB_TYPE_MASK) != USB_TYPE_STANDARD)
    {
        if(dev->ops->control == NULL)
        {
            return USB_ERR_STALL;
        }
        return dev->ops->control(dev, setup, data, len, actual);
    }

    UInt8 recip = type & USB_RECIP_MASK;
    switch(request)
    {
        case USB_REQ_GET_DESCRIPTOR:
            if(!(type & USB_DIR_IN))
            {
                return USB_ERR_STALL;
            }
            return usb_get_descriptor(dev, value, index, data, len, actual);
        case USB_REQ_SET_ADDRESS:
            if(recip != USB_RECIP_DEVICE || value > 127 || len != 0 || dev->configuration != 0)
            {
                return USB_ERR_STALL;
            }
            *new_address = (SInt16)value;
            return USB_OK;
        case USB_REQ_GET_CONFIGURATION:
            if(len < 1)
            {
                return USB_ERR_STALL;
            }
            data[0] = dev->configuration;
            *actual = 1;
            return USB_OK;
        case USB_REQ_SET_CONFIGURATION:
            if(dev->address == 0 || (value != 0 && value != dev->config_descriptor[5]))
            {
                return USB_ERR_STALL;
            }
            usb_device_set_configuration(dev, (UInt8)value);
            return USB_OK;
        case USB_REQ_GET_INTERFACE:
            if(dev->configuration == 0 || len < 1)
            {
                return USB_ERR_STALL;
            }
            data[0] = 0;
            *actual = 1;
            return USB_OK;
        case USB_REQ_SET_INTERFACE:
            return (dev->configuration != 0 && value == 0) ? USB_OK : USB_ERR_STALL;
        case USB_REQ_GET_STATUS:
        {
            if(len < 2)
            {
                return USB_ERR_STALL;
            }
            UInt16 status = 0;
            if(recip == USB_RECIP_DEVICE)
            {
                status = (dev->config_descriptor[7] & 0x40) ? 1 : 0;
            }
            else if(recip == USB_RECIP_ENDPOINT)
            {
                if(!usb_endpoint_valid(dev, (UInt8)index))
                {
                    return USB_ERR_STALL;
                }
                status = (*usb_halt_mask(dev, (UInt8)index) >> (index & 0x0F)) & 1;
            }
            else if(recip != USB_RECIP_INTERFACE)
            {
                return USB_ERR_STALL;
            }
            data[0] = (UInt8)status;
            data[1] = 0;
            *actual = 2;
            return USB_OK;
        }
        case USB_REQ_CLEAR_FEATURE:
        case USB_REQ_SET_FEATURE:
        {
            if(recip != USB_RECIP_ENDPOINT || value != USB_FEATURE_ENDPOINT_HALT || !usb_endpoint_valid(dev, (UInt8)index))
            {
                return USB_ERR_STALL;
            }
            UInt8 num = index & 0x0F;
            if(num == 0)
            {
                return USB_OK;
            }
            UInt16 *mask = usb_halt_mask(dev, (UInt8)index);
            if(request == USB_REQ_SET_FEATURE)
            {
                *mask |= (UInt16)(1u << num);
            }
            else if(dev->ops->clear_halt == NULL || dev->ops->clear_halt(dev, (UInt8)index))
            {
                *mask &= (UInt16)~(1u << num);
            }
            return USB_OK;
        }
        default:
            return USB_ERR_STALL;
    }
}

static emex64_usb_device_t *usb_find_device(emex64_usb_t *usb,
                                            UInt8 address)
{
    for(UInt8 p = 0; p < USB_MAX_PORTS; p++)
    {
        emex64_usb_port_t *port = &usb->ports[p];
        if(port->device != NULL && port->enabled && port->device->address == address)
        {
            return port->device;
        }
    }
    return NULL;
}

static void usb_update_irq(emex64_usb_t *usb)
{
    if((usb->cmd & USB_CMD_IE) && usb->sts)
    {
        emex64_pcie_raise_irq(&usb->function);
    }
}

static UInt8 usb_execute(emex64_usb_t *usb,
                         UInt8 *td,
                         UInt64 *actual)
{
    UInt64 type = usb_rd64(td + USB_TD_TYPE);
    UInt64 target = usb_rd64(td + USB_TD_TARGET);
    UInt64 buffer = usb_rd64(td + USB_TD_BUFFER);
    UInt64 length = usb_rd64(td + USB_TD_LENGTH);
    UInt8 address = target & 0x7F;
    UInt8 ep = (target >> 8) & 0x0F;

    *actual = 0;
    if(length > USB_TD_MAX_LENGTH)
    {
        return USB_ERR_BAD_TD;
    }

    emex64_usb_device_t *dev = usb_find_device(usb, address);

    switch(type)
    {
        case USB_TD_TYPE_CONTROL:
        {
            UInt8 setup[8];
            memcpy(setup, td + USB_TD_SETUP, 8);
            UInt32 wLength = setup[6] | (setup[7] << 8);
            if(ep != 0 || wLength > length)
            {
                return USB_ERR_BAD_TD;
            }
            if(dev == NULL)
            {
                return USB_ERR_NO_RESPONSE;
            }

            UInt8 *data = NULL;
            if(wLength != 0)
            {
                data = emex64_pcie_dma_map(&usb->function, buffer, wLength, (setup[0] & USB_DIR_IN) != 0);
                if(data == NULL)
                {
                    return USB_ERR_DMA;
                }
            }

            UInt32 got = 0;
            SInt16 new_address = -1;
            UInt8 err = usb_control(dev, setup, data, wLength, &got, &new_address);
            *actual = got;
            if(err == USB_OK && new_address >= 0)
            {
                dev->address = (UInt8)new_address;
            }
            return err;
        }
        case USB_TD_TYPE_BULK_IN:
        case USB_TD_TYPE_BULK_OUT:
        {
            Boolean in = type == USB_TD_TYPE_BULK_IN;
            UInt8 ep_address = ep | (in ? USB_DIR_IN : 0);
            if(ep == 0)
            {
                return USB_ERR_BAD_TD;
            }
            if(dev == NULL || !usb_endpoint_valid(dev, ep_address))
            {
                return USB_ERR_NO_RESPONSE;
            }
            UInt16 *halted = usb_halt_mask(dev, ep_address);
            if((*halted >> ep) & 1)
            {
                return USB_ERR_STALL;
            }

            UInt8 *data = NULL;
            if(length != 0)
            {
                data = emex64_pcie_dma_map(&usb->function, buffer, length, in);
                if(data == NULL)
                {
                    return USB_ERR_DMA;
                }
            }

            UInt8 err = in ? dev->ops->bulk_in(dev, ep, data, length, actual) : dev->ops->bulk_out(dev, ep, data, length, actual);
            if(err == USB_ERR_STALL)
            {
                *halted |= (UInt16)(1u << ep);
            }
            return err;
        }
        default:
            return USB_ERR_BAD_TD;
    }
}

static void usb_process(emex64_usb_t *usb,
                        UInt64 slots)
{
    if(!(usb->cmd & USB_CMD_RUN))
    {
        return;
    }

    slots &= (1ULL << USB_TD_SLOTS) - 1;
    while(slots != 0)
    {
        UInt8 s = (UInt8)__builtin_ctzll(slots);
        slots &= ~(1ULL << s);

        UInt8 *td = emex64_pcie_dma_map(&usb->function, usb->tdlist + (UInt64)s * USB_TD_SIZE, USB_TD_SIZE, true);
        if(td == NULL)
        {
            usb->error = USB_ERR_DMA;
            usb->sts |= USB_STS_TD_ERROR;
            continue;
        }

        UInt64 actual = 0;
        UInt8 err = usb_execute(usb, td, &actual);
        usb_wr64(td + USB_TD_ACTUAL, actual);
        usb_wr64(td + USB_TD_STATUS, USB_TD_STATUS_DONE | (err ? USB_TD_STATUS_ERROR : 0) | ((UInt64)err << 8));
        if(err)
        {
            usb->error = err;
            usb->sts |= USB_STS_TD_ERROR;
        }
        usb->sts |= USB_STS_TD_DONE;
    }

    usb_update_irq(usb);
}

static void usb_reset(emex64_usb_t *usb)
{
    usb->cmd = 0;
    usb->sts = 0;
    usb->tdlist = 0;
    usb->error = 0;
    for(UInt8 p = 0; p < USB_MAX_PORTS; p++)
    {
        emex64_usb_port_t *port = &usb->ports[p];
        port->enabled = false;
        port->change = port->device != NULL;
        if(port->device != NULL)
        {
            usb_device_reset(port->device);
        }
    }
    for(UInt8 p = 0; p < USB_MAX_PORTS; p++)
    {
        if(usb->ports[p].change)
        {
            usb->sts |= USB_STS_PORT_CHANGE;
        }
    }
}

static void usb_port_control(emex64_usb_t *usb,
                             emex64_usb_port_t *port,
                             UInt64 value)
{
    if(value & USB_PORTCTL_CLEAR_CHANGE)
    {
        port->change = false;
    }
    if(value & USB_PORTCTL_DISABLE)
    {
        port->enabled = false;
    }
    if((value & USB_PORTCTL_RESET) && port->device != NULL)
    {
        usb_device_reset(port->device);
        port->enabled = true;
    }
}

static UInt64 usb_bar_read(emex64_pcie_function_t *function,
                           UInt8 bar,
                           UInt64 offset)
{
    emex64_usb_t *usb = function->context;

    switch(offset)
    {
        case USB_REG_CAP:
            return USB_MAX_PORTS | (USB_TD_SLOTS << 8) | (1ULL << 32);
        case USB_REG_CMD:
            return usb->cmd;
        case USB_REG_STS:
            return usb->sts;
        case USB_REG_TDLIST:
            return usb->tdlist;
        case USB_REG_DOORBELL:
            return 0;
        case USB_REG_ERROR:
            return usb->error;
        default:
            break;
    }

    if(offset < USB_PORT_BASE(0) || offset >= USB_PORT_BASE(USB_MAX_PORTS))
    {
        return 0;
    }

    emex64_usb_port_t *port = &usb->ports[(offset - USB_PORT_BASE(0)) / 0x20];
    if((offset & 0x1F) == USB_PORTSC)
    {
        UInt64 sc = 0;
        if(port->device != NULL)
        {
            sc |= USB_PORTSC_CONNECTED | ((UInt64)port->device->speed << 8);
        }
        if(port->enabled)
        {
            sc |= USB_PORTSC_ENABLED;
        }
        if(port->change)
        {
            sc |= USB_PORTSC_CHANGE;
        }
        return sc;
    }
    return 0;
}

static void usb_bar_write(emex64_pcie_function_t *function,
                          UInt8 bar,
                          UInt64 offset,
                          UInt64 value)
{
    emex64_usb_t *usb = function->context;

    switch(offset)
    {
        case USB_REG_CMD:
            if(value & USB_CMD_RESET)
            {
                usb_reset(usb);
                return;
            }
            usb->cmd = value & (USB_CMD_RUN | USB_CMD_IE);
            usb_update_irq(usb);
            return;
        case USB_REG_STS:
            usb->sts &= ~value;
            return;
        case USB_REG_TDLIST:
            usb->tdlist = value & ~(UInt64)(USB_TD_SIZE - 1);
            return;
        case USB_REG_DOORBELL:
            usb_process(usb, value);
            return;
        case USB_REG_ERROR:
            usb->error = 0;
            return;
        default:
            break;
    }

    if(offset < USB_PORT_BASE(0) || offset >= USB_PORT_BASE(USB_MAX_PORTS))
    {
        return;
    }

    emex64_usb_port_t *port = &usb->ports[(offset - USB_PORT_BASE(0)) / 0x20];
    if((offset & 0x1F) == USB_PORTCTL)
    {
        usb_port_control(usb, port, value);
    }
}

static const emex64_pcie_ops_t usb_ops = {
    .bar_read = usb_bar_read,
    .bar_write = usb_bar_write,
};

emex64_usb_t *emex64_usb_alloc(emex64_pcie_t *pcie,
                               UInt8 dev)
{
    emex64_usb_t *usb = calloc(1, sizeof(emex64_usb_t));
    if(usb == NULL)
    {
        return NULL;
    }

    usb->function.vendor = PCIE_VENDOR_EMEXLABS;
    usb->function.device = USB_PCI_DEVICE_ID;
    usb->function.class_code = USB_PCI_CLASS;
    usb->function.revision = 1;
    usb->function.bar_size[0] = USB_BAR0_SIZE;
    usb->function.ops = &usb_ops;
    usb->function.context = usb;

    if(!emex64_pcie_attach(pcie, &usb->function, dev, 0))
    {
        free(usb);
        return NULL;
    }

    usb_reset(usb);
    return usb;
}

Boolean emex64_usb_attach(emex64_usb_t *usb,
                          UInt8 portIndex,
                          emex64_usb_device_t *device)
{
    if(usb == NULL || device == NULL || portIndex >= USB_MAX_PORTS || usb->ports[portIndex].device != NULL)
    {
        return false;
    }

    emex64_usb_port_t *port = &usb->ports[portIndex];
    port->device = device;
    port->enabled = false;
    port->change = true;
    usb_device_reset(device);
    usb->sts |= USB_STS_PORT_CHANGE;
    usb_update_irq(usb);
    return true;
}

void emex64_usb_reset(emex64_usb_t *usb)
{
    if(usb != NULL)
    {
        usb_reset(usb);
    }
}

void emex64_usb_dealloc(emex64_usb_t *usb)
{
    if(usb == NULL)
    {
        return;
    }

    for(UInt8 p = 0; p < USB_MAX_PORTS; p++)
    {
        emex64_usb_device_t *dev = usb->ports[p].device;
        if(dev != NULL && dev->ops->free != NULL)
        {
            dev->ops->free(dev);
        }
    }
    free(usb);
}
