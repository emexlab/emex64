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
#include <blk.h>
#include <sata.h>
#include <usb.h>
#include <EmexToolchain/Boot/e64boot.h>

#define SECTOR              E64BOOT_SECTOR_SIZE
#define MAX_ENTRY_BYTES     (64 * 1024)

#define GUI_LINE_HEIGHT     16
#define GUI_MARGIN          2

static u8 sector_buf[SECTOR];
static u8 entry_buf[MAX_ENTRY_BYTES];

static struct e64_boot_info boot_info;
extern unsigned short gui_line, gui_column;

static int read_gpt_header(struct blkdev *d,
                           u64 lba,
                           struct gpt_header *h)
{
    int err = d->read(d, lba, 1, sector_buf);
    if(err)
    {
        fw_printf("[boot] %s: reading LBA %lu failed (%s)\n", d->name, lba, d->strerror(d, err));
        return -1;
    }

    fw_memcpy(h, sector_buf, sizeof(*h));
    if((fw_memcmp(h->signature, GPT_SIGNATURE, 8) != 0) &&
       (h->header_size < GPT_HEADER_SIZE || h->header_size > SECTOR))
    {
        return -1;
    }

    u32 want = h->header_crc32;
    ((struct gpt_header *)sector_buf)->header_crc32 = 0;
    if(fw_crc32_update(0, sector_buf, h->header_size) != want)
    {
        fw_printf("[boot] %s: GPT header at LBA %lu has a bad checksum\n", d->name, lba);
        return -1;
    }

    if(h->entry_size < sizeof(struct gpt_entry) ||
       h->entry_count == 0 ||
       (u64)h->entry_count * h->entry_size > MAX_ENTRY_BYTES)
    {
        fw_printf("[boot] %s: unsupported GPT entry layout\n", d->name);
        return -1;
    }

    return 0;
}

static int read_gpt(struct blkdev *d,
                    struct gpt_header *h)
{
    if(read_gpt_header(d, 1, h) != 0)
    {
        if(d->sectors < 2 || read_gpt_header(d, d->sectors - 1, h) != 0)
        {
            return -1;
        }
        fw_printf("[boot] %s: using the backup GPT\n", d->name);
    }

    u64 bytes = (u64)h->entry_count * h->entry_size;
    u64 sectors = (bytes + SECTOR - 1) / SECTOR;
    int err = d->read(d, h->entries_lba, sectors, entry_buf);
    if(err)
    {
        fw_printf("[boot] %s: reading the partition table failed (%s)\n", d->name, d->strerror(d, err));
        return -1;
    }

    if(fw_crc32_update(0, entry_buf, bytes) != h->entries_crc32)
    {
        fw_printf("[boot] %s: partition table has a bad checksum\n", d->name);
        return -1;
    }

    return 0;
}

static int check_header(struct blkdev *d,
                        struct e64_boot_header *bh,
                        struct gpt_entry *e)
{
    u64 part_bytes = (e->last_lba - e->first_lba) * SECTOR;
    u64 mem = fw_ram_end();
    u64 fw_end = ((u64)__bss_end + EMEX64_PAGE_SIZE - 1) & ~(EMEX64_PAGE_SIZE - 1);
    u32 want = bh->header_crc32;

    bh->header_crc32 = 0;
    if(fw_memcmp(bh->magic, E64BOOT_MAGIC, 8) != 0)
    {
        fw_printf("[boot] %s: boot partition has no emex64 boot header\n", d->name);
        return -1;
    }
    if(fw_crc32_update(0, bh, sizeof(*bh)) != want)
    {
        fw_printf("[boot] %s: boot header checksum mismatch\n", d->name);
        return -1;
    }
    bh->header_crc32 = want;

    if(bh->version != E64BOOT_VERSION || bh->header_size != SECTOR)
    {
        fw_printf("[boot] %s: unsupported boot header version %u\n", d->name, bh->version);
        return -1;
    }
    if(bh->image_size == 0 || bh->image_size > part_bytes || bh->memory_size < bh->image_size)
    {
        fw_printf("[boot] %s: boot image size is invalid\n", d->name);
        return -1;
    }
    if(bh->load_address & (EMEX64_PAGE_SIZE - 1))
    {
        fw_printf("[boot] %s: load address 0x%lx is not page aligned\n", d->name, bh->load_address);
        return -1;
    }

    u64 footprint = (bh->image_size + SECTOR - 1) & ~(u64)(SECTOR - 1);
    if(footprint < bh->memory_size)
    {
        footprint = bh->memory_size;
    }
    if(bh->load_address < fw_end || bh->load_address > mem || footprint > mem - bh->load_address)
    {
        fw_printf("[boot] %s: image 0x%lx-0x%lx does not fit (firmware ends at 0x%lx, memory 0x%lx)\n",
                  d->name, bh->load_address, bh->load_address + bh->memory_size, fw_end, mem);
        return -1;
    }
    if(bh->entry < bh->load_address || bh->entry - bh->load_address >= bh->image_size)
    {
        fw_printf("[boot] %s: entry point 0x%lx is outside the image\n", d->name, bh->entry);
        return -1;
    }
    if(bh->text_size != 0 &&
       ((bh->text_size & (EMEX64_PAGE_SIZE - 1)) || bh->text_size > bh->memory_size ||
        bh->entry - bh->load_address >= bh->text_size))
    {
        fw_printf("[boot] %s: text size 0x%lx is invalid\n", d->name, bh->text_size);
        return -1;
    }

    return 0;
}

static void try_disk(struct blkdev *d)
{
    struct gpt_header h;
    struct e64_boot_header bh;

    if(read_gpt(d, &h) != 0)
    {
        fw_printf("[boot] %s: no valid GPT\n", d->name);
        return;
    }

    for(u32 i = 0; i < h.entry_count; i++)
    {
        struct gpt_entry *e = (struct gpt_entry *)(entry_buf + (u64)i * h.entry_size);
        if(fw_memcmp(e->type_guid, e64boot_part_type_guid, 16) != 0)
        {
            continue;
        }
        if(e->last_lba < e->first_lba || e->last_lba >= d->sectors)
        {
            fw_printf("[boot] %s: boot partition #%u lies outside the disk\n", d->name, i);
            continue;
        }

        fw_printf("[boot] %s: emex64 boot partition #%u at LBA %lu\n", d->name, i, e->first_lba);
        if(d->read(d, e->first_lba, 1, sector_buf) != 0)
        {
            continue;
        }
        fw_memcpy(&bh, sector_buf, sizeof(bh));
        bh.name[sizeof(bh.name) - 1] = 0;
        if(check_header(d, &bh, e) != 0)
        {
            continue;
        }

        fw_printf("[boot] loading '%s': ", bh.name);
        fw_put_size(bh.image_size);
        fw_printf(" to 0x%lx\n", bh.load_address);

        u64 sectors = (bh.image_size + SECTOR - 1) / SECTOR;
        int err = d->read(d, e->first_lba + 1, sectors, (void *)bh.load_address);
        if(err)
        {
            fw_printf("[boot] %s: reading the image failed (%s)\n", d->name, d->strerror(d, err));
            continue;
        }

        u64 loaded = sectors * SECTOR > bh.memory_size ? sectors * SECTOR : bh.memory_size;
        fw_memset((void *)(bh.load_address + bh.image_size), 0, loaded - bh.image_size);

        if(fw_crc32_update(0, (void *)bh.load_address, bh.image_size) != bh.image_crc32)
        {
            fw_printf("[boot] %s: image checksum mismatch\n", d->name);
            continue;
        }

        fw_memset(&boot_info, 0, sizeof(boot_info));
        boot_info.magic = E64BOOT_INFO_MAGIC;
        boot_info.version = E64BOOT_INFO_VERSION;
        boot_info.memory_size = mmio_read64(EMEX64_MC_REG_SIZE);
        boot_info.cores = (mmio_read64(EMEX64_APIC_REG_VERSION) >> 8) & 0xFF;
        boot_info.firmware_end = (u64)__bss_end;
        boot_info.image_start = bh.load_address;
        boot_info.image_end = bh.load_address + bh.memory_size;
        boot_info.boot_device = ((u64)d->kind << 32) | ((u64)d->ctrl->bus << 16) | ((u64)d->ctrl->dev << 8) | d->ctrl->fn;
        boot_info.boot_port = (u64)d->port;
        boot_info.partition_lba = e->first_lba;
        boot_info.partition_sectors = e->last_lba - e->first_lba + 1;
        fw_memcpy(boot_info.name, bh.name, sizeof(boot_info.name));

        /*
         * we have to make KTRR cover the kernel text region
         * kernel can then lock KTRR afterwards.
         */
        if(bh.text_size != 0)
        {
            if(mmio_read64(EMEX64_MC_REG_KTRR_LOCKED))
            {
                fw_printf("[boot] warning: KTRR is already locked, the image text stays unprotected\n");
            }
            else
            {
                mmio_write64(EMEX64_MC_REG_KTRR_SIZE, 0);
                mmio_write64(EMEX64_MC_REG_KTRR_BASE, bh.load_address);
                mmio_write64(EMEX64_MC_REG_KTRR_SIZE, bh.text_size);
                fw_printf("[boot] KTRR covers the image text 0x%lx-0x%lx\n", bh.load_address, bh.load_address + bh.text_size);
            }
        }

        boot_info.screen_y = 0;
        if(gui_fb)
        {
            boot_info.screen_y = (u64)(gui_line + (gui_column != 0)) * GUI_LINE_HEIGHT + GUI_MARGIN;
            boot_info.fb_base = gui_fb;
            boot_info.fb_width = gui_width;
            boot_info.fb_height = gui_height;
            boot_info.fb_stride = gui_width;
            boot_info.fb_format = 1;
            boot_info.gpu_regs = display_bar;
        }

        /* jumping to da kernel =3 */
        fw_printf("[boot] jumping to 0x%lx\n\n", bh.entry);
        fw_jump(bh.entry, &boot_info);
    }

    fw_printf("[boot] %s: nothing bootable\n", d->name);
}

/* called from main (main.c) :3c */
void fw_boot_main(void)
{
    /* scanning storage devices */
    if(pcie_init() <= 0)
    {
        return;
    }
    sata_init();
    usb_init();

    /* hopefully there are block devices to boot from */
    if(blk_count() == 0)
    {
        fw_printf("[boot] no disks\n");
        return;
    }

    /* trying to boot from each block device */
    for(int i = 0; i < blk_count(); i++)
    {
        try_disk(blk_get(i));
    }

    /* ouweeeeee */
    fw_printf("[boot] no bootable disk found\n");
}
