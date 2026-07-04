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

#ifndef EMEX64VM_OPTIONS_H
#define EMEX64VM_OPTIONS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum: uint8_t {
    kKeyboardModeOff,
    kKeyboardMode8042,
} kKeyboardMode;

typedef enum: uint8_t {
    kMouseModeOff,
    kMouseMode8042,
} kMouseMode;

typedef struct {
    bool display;
} emex64_machine_support_t;

typedef struct {
    bool enabled;
    uint16_t width;
    uint16_t height;
} emex64_machine_display_options_t;

typedef struct {
    emex64_machine_display_options_t display;
    uint64_t memory_size;
    kKeyboardMode keyboard_mode;
    kMouseMode mouse_mode;
} emex64_machine_options_t;

#endif /* EMEX64VM_OPTIONS_H */
