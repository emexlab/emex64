/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EMEX64VM_DEVICE_DISPLAY_H
#define EMEX64VM_DEVICE_DISPLAY_H

#include <stdint.h>
#include <pthread.h>
#include <emex64lib/vm/device/base.h>
#include <emex64lib/vm/device/board/controller/8042.h>

/* the freequency of the framebuffer */
#define EMEX64_FB_TICK_HZ 60.0
#define EMEX64_FB_TICK_DT (1.0 / EMEX64_FB_TICK_HZ)

/* registers of the framebuffer MMIO device */
#define EMEX64_FB_REG_ENABLED 0x00    /* readonly: from now on only serving the purpose to know if screens are available */
#define EMEX64_FB_REG_HEIGHT  0x08    /* readonly word: telling screen height */
#define EMEX64_FB_REG_WIDTH   0x10    /* readonly word: telling screen width */
#define EMEX64_FB_FRAMEBUFFER 0x18

typedef struct emex64_core emex64_core_t;
typedef struct emex64_machine emex64_machine_t;

typedef struct {
    uint8_t enabled;
    uint8_t *palette;
    uint8_t *fb;
    pthread_t pthread;
    emex64_8042_t *emex8042;

    uint16_t width;
    uint16_t height;
    uint64_t fb_size;
} emex64_display_t;

emex64_display_t *emex64_display_alloc(emex64_machine_t *machine, bool install, uint16_t width, uint16_t height);
void emex64_display_dealloc(emex64_display_t *display);

void *display_start(void *arg);

uint64_t emex64_fb_read(emex64_core_t *core, void *device, uint64_t offset, int size);
void emex64_fb_write(emex64_core_t *core, void *device, uint64_t offset, uint64_t value, int size);

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

