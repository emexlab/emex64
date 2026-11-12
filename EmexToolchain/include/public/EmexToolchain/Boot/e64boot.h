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

#ifndef E64BOOT_H
#define E64BOOT_H

#include <stdint.h>

#define E64BOOT_SECTOR_SIZE     512
#define E64BOOT_MAGIC           "EMEX64BT"
#define E64BOOT_VERSION         1

struct e64_boot_header {
    char magic[8];
    uint32_t version;
    uint32_t header_size;
    uint64_t load_address;      /* must be page aligned */
    uint64_t entry;             /* absolute address inside the image */
    uint64_t image_size;
    uint64_t memory_size;
    uint32_t image_crc32;
    uint32_t header_crc32;
    char name[32];
    uint64_t text_size;
    uint8_t reserved[E64BOOT_SECTOR_SIZE - 96];
};

#define E64BOOT_INFO_MAGIC      0x4F464E4934364545ULL
#define E64BOOT_INFO_VERSION    5
#define E64BOOT_INFO_CMDLINE_SIZE 296

#define E64BOOT_DEVICE_SATA     0
#define E64BOOT_DEVICE_USB      1
#define E64BOOT_DEVICE_KIND(d)  ((uint32_t)((d) >> 32))
#define E64BOOT_DEVICE_BUS(d)   ((uint8_t)((d) >> 16))
#define E64BOOT_DEVICE_DEV(d)   ((uint8_t)((d) >> 8))
#define E64BOOT_DEVICE_FN(d)    ((uint8_t)(d))

struct e64_boot_info {
    /* version 1 */
    uint64_t magic;
    uint64_t version;
    uint64_t memory_size;       /* installed RAM in bytes */
    uint64_t firmware_end;      /* first byte after the firmware image + bss */
    uint64_t image_start;       /* load_address */
    uint64_t image_end;         /* load_address + memory_size of the header */
    uint64_t boot_device;
    uint64_t boot_port;
    uint64_t partition_lba;
    uint64_t partition_sectors;
    char name[32];              /* copied from the boot header */

    /* version 2 */
    uint64_t screen_y;
    uint64_t fb_base;
    uint64_t fb_width;
    uint64_t fb_height;
    uint64_t fb_stride;
    uint64_t fb_format;
    uint64_t gpu_regs;
    uint64_t cores;

    /* version 5: from a UEFI boot loader (GRUB), 0 from emexFirmware */
    uint64_t efi_system_table;
    uint64_t efi_mmap;
    uint64_t efi_mmap_size;
    uint64_t efi_mmap_desc_size;
    uint64_t efi_mmap_desc_version;
    char cmdline[E64BOOT_INFO_CMDLINE_SIZE];    /* added to the kernel's own */
};

#define GPT_SIGNATURE           "EFI PART"
#define GPT_REVISION            0x00010000u
#define GPT_HEADER_SIZE         92u
#define GPT_ENTRY_SIZE          128u
#define GPT_ENTRY_COUNT         128u
#define GPT_ENTRIES_SECTORS     ((GPT_ENTRY_COUNT * GPT_ENTRY_SIZE) / E64BOOT_SECTOR_SIZE)

struct gpt_header {
    char signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t disk_guid[16];
    uint64_t entries_lba;
    uint32_t entry_count;
    uint32_t entry_size;
    uint32_t entries_crc32;
};

struct gpt_entry {
    uint8_t type_guid[16];
    uint8_t unique_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attributes;
    uint16_t name[36];          /* UTF-16LE */
};

#define E64BOOT_PART_TYPE_GUID_STR "E6400B00-7E64-4C64-8E64-B0075EC70264"
static const uint8_t e64boot_part_type_guid[16] = {
    0x00, 0x0B, 0x40, 0xE6, 0x64, 0x7E, 0x64, 0x4C,
    0x8E, 0x64, 0xB0, 0x07, 0x5E, 0xC7, 0x02, 0x64
};

static inline uint32_t e64boot_crc32_update(uint32_t crc,
                                            const void *data,
                                            uint64_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    crc = ~crc;
    while(len--)
    {
        crc ^= *p++;
        for(int k = 0; k < 8; k++)
        {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

static inline uint32_t e64boot_crc32(const void *data,
                                     uint64_t len)
{
    return e64boot_crc32_update(0, data, len);
}

#endif /* E64BOOT_H */
