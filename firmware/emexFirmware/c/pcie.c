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

static struct pcie_dev devices[PCIE_MAX_FOUND];
static int device_count;

static const char *class_name(u32 class_code)
{
    switch(class_code >> 8)
    {
        case 0x0106:
            return "SATA controller";
        case 0x0108: /* eta wen */
            return "NVMe controller";
        case 0x0C03:
            return "USB controller";
        case 0x0200: /* eta wen */
            return "Ethernet controller";
        case 0x0300:
            return "display controller";
        case 0x0600: /* eta wen */
            return "host bridge";
        case 0x0604: /* eta wen */
            return "PCIe bridge";
    }
    switch(class_code >> 16)
    {
        case 0x01:
            return "mass storage controller";
        case 0x02: /* eta wen */
            return "network controller";
        case 0x03:
            return "display controller";
        case 0x0C: /* eta wen */
            return "serial bus controller";
    }
    return "device";
}

static void probe(u64 ecam,
                  u8 dev,
                  u8 fn,
                  u64 *next,
                  u64 end)
{
    u64 cfg = ecam + ((u64)dev << 16) + ((u64)fn << 13);
    u64 id = mmio_read64(cfg + PCIE_CFG_ID);
    struct pcie_dev *d;

    if((id & 0xFFFF) == 0xFFFF || device_count >= PCIE_MAX_FOUND)
    {
        return;
    }

    d = &devices[device_count++];
    d->bus = 0;
    d->dev = dev;
    d->fn = fn;
    d->cfg = cfg;
    d->vendor = (u16)id;
    d->device = (u16)(id >> 16);
    d->class_code = (u32)((id >> 32) & 0xFFFFFF);
    d->revision = (u8)(id >> 56);
    d->irq = mmio_read64(cfg + PCIE_CFG_IRQ);

    /*
     * some devices got already setup by the firmware's
     * assembly code.
     */
    u64 control = mmio_read64(cfg + PCIE_CFG_CONTROL);
    if(control & PCIE_CONTROL_MEM)
    {
        fw_printf("[pcie] 00:%02x.%u %04x:%04x %s (class %06x)", dev, fn, d->vendor, d->device, class_name(d->class_code), d->class_code);
        if(d->irq != ~0UL)
        {
            fw_printf(", irq %lu", d->irq);
        }
        fw_printf(", set up already\n");
        for(int b = 0; b < PCIE_MAX_BARS; b++)
        {
            d->bar_size[b] = mmio_read64(cfg + PCIE_CFG_BAR_SIZE(b));
            d->bar[b] = d->bar_size[b] ? mmio_read64(cfg + PCIE_CFG_BAR(b)) : 0;
            if(d->bar_size[b])
            {
                fw_printf("[pcie]        BAR%d: 0x%lx (", b, d->bar[b]);
                fw_put_size(d->bar_size[b]);
                fw_printf(")\n");
            }
        }
        return;
    }

    mmio_write64(cfg + PCIE_CFG_CONTROL, 0);
    fw_printf("[pcie] 00:%02x.%u %04x:%04x %s (class %06x)", dev, fn, d->vendor, d->device, class_name(d->class_code), d->class_code);
    if(d->irq != ~0UL)
    {
        fw_printf(", irq %lu", d->irq);
    }
    fw_printf("\n");

    for(int b = 0; b < PCIE_MAX_BARS; b++)
    {
        u64 size = mmio_read64(cfg + PCIE_CFG_BAR_SIZE(b));
        d->bar_size[b] = size;
        d->bar[b] = 0;
        if(size == 0)
        {
            continue;
        }

        /* naturally aligned allocation from the aperture */
        u64 base = (*next + size - 1) & ~(size - 1);
        if(base + size > end)
        {
            fw_printf("[pcie]        BAR%d: no space left in the aperture\n", b);
            d->bar_size[b] = 0;
            continue;
        }
        *next = base + size;
        mmio_write64(cfg + PCIE_CFG_BAR(b), base);
        d->bar[b] = mmio_read64(cfg + PCIE_CFG_BAR(b));
        fw_printf("[pcie]        BAR%d: 0x%lx (", b, d->bar[b]);
        fw_put_size(size);
        fw_printf(")\n");
    }
}

int pcie_init(void)
{
    u64 id = mmio_read64(PCIE_RC_BASE + PCIE_RC_REG_ID);
    if((id & 0xFFFFFFFF) != PCIE_RC_ID_MAGIC)
    {
        fw_printf("[pcie] no root complex\n");
        return -1;
    }

    u64 ecam = mmio_read64(PCIE_RC_BASE + PCIE_RC_REG_ECAM_BASE);
    u64 next = mmio_read64(PCIE_RC_BASE + PCIE_RC_REG_MMIO_BASE);
    u64 end = next + mmio_read64(PCIE_RC_BASE + PCIE_RC_REG_MMIO_SIZE);
    u64 aperture_end = end;

    /* the display controller's BAR is at the top already */
    if(display_bar != 0 && display_bar > next && display_bar < end)
    {
        end = display_bar;
    }

    fw_printf("[pcie] root complex v%lu, config space 0x%lx, aperture 0x%lx-0x%lx\n", id >> 32, ecam, next, aperture_end - 1);

    device_count = 0;
    for(u8 dev = 0; dev < 32; dev++)
    {
        u64 id0 = mmio_read64(ecam + ((u64)dev << 16) + PCIE_CFG_ID);
        if((id0 & 0xFFFF) == 0xFFFF)
        {
            continue;
        }
        probe(ecam, dev, 0, &next, end);

        /* multi function device? */
        if(mmio_read64(ecam + ((u64)dev << 16) + PCIE_CFG_HEADER) & (1UL << 8))
        {
            for(u8 fn = 1; fn < 8; fn++)
            {
                probe(ecam, dev, fn, &next, end);
            }
        }
    }

    fw_printf("[pcie] %d function(s) found\n", device_count);
    return device_count;
}

int pcie_count(void)
{
    return device_count;
}

struct pcie_dev *pcie_get(int index)
{
    return index >= 0 && index < device_count ? &devices[index] : NULL;
}

struct pcie_dev *pcie_find_class(u32 class_code, int nth)
{
    for(int i = 0; i < device_count; i++)
    {
        if(devices[i].class_code == class_code && nth-- == 0)
        {
            return &devices[i];
        }
    }
    return NULL;
}

void pcie_enable(struct pcie_dev *d, u64 control)
{
    mmio_write64(d->cfg + PCIE_CFG_CONTROL, control);
}
