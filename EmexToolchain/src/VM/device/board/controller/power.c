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

#include <stdio.h>
#include <stdlib.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/board/controller/power.h>
#include <EmexToolchain/VM/E64Machine.h>

UInt64 emex64_platform_read(E64CoreRef core,
                            void *device,
                            UInt64 offset,
                            EFSize size)
{
    return 1;
}

void emex64_platform_write(E64CoreRef core,
                           void *device,
                           UInt64 offset,
                           UInt64 value,
                           EFSize size)
{
    if(value == 0)
    {
        E64CoreTerminate(core);
    }
}
