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

#include <EmexToolchain/vm/E64Machine.h>
#include <EmexToolchain/vm/E64Core.h>
#include <EmexToolchain/vm/device/board/controller/8042.h>
#include <EmexToolchain/vm/device/internal/controller/E64IC.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    UInt8 length;
    UInt8 data[4];
} ps2_scancode_t;

static const ps2_scancode_t kEmexKeyPhysToPS2Set1[] = {
    [kEmexKeyPhysEsc] = {1, {0x01}},
    [kEmexKeyPhysF1] = {1, {0x3B}},
    [kEmexKeyPhysF2] = {1, {0x3C}},
    [kEmexKeyPhysF3] = {1, {0x3D}},
    [kEmexKeyPhysF4] = {1, {0x3E}},
    [kEmexKeyPhysF5] = {1, {0x3F}},
    [kEmexKeyPhysF6] = {1, {0x40}},
    [kEmexKeyPhysF7] = {1, {0x41}},
    [kEmexKeyPhysF8] = {1, {0x42}},
    [kEmexKeyPhysF9] = {1, {0x43}},
    [kEmexKeyPhysF10] = {1, {0x44}},
    [kEmexKeyPhysF11] = {1, {0x57}},
    [kEmexKeyPhysF12] = {1, {0x58}},
    [kEmexKeyPhysGrave] = {1, {0x29}},
    [kEmexKeyPhys1] = {1, {0x02}},
    [kEmexKeyPhys2] = {1, {0x03}},
    [kEmexKeyPhys3] = {1, {0x04}},
    [kEmexKeyPhys4] = {1, {0x05}},
    [kEmexKeyPhys5] = {1, {0x06}},
    [kEmexKeyPhys6] = {1, {0x07}},
    [kEmexKeyPhys7] = {1, {0x08}},
    [kEmexKeyPhys8] = {1, {0x09}},
    [kEmexKeyPhys9] = {1, {0x0A}},
    [kEmexKeyPhys0] = {1, {0x0B}},
    [kEmexKeyPhysMinus] = {1, {0x0C}},
    [kEmexKeyPhysEqual] = {1, {0x0D}},
    [kEmexKeyPhysBackspace] = {1, {0x0E}},
    [kEmexKeyPhysTab] = {1, {0x0F}},
    [kEmexKeyPhysQ] = {1, {0x10}},
    [kEmexKeyPhysW] = {1, {0x11}},
    [kEmexKeyPhysE] = {1, {0x12}},
    [kEmexKeyPhysR] = {1, {0x13}},
    [kEmexKeyPhysT] = {1, {0x14}},
    [kEmexKeyPhysY] = {1, {0x15}},
    [kEmexKeyPhysU] = {1, {0x16}},
    [kEmexKeyPhysI] = {1, {0x17}},
    [kEmexKeyPhysO] = {1, {0x18}},
    [kEmexKeyPhysP] = {1, {0x19}},
    [kEmexKeyPhysLeftBracket] = {1, {0x1A}},
    [kEmexKeyPhysRightBracket] = {1, {0x1B}},
    [kEmexKeyPhysBackslash] = {1, {0x2B}},
    [kEmexKeyPhysCapsLock] = {1, {0x3A}},
    [kEmexKeyPhysA] = {1, {0x1E}},
    [kEmexKeyPhysS] = {1, {0x1F}},
    [kEmexKeyPhysD] = {1, {0x20}},
    [kEmexKeyPhysF] = {1, {0x21}},
    [kEmexKeyPhysG] = {1, {0x22}},
    [kEmexKeyPhysH] = {1, {0x23}},
    [kEmexKeyPhysJ] = {1, {0x24}},
    [kEmexKeyPhysK] = {1, {0x25}},
    [kEmexKeyPhysL] = {1, {0x26}},
    [kEmexKeyPhysSemicolon] = {1, {0x27}},
    [kEmexKeyPhysQuote] = {1, {0x28}},
    [kEmexKeyPhysEnter] = {1, {0x1C}},
    [kEmexKeyPhysLeftShift] = {1, {0x2A}},
    [kEmexKeyPhysZ] = {1, {0x2C}},
    [kEmexKeyPhysX] = {1, {0x2D}},
    [kEmexKeyPhysC] = {1, {0x2E}},
    [kEmexKeyPhysV] = {1, {0x2F}},
    [kEmexKeyPhysB] = {1, {0x30}},
    [kEmexKeyPhysN] = {1, {0x31}},
    [kEmexKeyPhysM] = {1, {0x32}},
    [kEmexKeyPhysComma] = {1, {0x33}},
    [kEmexKeyPhysPeriod] = {1, {0x34}},
    [kEmexKeyPhysSlash] = {1, {0x35}},
    [kEmexKeyPhysRightShift] = {1, {0x36}},
    [kEmexKeyPhysLeftCtrl] = {1, {0x1D}},
    [kEmexKeyPhysLeftGUI] = {2, {0xE0, 0x5B}},
    [kEmexKeyPhysLeftAlt] = {1, {0x38}},
    [kEmexKeyPhysSpace] = {1, {0x39}},
    [kEmexKeyPhysRightAlt] = {2, {0xE0, 0x38}},
    [kEmexKeyPhysRightGUI] = {2, {0xE0, 0x5C}},
    [kEmexKeyPhysMenu] = {2, {0xE0, 0x5D}},
    [kEmexKeyPhysRightCtrl] = {2, {0xE0, 0x1D}},
    [kEmexKeyPhysInsert] = {2, {0xE0, 0x52}},
    [kEmexKeyPhysDelete] = {2, {0xE0, 0x53}},
    [kEmexKeyPhysHome] = {2, {0xE0, 0x47}},
    [kEmexKeyPhysEnd] = {2, {0xE0, 0x4F}},
    [kEmexKeyPhysPageUp] = {2, {0xE0, 0x49}},
    [kEmexKeyPhysPageDown] = {2, {0xE0, 0x51}},
    [kEmexKeyPhysArrowUp] = {2, {0xE0, 0x48}},
    [kEmexKeyPhysArrowLeft] = {2, {0xE0, 0x4B}},
    [kEmexKeyPhysArrowDown] = {2, {0xE0, 0x50}},
    [kEmexKeyPhysArrowRight] = {2, {0xE0, 0x4D}},
    [kEmexKeyPhysNumLock] = {1, {0x45}},
    [kEmexKeyPhysNumpadDivide] = {2, {0xE0, 0x35}},
    [kEmexKeyPhysNumpadMultiply] = {1, {0x37}},
    [kEmexKeyPhysNumpadMinus] = {1, {0x4A}},
    [kEmexKeyPhysNumpadPlus] = {1, {0x4E}},
    [kEmexKeyPhysNumpadEnter] = {2, {0xE0, 0x1C}},
    [kEmexKeyPhysNumpad1] = {1, {0x4F}},
    [kEmexKeyPhysNumpad2] = {1, {0x50}},
    [kEmexKeyPhysNumpad3] = {1, {0x51}},
    [kEmexKeyPhysNumpad4] = {1, {0x4B}},
    [kEmexKeyPhysNumpad5] = {1, {0x4C}},
    [kEmexKeyPhysNumpad6] = {1, {0x4D}},
    [kEmexKeyPhysNumpad7] = {1, {0x47}},
    [kEmexKeyPhysNumpad8] = {1, {0x48}},
    [kEmexKeyPhysNumpad9] = {1, {0x49}},
    [kEmexKeyPhysNumpad0] = {1, {0x52}},
    [kEmexKeyPhysNumpadDot] = {1, {0x53}},
    [kEmexKeyPhysPrintScreen] = {4, {0xE0, 0x2A, 0xE0, 0x37}},
    [kEmexKeyPhysScrollLock] = {1, {0x46}},
    [kEmexKeyPhysPause] = {3, {0xE1, 0x1D, 0x45}},
    [kEmexKeyPhysUnknown] = {0, {0}},
};

#define STATUS_OBF          0x01
#define STATUS_MOUSE_OBF    0x20

emex64_8042_t *emex64_8042_alloc(E64MachineRef machine,
                                 Boolean keyboard_attached,
                                 Boolean mouse_attached)
{
    emex64_8042_t *dev = calloc(1, sizeof(emex64_8042_t));
    if(!dev)
    {
        return NULL;
    }

    dev->keyboard_attached = keyboard_attached;
    dev->mouse_attached = mouse_attached;

    pthread_mutex_init(&dev->lock, NULL);
    dev->machine = machine;
    dev->kbd_enabled = true;
    dev->mouse_enabled = true;
    dev->status = 0x10;

    E64MMIORegionRef MMIO8042Region = E64MMIORegionCreate(kEFAllocatorDefault, EMEX64_8042_BASE, EMEX64_8042_SIZE, dev, emex64_8042_read, emex64_8042_write);
    if(MMIO8042Region == NULL)
    {
        free(dev);
        return NULL;
    }

    Boolean success = E64MMIOBusRegisterRegion(machine->mmio_bus, MMIO8042Region);
    EFRelease(MMIO8042Region);
    if(!success)
    {
        free(dev);
        return NULL;
    }

    return dev;
}

void emex64_8042_dealloc(emex64_8042_t *dev)
{
    pthread_mutex_destroy(&dev->lock);
    free(dev);
}

static void update_8042_interrupt(emex64_8042_t *dev)
{
    Boolean has_kbd = (dev->kbd_head   != dev->kbd_tail);
    Boolean has_mouse = (dev->mouse_head != dev->mouse_tail);

    if(has_kbd || has_mouse)
    {
        dev->status |= STATUS_OBF;

        if(has_mouse && !has_kbd)
        {
            dev->status |= STATUS_MOUSE_OBF;
        }
        else
        {
            dev->status &= ~STATUS_MOUSE_OBF;
        }

        E64ICRaiseInterrupt(dev->machine->intc, EMEX64_IRQ_8042);
    }
    else
    {
        dev->status &= ~(STATUS_OBF | STATUS_MOUSE_OBF);
    }
}

void emex64_8042_send_keyboard(emex64_8042_t *dev, UInt8 scancode)
{
    if(dev->keyboard_attached)
    {
        pthread_mutex_lock(&dev->lock);

        int next = (dev->kbd_tail + 1) % 64;

        if(next == dev->kbd_head)
        {
            dev->kbd_head = (dev->kbd_head + 1) % 64;
        }

        dev->kbd_buf[dev->kbd_tail] = scancode;
        dev->kbd_tail = next;

        update_8042_interrupt(dev);

        pthread_mutex_unlock(&dev->lock);
    }
}

void emex64_8042_send_keyboard_make(emex64_8042_t *dev, kEmexKeyPhys key)
{
    if(dev->keyboard_attached)
    {
        const ps2_scancode_t *sc = &kEmexKeyPhysToPS2Set1[key];
        for(UInt8 i = 0; i < sc->length; i++)
        {
            emex64_8042_send_keyboard(dev, sc->data[i]);
        }
    }
}

void emex64_8042_send_keyboard_break(emex64_8042_t *dev, kEmexKeyPhys key)
{
    if(dev->keyboard_attached)
    {
        const ps2_scancode_t *sc = &kEmexKeyPhysToPS2Set1[key];

        if(key == kEmexKeyPhysPause)
        {
            /* pause has no normal break code */
            return;
        }

        if(sc->length == 1)
        {
            emex64_8042_send_keyboard(dev, 0x80 | sc->data[0]);
        }
        else if(sc->length == 2)
        {
            emex64_8042_send_keyboard(dev, 0xE0);
            emex64_8042_send_keyboard(dev, 0x80 | sc->data[1]);
        }
        else if(key == kEmexKeyPhysPrintScreen)
        {
            emex64_8042_send_keyboard(dev, 0xE0);
            emex64_8042_send_keyboard(dev, 0xB7);
            emex64_8042_send_keyboard(dev, 0xE0);
            emex64_8042_send_keyboard(dev, 0xAA);
        }
    }  
}

void emex64_8042_send_mouse(emex64_8042_t *dev, UInt8 byte)
{
    if(dev->mouse_attached)
    {
        pthread_mutex_lock(&dev->lock);

        int next = (dev->mouse_tail + 1) % 64;

        if(next == dev->mouse_head)
        {
            dev->mouse_head = (dev->mouse_head + 1) % 64;
        }

        dev->mouse_buf[dev->mouse_tail] = byte;
        dev->mouse_tail = next;

        update_8042_interrupt(dev);

        pthread_mutex_unlock(&dev->lock);
    }
}

UInt64 emex64_8042_read(E64CoreRef core, void *device, UInt64 offset, int size)
{
    emex64_8042_t *dev = device;
    UInt64 val = 0;

    pthread_mutex_lock(&dev->lock);

    if(offset == EMEX64_8042_DATA)
    {
        if(dev->kbd_head != dev->kbd_tail)
        {
            val = dev->kbd_buf[dev->kbd_head];
            dev->kbd_head = (dev->kbd_head + 1) % 64;
            dev->status &= ~STATUS_MOUSE_OBF;
        }
        else if(dev->mouse_head != dev->mouse_tail)
        {
            val = dev->mouse_buf[dev->mouse_head];
            dev->mouse_head = (dev->mouse_head + 1) % 64;
        }

        update_8042_interrupt(dev);
    }
    else if(offset == EMEX64_8042_STATUS)
    {
        val = dev->status;
    }

    pthread_mutex_unlock(&dev->lock);
    return val;
}

void emex64_8042_write(E64CoreRef core,
                       void *device,
                       UInt64 offset,
                       UInt64 value,
                       int size)
{
    emex64_8042_t *dev = device;

    pthread_mutex_lock(&dev->lock);

    if(offset == EMEX64_8042_DATA)
    {
        if(dev->last_command == 0x60)
        {
            dev->command_byte = value;
        }
        dev->last_command = 0;
    }
    else if(offset == EMEX64_8042_STATUS)
    {
        dev->last_command = value;

        switch(value)
        {
            case 0x20:
                break;
            case 0x60:
                break;
            case 0xA7:
                dev->mouse_enabled = false;
                break;
            case 0xA8:
                dev->mouse_enabled = true;
                break;
            case 0xAD:
                dev->kbd_enabled = false;
                break;
            case 0xAE:
                dev->kbd_enabled = true;
                break;
            case 0xD4:
                dev->expecting_mouse_data = true;
                break;
        }
    }

    pthread_mutex_unlock(&dev->lock);
}
