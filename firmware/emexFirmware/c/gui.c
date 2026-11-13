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
#include <display.h>

u16 gui_line, gui_column;

static void gui_scroll_up(u64 lines)
{
    u64 shift = lines * gui_width;
    u64 rest = (gui_height - lines) * gui_width;
    memmove((void *)gui_fb, (void *)(gui_fb + shift), rest);
    memset((void *)(gui_fb + rest), 0, shift);
}

static void gui_draw_glyph(unsigned char c,
                           u64 x,
                           u64 y)
{
    const u8 *glyph = &font_data[c * GUI_GLYPH_BYTES];
    for(u64 row = 0; row < GUI_GLYPH_BYTES && y + row < gui_height; row++)
    {
        u8 *line = (u8 *)(gui_fb + (y + row) * gui_width);
        for(u64 col = 0; col < 8 && x + col < gui_width; col++)
        {
            line[x + col] = (glyph[row] >> (7 - col)) & 1 ? FB_TEXT_COLOR : 0;
        }
    }
}

void gui_putc(int c)
{
    unsigned char ch = (unsigned char)c;
    if(ch == 0)
    {
        return;
    }

    if(ch == '\n')
    {
        gui_column = 0;
        if(gui_line + 1 >= gui_height / GUI_LINE_HEIGHT)
        {
            gui_scroll_up(GUI_LINE_HEIGHT);
        }
        else
        {
            gui_line++;
        }
        return;
    }
    if(ch == '\r')
    {
        gui_column = 0;
        return;
    }

    u64 x = (u64)gui_column * 8 + GUI_MARGIN;
    u64 y = (u64)gui_line * GUI_LINE_HEIGHT + GUI_MARGIN;
    gui_column++;
    if(ch > 127)
    {
        return;
    }
    gui_draw_glyph(ch, x, y);
}

void gui_draw_logo(u64 scale,
                   u64 x,
                   u64 y,
                   const u8 *logo)
{
    for(u64 py = 0; py < 20; py++)
    {
        for(u64 px = 0; px < 22; px++)
        {
            u8 color = logo[py * 22 + px];
            if(color == 0)
            {
                continue;
            }
            for(u64 sy = 0; sy < scale; sy++)
            {
                u64 fy = y + py * scale + sy;
                if(fy >= gui_height)
                {
                    break;
                }
                for(u64 sx = 0; sx < scale; sx++)
                {
                    u64 fx = x + px * scale + sx;
                    if(fx >= gui_width)
                    {
                        break;
                    }
                    *(u8 *)(gui_fb + fy * gui_width + fx) = color;
                }
            }
        }
    }
}
