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

#include <EmexToolchain/VM/E64MachineOptions.h>

E64MachineSupport E64MachineSupportCurrent = {
    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    .display = true,
    #else
    .display = false,
    #endif /* EMEX64VM_DEVICE_DISPLAY */
};

E64MachineOptions E64MachineOptionsDefault = {
    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    .displayOptions = {
        .enabled = true,
        .width = 640,
        .height = 480,
    },
    .keyboardPeripheralMode = kE64PeripheralMode8042,
    .mousePeripheralMode = kE64PeripheralMode8042,
    #else
    .displayOptions = {
        .enabled = false,
    },
    .keyboardPeripheralMode = kE64PeripheralModeOff,
    .mousePeripheralMode = kE64PeripheralModeOff,
    #endif /* EMEX64VM_DEVICE_DISPLAY */
    .memoryLength = 100 * 1024 * 1024,
};
