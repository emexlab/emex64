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
#include <sata.h>

#define SLOTS_PER_PORT  32
#define POLL_LIMIT      100000000UL

static struct sata_disk disks[SATA_MAX_DISKS];
static int disk_count;

static u8 cmd_lists[SATA_MAX_DISKS * SLOTS_PER_PORT * sizeof(struct sata_slot) + 64];
static u8 identify_buf[SATA_SECTOR_SIZE];

static u64 port_reg(struct sata_disk *d,
                    u64 reg)
{
    return SATA_PORT_BASE(d->port) + reg;
}

static const char *trim(char *s,
                        int len)
{
    s[len] = 0;
    while(len > 0 && (s[len - 1] == ' ' || s[len - 1] == 0))
    {
        s[--len] = 0;
    }
    return s;
}

const char *sata_strerror(int err)
{
    switch(err)
    {
        case 0:
            return "ok";
        case 1:
            return "bad command";
        case 2:
            return "out of range";
        case 3:
            return "DMA fault";
        case 4:
            return "I/O error";
        case 5:
            return "read only";
        case 6:
            return "no device";
        case -1:
            return "timeout";
    }
    return "unknown error";
}

static int issue(struct sata_disk *d,
                 u64 opcode,
                 u64 lba,
                 u64 count,
                 u64 buffer)
{
    struct sata_slot *s = &d->slots[0];
    u64 n;

    s->opcode = opcode;
    s->lba = lba;
    s->count = count;
    s->buffer = buffer;
    s->status = 0;
    s->bytes = 0;

    pcie_bar_write(d->ctrl, 0, port_reg(d, SATA_PxIS), ~0UL);
    pcie_bar_write(d->ctrl, 0, port_reg(d, SATA_PxCI), 1);

    for(n = 0; n < POLL_LIMIT; n++)
    {
        if(!(pcie_bar_read(d->ctrl, 0, port_reg(d, SATA_PxCI)) & 1))
        {
            break;
        }
    }
    if(n == POLL_LIMIT || !(s->status & SATA_STATUS_DONE))
    {
        return -1;
    }
    if(s->status & SATA_STATUS_ERROR)
    {
        return (int)((s->status >> 8) & 0xFF);
    }
    return 0;
}

static int transfer(struct sata_disk *d, u64 opcode, u64 lba, u64 count, u64 buffer)
{
    while(count)
    {
        u64 n = count < d->max_sectors ? count : d->max_sectors;
        int err = issue(d, opcode, lba, n, buffer);
        if(err)
        {
            return err;
        }
        lba += n;
        count -= n;
        buffer += n * SATA_SECTOR_SIZE;
    }
    return 0;
}

int sata_read(struct sata_disk *d, u64 lba, u64 count, void *buffer)
{
    return transfer(d, SATA_OP_READ_DMA_EXT, lba, count, (u64)buffer);
}

int sata_write(struct sata_disk *d, u64 lba, u64 count, const void *buffer)
{
    return transfer(d, SATA_OP_WRITE_DMA_EXT, lba, count, (u64)buffer);
}

/* block device interface (blk.h), blk is the first member of sata_disk */
static int blk_read(struct blkdev *b, u64 lba, u64 count, void *buffer)
{
    return sata_read((struct sata_disk *)b, lba, count, buffer);
}

static int blk_write(struct blkdev *b, u64 lba, u64 count, const void *buffer)
{
    return sata_write((struct sata_disk *)b, lba, count, buffer);
}

static const char *blk_strerror(struct blkdev *b, int err)
{
    return sata_strerror(err);
}

static void init_controller(struct pcie_dev *ctrl)
{
    u8 *lists = (u8 *)(((u64)cmd_lists + 63) & ~63UL);

    if(ctrl->bar_size[0] == 0)
    {
        return;
    }

    /* pollinggg~~ */
    pcie_enable(ctrl, PCIE_CONTROL_MEM | PCIE_CONTROL_MASTER);
    pcie_bar_write(ctrl, 0, SATA_REG_GHC, SATA_GHC_RESET);

    u64 cap = pcie_bar_read(ctrl, 0, SATA_REG_CAP);
    u64 pi = pcie_bar_read(ctrl, 0, SATA_REG_PI);
    int ports = (int)(cap & 0xFF);

    fw_printf("[sata] controller at 00:%02x.%u, %d ports, %lu slots, v%lu\n", ctrl->dev, ctrl->fn,
              ports, (cap >> 8) & 0xFF, cap >> 32);

    for(int p = 0; p < ports && p < 64; p++)
    {
        if(!(pi & (1UL << p)))
        {
            continue;
        }

        u64 ssts = pcie_bar_read(ctrl, 0, SATA_PORT_BASE(p) + SATA_PxSSTS);
        if(!(ssts & SATA_SSTS_PRESENT) || disk_count >= SATA_MAX_DISKS)
        {
            continue;
        }

        struct sata_disk *d = &disks[disk_count];
        d->ctrl = ctrl;
        d->index = disk_count;
        d->port = p;
        d->readonly = (ssts & SATA_SSTS_READONLY) != 0;
        d->slots = (struct sata_slot *)(lists + (u64)disk_count * SLOTS_PER_PORT * sizeof(struct sata_slot));
        d->max_sectors = 1;

        pcie_bar_write(ctrl, 0, port_reg(d, SATA_PxCLB), (u64)d->slots);
        pcie_bar_write(ctrl, 0, port_reg(d, SATA_PxCMD), SATA_CMD_START);

        int err = issue(d, SATA_OP_IDENTIFY, 0, 1, (u64)identify_buf);
        if(err)
        {
            fw_printf("[sata] port %d: IDENTIFY failed (%s)\n", p, sata_strerror(err));
            continue;
        }

        fw_memcpy(d->model, identify_buf + 0x00, 40);
        fw_memcpy(d->serial, identify_buf + 0x28, 20);
        trim(d->model, 40);
        trim(d->serial, 20);
        d->sectors = *(u64 *)(identify_buf + 0x40);
        d->max_sectors = *(u64 *)(identify_buf + 0x58);
        if(d->max_sectors == 0 || d->max_sectors > 65536)
        {
            d->max_sectors = 65536;
        }

        fw_printf("[sata] sata%d (port %d): %s, serial %s, ", disk_count, p, d->model, d->serial);
        fw_put_size(d->sectors * SATA_SECTOR_SIZE);
        fw_printf("%s\n", d->readonly ? ", read only" : "");

        d->blk.name[0] = 's'; d->blk.name[1] = 'a'; d->blk.name[2] = 't'; d->blk.name[3] = 'a';
        d->blk.name[4] = (char)('0' + disk_count);
        d->blk.name[5] = 0;
        d->blk.kind = BLK_KIND_SATA;
        d->blk.ctrl = ctrl;
        d->blk.port = p;
        d->blk.sectors = d->sectors;
        d->blk.readonly = d->readonly;
        d->blk.read = blk_read;
        d->blk.write = blk_write;
        d->blk.strerror = blk_strerror;
        blk_register(&d->blk);
        disk_count++;
    }
}

int sata_init(void)
{
    struct pcie_dev *ctrl;

    disk_count = 0;
    for(int i = 0; (ctrl = pcie_find_class(SATA_PCI_CLASS, i)) != NULL; i++)
    {
        init_controller(ctrl);
    }
    if(disk_count == 0)
    {
        fw_printf("[sata] no disks attached\n");
    }
    return disk_count;
}

int sata_disk_count(void)
{
    return disk_count;
}

struct sata_disk *sata_disk(int index)
{
    return index >= 0 && index < disk_count ? &disks[index] : NULL;
}
