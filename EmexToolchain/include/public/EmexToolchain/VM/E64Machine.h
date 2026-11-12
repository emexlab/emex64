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

#ifndef E64MACHINE_H
#define E64MACHINE_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64MachineOptions.h>
#include <EmexToolchain/VM/E64Core.h>
#include <EmexToolchain/VM/E64Memory.h>
#include <EmexToolchain/VM/E64MMIOBus.h>
#include <EmexToolchain/VM/device/base.h>
#include <EmexToolchain/Support/keyboard.h>
#include <EmexToolchain/VM/device/internal/controller/E64IC.h>
#ifdef ET_PRIVATE
#include <EmexToolchain/VM/__E64Machine.h>
#endif /* ET_PRIVATE */

typedef struct __E64Machine *E64MachineRef;

EFTypeID E64MachineGetTypeID(void);

E64MachineRef E64MachineCreate(EFAllocatorRef allocatorRef);
E64MachineRef E64MachineCreateWithOptions(EFAllocatorRef allocatorRef, E64MachineOptions options);

E64CoreRef E64MachineGetCore(E64MachineRef machineRef);
E64MemoryRef E64MachineGetMemory(E64MachineRef machineRef);
E64MMIOBusRef E64MachineGetMMIOBus(E64MachineRef machineRef);

E64ICRef E64MachineGetIC(E64MachineRef machineRef);

/*
 * For a host that embeds the machine (E64MachineOptions.host). These are
 * safe from any thread while E64CoreExecute runs on another.
 */

/* powers the machine off: E64CoreExecute returns */
void E64MachineStop(E64MachineRef machineRef);

/* bytes typed into the UART (the serial console) */
void E64MachineSendUARTInput(E64MachineRef machineRef, const UInt8 *bytes, EFSize length);

/* a key on the keyboard (by position, as a PC keyboard sends it) went down or up */
void E64MachineSendKey(E64MachineRef machineRef, kEmexKeyPhys key, Boolean down);

/* what the monitor shows now */
typedef struct {
    Boolean active;             /* false: the screen is black */
    const UInt8 *pixels;        /* in guest RAM: read it while drawing, don't keep it */
    UInt32 width, height;       /* pixels */
    UInt32 stride;              /* bytes from one row to the next */
    UInt32 format;              /* 1: 8-bit palette indices, 2: XRGB8888 (bytes B, G, R, X) */
    UInt32 paletteGeneration;   /* changes when the palette does */
    UInt8 palette[256 * 3];     /* R, G, B for format 1; copied when paletteGeneration changed */
    /* the rows written since the last call: [dirtyTop, dirtyBottom), all
       of them for a new scanout, none when the picture didn't change */
    UInt32 dirtyTop, dirtyBottom;
} E64DisplayFrame;

/* false without a display (E64MachineOptions.displayOptions.enabled). Zero
   the frame once and pass the same one every time: the palette is only
   copied when it changed */
Boolean E64MachineGetDisplayFrame(E64MachineRef machineRef, E64DisplayFrame *frame);

#endif /* E64MACHINE_H */
