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
#include <blk.h>

static struct blkdev *devices[BLK_MAX_DEVICES];
static int device_count;

void blk_register(struct blkdev *b)
{
    if(device_count < BLK_MAX_DEVICES)
    {
        devices[device_count++] = b;
    }
}

int blk_count(void)
{
    return device_count;
}

struct blkdev *blk_get(int index)
{
    return index >= 0 && index < device_count ? devices[index] : NULL;
}
