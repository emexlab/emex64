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
 * display.h - the screen: the display controller on PCIe (e640:0003, see
 * docs/devices/gpu.md) and the monitor on its DisplayPort connector
 */

#ifndef FW_DISPLAY_H
#define FW_DISPLAY_H

#include <fw.h>

#define GPU_PCI_ID              0x0003E640UL    /* device 0003, vendor e640 */

/* display controller registers (BAR0) */
#define GPU_DP_STATUS           0x100           /* bit 0 hot plug detect, bit 1 link up */
#define GPU_DP_LINK             0x108           /* 1: train the link */
#define GPU_DP_AUX_ADDR         0x110
#define GPU_DP_AUX_DATA         0x118           /* 8 bytes at the AUX address, which advances */
#define GPU_AUX_EDID            0x80000000UL
#define GPU_DP_HPD              1
#define GPU_SCAN_CTRL           0x200
#define GPU_SCAN_BASE           0x208
#define GPU_SCAN_WIDTH          0x210
#define GPU_SCAN_HEIGHT         0x218
#define GPU_SCAN_STRIDE         0x220
#define GPU_SCAN_FORMAT         0x228
#define GPU_FORMAT_I8           1
#define GPU_PALETTE             0x1000          /* 256 x 0x00RRGGBB, one register each */

#define FB_TEXT_COLOR           0xFF

/* text: 8x16 glyphs in lines of 16 pixels, 2 pixels in from the top left */
#define GUI_LINE_HEIGHT         16
#define GUI_GLYPH_BYTES         16
#define GUI_MARGIN              2

extern u16 gui_line, gui_column;
extern const u8 font_data[128 * 16];
extern const u8 la_logo[440], emex_logo[440];

int display_init(void);
void put_init(void);
void gui_putc(int c);
void gui_draw_logo(u64 scale, u64 x, u64 y, const u8 *logo);

#endif /* FW_DISPLAY_H */
