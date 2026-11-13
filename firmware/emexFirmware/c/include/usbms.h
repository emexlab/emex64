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
 * usbms.h - USB mass storage: Bulk-Only Transport with SCSI commands.
 * Every working drive is registered as block device usb<n>.
 */

#ifndef FW_USBMS_H
#define FW_USBMS_H

#include <fw.h>
#include <usb.h>
#include <blk.h>

#define USBMS_MAX_DISKS     8

/* errors returned through the block device, on top of the USB_ERR_* codes */
#define USBMS_ERR_FAILED    0x10    /* CSW: command failed (see sense) */
#define USBMS_ERR_PHASE     0x11    /* CSW: phase error */
#define USBMS_ERR_CSW       0x12    /* missing or malformed CSW */

struct usbms_disk {
    struct blkdev blk;          /* first member */
    struct usb_device *dev;
    u32 tag;
    u32 block_size;
    u64 blocks;
    u8 sense_key, asc, ascq;    /* of the last failed command */
    char vendor[9];
    char product[17];
};

/* binds a bulk-only SCSI interface, returns 0 when a disk was registered */
int usbms_probe(struct usb_device *dev);

#endif /* FW_USBMS_H */
