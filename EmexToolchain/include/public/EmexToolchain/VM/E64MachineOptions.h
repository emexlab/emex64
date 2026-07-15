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

#ifndef E64MACHINEOPTIONS_H
#define E64MACHINEOPTIONS_H

#include <EmexFoundation/EmexFoundation.h>

typedef enum: UInt8 {
    kE64PeripheralModeOff,
    kE64PeripheralMode8042,
} E64PeripheralMode;

typedef struct {
    Boolean display;
} E64MachineSupport;

typedef struct {
    Boolean enabled;
    UInt16 width;
    UInt16 height;
} E64DisplayOptions;

typedef struct {
    UInt64 memoryLength;
    E64DisplayOptions displayOptions;
    E64PeripheralMode keyboardPeripheralMode;
    E64PeripheralMode mousePeripheralMode;
} E64MachineOptions;

extern E64MachineSupport E64MachineSupportCurrent;
extern E64MachineOptions E64MachineOptionsDefault;

#endif /* E64MACHINEOPTIONS_H */
