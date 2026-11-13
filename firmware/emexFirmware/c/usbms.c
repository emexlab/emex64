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
#include <usb.h>
#include <usbms.h>

#define CBW_SIGNATURE       0x43425355u     /* "USBC" */
#define CSW_SIGNATURE       0x53425355u     /* "USBS" */
#define MAX_BLOCKS_PER_CMD  2048            /* 1 MiB per command */

static struct usbms_disk disks[USBMS_MAX_DISKS];
static int disk_count;

static u8 cbw[31];
static u8 csw[13];
static u8 data_buf[64];

static void put_le32(u8 *p, u32 v) { p[0] = (u8)v; p[1] = (u8)(v >> 8); p[2] = (u8)(v >> 16); p[3] = (u8)(v >> 24); }
static u32 le32(const u8 *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((u32)p[3] << 24); }
static void put_be32(u8 *p, u32 v) { p[0] = (u8)(v >> 24); p[1] = (u8)(v >> 16); p[2] = (u8)(v >> 8); p[3] = (u8)v; }
static u32 be32(const u8 *p) { return ((u32)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }

static void reset_recovery(struct usbms_disk *d)
{
    struct usb_device *dev = d->dev;
    usb_control(dev, 0x21, 0xFF, 0, dev->if_number, 0, NULL, NULL);
    usb_clear_halt(dev, USB_DIR_IN | dev->ep_in);
    usb_clear_halt(dev, dev->ep_out);
}

static int read_csw(struct usbms_disk *d, u64 *got)
{
    int err = usb_bulk(d->dev, d->dev->ep_in, 1, csw, sizeof(csw), got);
    if(err == USB_ERR_STALL)
    {
        usb_clear_halt(d->dev, USB_DIR_IN | d->dev->ep_in);
        err = usb_bulk(d->dev, d->dev->ep_in, 1, csw, sizeof(csw), got);
    }
    return err;
}

static int bot(struct usbms_disk *d, const u8 *cdb, int cdb_len, int in, void *data, u32 length)
{
    struct usb_device *dev = d->dev;
    u64 got = 0;
    int err;

    fw_memset(cbw, 0, sizeof(cbw));
    put_le32(cbw, CBW_SIGNATURE);
    put_le32(cbw + 4, ++d->tag);
    put_le32(cbw + 8, length);
    cbw[12] = in ? 0x80 : 0x00;
    cbw[13] = 0;
    cbw[14] = (u8)cdb_len;
    fw_memcpy(cbw + 15, cdb, cdb_len);

    err = usb_bulk(dev, dev->ep_out, 0, cbw, sizeof(cbw), NULL);
    if(err != USB_OK)
    {
        reset_recovery(d);
        return err;
    }

    if(length != 0)
    {
        err = usb_bulk(dev, in ? dev->ep_in : dev->ep_out, in, data, length, NULL);
        if(err == USB_ERR_STALL)
        {
            /* the device refused the data stage */
            usb_clear_halt(dev, in ? (USB_DIR_IN | dev->ep_in) : dev->ep_out);
        }
        else if(err != USB_OK)
        {
            reset_recovery(d);
            return err;
        }
    }

    err = read_csw(d, &got);
    if(err != USB_OK)
    {
        reset_recovery(d);
        return err;
    }
    if(got != sizeof(csw) || le32(csw) != CSW_SIGNATURE || le32(csw + 4) != d->tag)
    {
        reset_recovery(d);
        return USBMS_ERR_CSW;
    }

    switch(csw[12])
    {
        case 0:
            return 0;
        case 1:
            return USBMS_ERR_FAILED;
        default:
            reset_recovery(d);
            return USBMS_ERR_PHASE;
    }
}

static void request_sense(struct usbms_disk *d)
{
    u8 cdb[6] = { 0x03, 0, 0, 0, 18, 0 };
    fw_memset(data_buf, 0, 18);
    if(bot(d, cdb, 6, 1, data_buf, 18) == 0)
    {
        d->sense_key = data_buf[2] & 0x0F;
        d->asc = data_buf[12];
        d->ascq = data_buf[13];
    }
}

static int command(struct usbms_disk *d, const u8 *cdb, int cdb_len, int in, void *data, u32 length)
{
    int err = bot(d, cdb, cdb_len, in, data, length);
    if(err == USBMS_ERR_FAILED)
    {
        request_sense(d);
    }
    return err;
}

static int transfer(struct usbms_disk *d, int write, u64 lba, u64 count, u64 buffer)
{
    while(count)
    {
        u64 n = count < MAX_BLOCKS_PER_CMD ? count : MAX_BLOCKS_PER_CMD;
        u8 cdb[16];
        int len;

        fw_memset(cdb, 0, sizeof(cdb));
        if(lba + n <= 0xFFFFFFFFUL)
        {
            cdb[0] = write ? 0x2A : 0x28;       /* WRITE(10) / READ(10) */
            put_be32(cdb + 2, (u32)lba);
            cdb[7] = (u8)(n >> 8);
            cdb[8] = (u8)n;
            len = 10;
        }
        else
        {
            cdb[0] = write ? 0x8A : 0x88;       /* WRITE(16) / READ(16) */
            put_be32(cdb + 2, (u32)(lba >> 32));
            put_be32(cdb + 6, (u32)lba);
            put_be32(cdb + 10, (u32)n);
            len = 16;
        }

        int err = command(d, cdb, len, !write, (void *)buffer, (u32)(n * d->block_size));
        if(err)
        {
            return err;
        }
        lba += n;
        count -= n;
        buffer += n * d->block_size;
    }
    return 0;
}

static int blk_read(struct blkdev *b, u64 lba, u64 count, void *buffer)
{
    return transfer((struct usbms_disk *)b, 0, lba, count, (u64)buffer);
}

static int blk_write(struct blkdev *b, u64 lba, u64 count, const void *buffer)
{
    return transfer((struct usbms_disk *)b, 1, lba, count, (u64)buffer);
}

static const char *sense_name(struct usbms_disk *d)
{
    switch(d->sense_key)
    {
        case 0x2: return "not ready";
        case 0x3: return "medium error";
        case 0x4: return "hardware error";
        case 0x5: return d->asc == 0x21 ? "LBA out of range" : "illegal request";
        case 0x6: return "unit attention";
        case 0x7: return "write protected";
    }
    return "check condition";
}

static const char *blk_strerror(struct blkdev *b, int err)
{
    switch(err)
    {
        case 0: return "ok";
        case USBMS_ERR_FAILED: return sense_name((struct usbms_disk *)b);
        case USBMS_ERR_PHASE: return "phase error";
        case USBMS_ERR_CSW: return "bad status wrapper";
    }
    return usb_strerror(err);
}

static void copy_trimmed(char *dst, const u8 *src, int len)
{
    fw_memcpy(dst, src, len);
    dst[len] = 0;
    while(len > 0 && (dst[len - 1] == ' ' || dst[len - 1] == 0))
    {
        dst[--len] = 0;
    }
}

int usbms_probe(struct usb_device *dev)
{
    if(disk_count >= USBMS_MAX_DISKS)
    {
        return -1;
    }

    struct usbms_disk *d = &disks[disk_count];
    fw_memset(d, 0, sizeof(*d));
    d->dev = dev;
    d->tag = 0xE6400000u;

    u64 got = 0;
    u8 max_lun = 0;
    if(usb_control(dev, 0xA1, 0xFE, 0, dev->if_number, 1, data_buf, &got) == USB_OK && got == 1)
    {
        max_lun = data_buf[0];
    }

    u8 inquiry[6] = { 0x12, 0, 0, 0, 36, 0 };
    int err = command(d, inquiry, 6, 1, data_buf, 36);
    if(err)
    {
        fw_printf("[usbms] port %d: INQUIRY failed (%s)\n", dev->port, blk_strerror(&d->blk, err));
        return -1;
    }
    if((data_buf[0] & 0x1F) != 0x00)
    {
        fw_printf("[usbms] port %d: not a block device (type %u)\n", dev->port, data_buf[0] & 0x1F);
        return -1;
    }
    copy_trimmed(d->vendor, data_buf + 8, 8);
    copy_trimmed(d->product, data_buf + 16, 16);

    u8 tur[6] = { 0x00, 0, 0, 0, 0, 0 };
    for(int tries = 0; (err = command(d, tur, 6, 0, NULL, 0)) != 0 && tries < 5; tries++)
    {
    }
    if(err)
    {
        fw_printf("[usbms] port %d: unit not ready (%s)\n", dev->port, blk_strerror(&d->blk, err));
        return -1;
    }

    u8 cap10[10] = { 0x25 };
    err = command(d, cap10, 10, 1, data_buf, 8);
    if(err)
    {
        fw_printf("[usbms] port %d: READ CAPACITY failed (%s)\n", dev->port, blk_strerror(&d->blk, err));
        return -1;
    }
    d->blocks = (u64)be32(data_buf) + 1;
    d->block_size = be32(data_buf + 4);
    if(be32(data_buf) == 0xFFFFFFFFu)
    {
        u8 cap16[16] = { 0x9E, 0x10 };
        cap16[13] = 32;
        err = command(d, cap16, 16, 1, data_buf, 32);
        if(err)
        {
            fw_printf("[usbms] port %d: READ CAPACITY(16) failed (%s)\n", dev->port, blk_strerror(&d->blk, err));
            return -1;
        }
        d->blocks = (((u64)be32(data_buf) << 32) | be32(data_buf + 4)) + 1;
        d->block_size = be32(data_buf + 8);
    }

    u8 mode_sense[6] = { 0x1A, 0, 0x3F, 0, 4, 0 };
    int readonly = command(d, mode_sense, 6, 1, data_buf, 4) == 0 && (data_buf[2] & 0x80);

    char name[8] = { 'u', 's', 'b', (char)('0' + disk_count), 0 };
    fw_printf("[usbms] %s (port %d): %s %s, ", name, dev->port, d->vendor, d->product);
    fw_put_size(d->blocks * d->block_size);
    fw_printf("%s%s\n", max_lun ? ", LUN 0 only" : "", readonly ? ", write protected" : "");

    if(d->block_size != BLK_SECTOR_SIZE)
    {
        fw_printf("[usbms] %s: %u byte blocks are not supported\n", name, d->block_size);
        return -1;
    }

    fw_memcpy(d->blk.name, name, sizeof(name));
    d->blk.kind = BLK_KIND_USB;
    d->blk.ctrl = dev->hc->pci;
    d->blk.port = dev->port;
    d->blk.sectors = d->blocks;
    d->blk.readonly = readonly;
    d->blk.read = blk_read;
    d->blk.write = blk_write;
    d->blk.strerror = blk_strerror;
    blk_register(&d->blk);
    disk_count++;
    return 0;
}
