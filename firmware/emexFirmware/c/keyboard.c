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
#include <interrupt.h>

#define EMEX64_8042_REG_DATA    (EMEX64_8042_BASE + 0x00)
#define EMEX64_8042_REG_STATUS  (EMEX64_8042_BASE + 0x08)

#define EMEX64_8042_STATUS_OBF  0x01
#define EMEX64_8042_STATUS_AUX  0x20

void kbd_handler(struct fw_frame *frame)
{
    (void)frame;

    u64 status = mmio_read64(EMEX64_8042_REG_STATUS);
    if(!(status & EMEX64_8042_STATUS_OBF) || (status & EMEX64_8042_STATUS_AUX))
    {
        return;
    }

    fw_printf("[8042] unrecognized key event: scancode=0x%lx\n", mmio_read64(EMEX64_8042_REG_DATA));
}
