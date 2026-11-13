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
 * blk.h - block devices: what the storage drivers (sata.c, usbms.c)
 * register and the boot code (boot.c) walks. Sectors are 512 bytes.
 */

#ifndef FW_BLK_H
#define FW_BLK_H

#include <fw.h>
#include <pcie.h>

#define BLK_MAX_DEVICES     16
#define BLK_SECTOR_SIZE     512

/* same values as E64BOOT_DEVICE_* in <EmexToolchain/Boot/e64boot.h> */
#define BLK_KIND_SATA       0
#define BLK_KIND_USB        1

struct blkdev {
    char name[8];               /* "sata0", "usb1", ... */
    int kind;                   /* BLK_KIND_* */
    struct pcie_dev *ctrl;      /* host controller */
    int port;                   /* SATA port / USB root port */
    u64 sectors;
    int readonly;

    /* return 0 or a driver error code for strerror() */
    int (*read)(struct blkdev *b, u64 lba, u64 count, void *buffer);
    int (*write)(struct blkdev *b, u64 lba, u64 count, const void *buffer);
    const char *(*strerror)(struct blkdev *b, int err);
};

void blk_register(struct blkdev *b);
int blk_count(void);
struct blkdev *blk_get(int index);

#endif /* FW_BLK_H */
