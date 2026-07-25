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

#ifndef EMEX64VM_DEVICE_8042_H
#define EMEX64VM_DEVICE_8042_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/keyboard.h>
#include <EmexToolchain/VM/device/base.h>

#define EMEX64_8042_SIZE    0x10

#define EMEX64_8042_DATA    0x00
#define EMEX64_8042_STATUS  0x08

typedef struct __E64Core *E64CoreRef;
typedef struct __E64Machine *E64MachineRef;

typedef struct {
    UInt8 status;
    UInt8 command_byte;
    UInt8 last_command;

    UInt8 kbd_buf[64];
    SInt32 kbd_head;
    SInt32 kbd_tail;

    UInt8 mouse_buf[64];
    SInt32 mouse_head;
    SInt32 mouse_tail;

    Boolean kbd_enabled;
    Boolean mouse_enabled;
    Boolean expecting_mouse_data;

    pthread_mutex_t lock;
    E64MachineRef machine;

    Boolean keyboard_attached;
    Boolean mouse_attached;
} emex64_8042_t;

emex64_8042_t *emex64_8042_alloc(E64MachineRef machine, Boolean keyboard_attached, Boolean mouse_attached);
void emex64_8042_dealloc(emex64_8042_t *dev);

/* for display backend */
void emex64_8042_send_keyboard(emex64_8042_t *dev, UInt8 scancode);
void emex64_8042_send_keyboard_make(emex64_8042_t *dev, kEmexKeyPhys key);
void emex64_8042_send_keyboard_break(emex64_8042_t *dev, kEmexKeyPhys key);

void emex64_8042_send_mouse(emex64_8042_t *dev, UInt8 byte);

UInt64 emex64_8042_read(E64CoreRef core, void *device, UInt64 offset, EFSize size);
void emex64_8042_write(E64CoreRef core, void *device, UInt64 offset, UInt64 value, EFSize size);

#endif /* EMEX64VM_DEVICE_8042_H */
