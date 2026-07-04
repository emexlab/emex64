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

#ifndef EMEX64VM_DEVICE_DISPLAY_H
#define EMEX64VM_DEVICE_DISPLAY_H

#include <stdint.h>
#include <pthread.h>
#include <EmexToolchain/vm/device/base.h>
#include <EmexToolchain/vm/device/board/controller/8042.h>

/* the freequency of the framebuffer */
#define EMEX64_FB_TICK_HZ 60.0
#define EMEX64_FB_TICK_DT (1.0 / EMEX64_FB_TICK_HZ)

/* registers of the framebuffer MMIO device */
#define EMEX64_FB_REG_ENABLED 0x00    /* readonly: from now on only serving the purpose to know if screens are available */
#define EMEX64_FB_REG_HEIGHT  0x08    /* readonly word: telling screen height */
#define EMEX64_FB_REG_WIDTH   0x10    /* readonly word: telling screen width */
#define EMEX64_FB_FRAMEBUFFER 0x18

typedef struct emex64_core emex64_core_t;
typedef struct __E64Machine *E64MachineRef;

typedef struct {
    UInt8 enabled;
    UInt8 *palette;
    UInt8 *fb;
    pthread_t pthread;
    emex64_8042_t *emex8042;

    UInt16 width;
    UInt16 height;
    UInt64 fb_size;
} emex64_display_t;

emex64_display_t *emex64_display_alloc(E64MachineRef machine, Boolean install, UInt16 width, UInt16 height);
void emex64_display_dealloc(emex64_display_t *display);

void *display_start(void *arg);

UInt64 emex64_fb_read(emex64_core_t *core, void *device, UInt64 offset, int size);
void emex64_fb_write(emex64_core_t *core, void *device, UInt64 offset, UInt64 value, int size);

#endif /* __linux__ | __APPLE__ */

/* the apple bozo only part of this header */
#if defined(__APPLE__) && defined(__OBJC__)

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#import <OpenGL/OpenGL.h>

@interface EMEX64GLView : NSOpenGLView <NSWindowDelegate>
{
    emex64_display_t *_display;

    GLuint _prog;
    GLuint _vao, _vbo, _ebo;
    GLuint _texIndex, _texPal;

    NSTimer *_timer;
}

- (instancetype)initWithFrame:(NSRect)frame display:(emex64_display_t *)display;

@end

#endif /* __APPLE__ */

