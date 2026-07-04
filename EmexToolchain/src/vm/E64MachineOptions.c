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

#include <EmexToolchain/vm/E64MachineOptions.h>

E64MachineSupport E64MachineSupportGet(void)
{
    E64MachineSupport support;
    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    support.display = true;
    #else
    support.display = false;
    #endif /* EMEX64VM_DEVICE_DISPLAY */
    return support;
}

E64MachineOptions E64MachineOptionsGetDefault(void)
{
    E64MachineOptions options;
    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    options.displayOptions.enabled = true;
    options.keyboardPeripheralMode = kE64PeripheralMode8042;
    options.mousePeripheralMode = kE64PeripheralMode8042;
    #else
    options.displayOptions.enabled = false;
    options.keyboardPeripheralMode = kE64PeripheralModeOff;
    options.mousePeripheralMode = kE64PeripheralModeOff;
    #endif /* EMEX64VM_DEVICE_DISPLAY */
    options.displayOptions.width = 640;
    options.displayOptions.height = 480;
    options.memoryLength = 100 * 1024 * 1024;
    return options;
}
