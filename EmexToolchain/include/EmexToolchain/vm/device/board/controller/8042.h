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

#include <EmexToolchain/support/keyboard.h>
#include <EmexToolchain/vm/device/base.h>

#define EMEX64_8042_SIZE    0x10

#define EMEX64_8042_DATA    0x00
#define EMEX64_8042_STATUS  0x08

typedef struct emex64_core emex64_core_t;
typedef struct __E64Machine *E64MachineRef;

typedef struct {
    uint8_t status;
    uint8_t command_byte;
    uint8_t last_command;

    uint8_t kbd_buf[64];
    int kbd_head;
    int kbd_tail;

    uint8_t mouse_buf[64];
    int mouse_head;
    int mouse_tail;

    bool kbd_enabled;
    bool mouse_enabled;
    bool expecting_mouse_data;

    pthread_mutex_t lock;
    E64MachineRef machine;

    bool keyboard_attached;
    bool mouse_attached;
} emex64_8042_t;

emex64_8042_t *emex64_8042_alloc(E64MachineRef machine, bool keyboard_attached, bool mouse_attached);
void emex64_8042_dealloc(emex64_8042_t *dev);

/* for display backend */
void emex64_8042_send_keyboard(emex64_8042_t *dev, uint8_t scancode);
void emex64_8042_send_keyboard_make(emex64_8042_t *dev, kEmexKeyPhys key);
void emex64_8042_send_keyboard_break(emex64_8042_t *dev, kEmexKeyPhys key);

void emex64_8042_send_mouse(emex64_8042_t *dev, uint8_t byte);

uint64_t emex64_8042_read(emex64_core_t *core, void *device, uint64_t offset, int size);
void emex64_8042_write(emex64_core_t *core, void *device, uint64_t offset, uint64_t value, int size);

#endif /* EMEX64VM_DEVICE_8042_H */
