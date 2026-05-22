/*
 * MIT License
 *
 * Copyright (c) 2024 cr4zyengineer
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

#ifndef LA64VM_DEVICE_DISPLAY_H
#define LA64VM_DEVICE_DISPLAY_H

/* the apple and linux part yayy */
#if defined(__linux__) || defined(__APPLE__)

/* the size of the screen/framebuffer */
#define LA64_FB_WIDTH   256
#define LA64_FB_HEIGHT  256

/* the freequency of the framebuffer */
#define LA64_FB_TICK_HZ 60.0
#define LA64_FB_TICK_DT (1.0 / LA64_FB_TICK_HZ)

/* registers of the framebuffer MMIO device */
#define LA64_FB_REG_ENABLED 0x00
#define LA64_FB_PALLETE     0x01
#define LA64_FB_FRAMEBUFFER 0x301

#define LA64_FB_BASE        0x1FE00700
#define LA64_FB_SIZE        LA64_FB_FRAMEBUFFER + (LA64_FB_WIDTH * LA64_FB_HEIGHT)

#include <stdint.h>
#include <pthread.h>

typedef struct la64_core la64_core_t;
typedef struct la64_machine la64_machine_t;

typedef struct {
    uint8_t enabled;
    uint8_t *palette;
    uint8_t *fb;
    pthread_t pthread;
} la64_display_t;

la64_display_t *la64_display_alloc(la64_machine_t *machine);
void la64_display_dealloc(la64_display_t *display);

void *display_start(void *arg);

uint64_t la64_fb_read(la64_core_t *core, void *device, uint64_t offset, int size);
void la64_fb_write(la64_core_t *core, void *device, uint64_t offset, uint64_t value, int size);

#endif /* __linux__ | __APPLE__ */

/* the apple bozo only part of this header */
#if defined(__APPLE__) && defined(__OBJC__)

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>
#import <OpenGL/OpenGL.h>

@interface LA64GLView : NSOpenGLView
{
    la64_display_t *_display;

    GLuint _prog;
    GLuint _vao, _vbo, _ebo;
    GLuint _texIndex, _texPal;

    NSTimer *_timer;
}

- (instancetype)initWithFrame:(NSRect)frame display:(la64_display_t *)display;

@end

#endif /* __APPLE__ */

#endif /* LA64VM_DEVICE_DISPLAY_H */