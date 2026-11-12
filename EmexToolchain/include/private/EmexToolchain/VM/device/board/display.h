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

#include <pthread.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/base.h>
#include <EmexToolchain/VM/device/board/controller/8042.h>

#define EMEX64_FB_TICK_HZ 60.0
#define EMEX64_FB_TICK_DT (1.0 / EMEX64_FB_TICK_HZ)

typedef struct __E64Core *E64CoreRef;
typedef struct __E64Machine *E64MachineRef;
typedef struct emex64_display emex64_display_t;

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || (defined(__APPLE__) && !TARGET_OS_IPHONE))
#define EMEX64VM_DISPLAY_WINDOW 1
#else
#define EMEX64VM_DISPLAY_WINDOW 0
#endif

struct emex64_display {
    Boolean enabled;
    Boolean embedded;
    volatile Boolean stopping;
    pthread_t pthread;
    emex64_8042_t *emex8042;
    void *gpu;

    UInt16 width;
    UInt16 height;
};

emex64_display_t *emex64_display_alloc(E64MachineRef machine, Boolean install, UInt16 width, UInt16 height);
void emex64_display_dealloc(emex64_display_t *display);
void emex64_display_stop(emex64_display_t *display);

void *display_start(void *arg);

/* the apple bozo only part of this header */
#if defined(__APPLE__) && defined(__OBJC__) && !TARGET_OS_IPHONE

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

@interface EMEX64MetalView : MTKView <NSWindowDelegate, MTKViewDelegate>

- (instancetype)initWithFrame:(NSRect)frame display:(emex64_display_t *)display;

@end

#endif /* __APPLE__ && __OBJC__ */

#endif /* EMEX64VM_DEVICE_DISPLAY_H */
