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

#if defined(__APPLE__)

#define GL_SILENCE_DEPRECATION 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <CoreGraphics/CoreGraphics.h>
#include <EmexToolchain/vm/device/board/display.h>
#include <EmexToolchain/vm/E64Core.h>

typedef struct {
    UInt8 sc;
    Boolean e0;
} ps2_code_t;

kEmexKeyPhys mac_keycode_to_kEmexKeyPhys(UInt16 keyCode)
{
    switch(keyCode)
    {
        case 0: return kEmexKeyPhysA;
        case 11: return kEmexKeyPhysB;
        case 8: return kEmexKeyPhysC;
        case 2: return kEmexKeyPhysD;
        case 14: return kEmexKeyPhysE;
        case 3: return kEmexKeyPhysF;
        case 5: return kEmexKeyPhysG;
        case 4: return kEmexKeyPhysH;
        case 34: return kEmexKeyPhysI;
        case 38: return kEmexKeyPhysJ;
        case 40: return kEmexKeyPhysK;
        case 37: return kEmexKeyPhysL;
        case 46: return kEmexKeyPhysM;
        case 45: return kEmexKeyPhysN;
        case 31: return kEmexKeyPhysO;
        case 35: return kEmexKeyPhysP;
        case 12: return kEmexKeyPhysQ;
        case 15: return kEmexKeyPhysR;
        case 1: return kEmexKeyPhysS;
        case 17: return kEmexKeyPhysT;
        case 32: return kEmexKeyPhysU;
        case 9: return kEmexKeyPhysV;
        case 13: return kEmexKeyPhysW;
        case 7: return kEmexKeyPhysX;
        case 16: return kEmexKeyPhysY;
        case 6: return kEmexKeyPhysZ;

        case 18: return kEmexKeyPhys1;
        case 19: return kEmexKeyPhys2;
        case 20: return kEmexKeyPhys3;
        case 21: return kEmexKeyPhys4;
        case 23: return kEmexKeyPhys5;
        case 22: return kEmexKeyPhys6;
        case 26: return kEmexKeyPhys7;
        case 28: return kEmexKeyPhys8;
        case 25: return kEmexKeyPhys9;
        case 29: return kEmexKeyPhys0;

        case 50: return kEmexKeyPhysGrave;
        case 27: return kEmexKeyPhysMinus;
        case 24: return kEmexKeyPhysEqual;
        case 33: return kEmexKeyPhysLeftBracket;
        case 30: return kEmexKeyPhysRightBracket;
        case 42: return kEmexKeyPhysBackslash;
        case 41: return kEmexKeyPhysSemicolon;
        case 39: return kEmexKeyPhysQuote;
        case 43: return kEmexKeyPhysComma;
        case 47: return kEmexKeyPhysPeriod;
        case 44: return kEmexKeyPhysSlash;

        case 36: return kEmexKeyPhysEnter;
        case 49: return kEmexKeyPhysSpace;
        case 51: return kEmexKeyPhysBackspace;
        case 48: return kEmexKeyPhysTab;
        case 53: return kEmexKeyPhysEsc;

        case 56: return kEmexKeyPhysLeftShift;
        case 60: return kEmexKeyPhysRightShift;
        case 59: return kEmexKeyPhysLeftCtrl;
        case 62: return kEmexKeyPhysRightCtrl;
        case 58: return kEmexKeyPhysLeftAlt;
        case 61: return kEmexKeyPhysRightAlt;
        case 54: return kEmexKeyPhysLeftGUI;
        case 55: return kEmexKeyPhysRightGUI;

        case 123: return kEmexKeyPhysArrowLeft;
        case 124: return kEmexKeyPhysArrowRight;
        case 125: return kEmexKeyPhysArrowDown;
        case 126: return kEmexKeyPhysArrowUp;
        case 115: return kEmexKeyPhysHome;
        case 119: return kEmexKeyPhysEnd;
        case 116: return kEmexKeyPhysPageUp;
        case 121: return kEmexKeyPhysPageDown;
        case 114: return kEmexKeyPhysInsert;
        case 117: return kEmexKeyPhysDelete;

        case 122: return kEmexKeyPhysF1;
        case 120: return kEmexKeyPhysF2;
        case 99: return kEmexKeyPhysF3;
        case 118: return kEmexKeyPhysF4;
        case 96: return kEmexKeyPhysF5;
        case 97: return kEmexKeyPhysF6;
        case 98: return kEmexKeyPhysF7;
        case 100: return kEmexKeyPhysF8;
        case 101: return kEmexKeyPhysF9;
        case 109: return kEmexKeyPhysF10;
        case 103: return kEmexKeyPhysF11;
        case 111: return kEmexKeyPhysF12;

        case 57: return kEmexKeyPhysCapsLock;
        case 71: return kEmexKeyPhysNumLock;
        case 107: return kEmexKeyPhysScrollLock;

        case 105: return kEmexKeyPhysPrintScreen;
        case 113: return kEmexKeyPhysPause;

        default: return kEmexKeyPhysUnknown;
    }
}

static void die(const char* msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    abort();
}

static void run_on_main(void (^block)(void))
{
    if([NSThread isMainThread])
    {
        block();
    }
    else
    {
        dispatch_sync(dispatch_get_main_queue(), block);
    }
}

static GLuint compileShader(GLenum type, const char* src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok)
    {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        GLsizei log_size = (len > 1) ? len : 2;
        char *log = (char *)malloc((size_t)log_size);
        glGetShaderInfoLog(s, log_size, NULL, log);
        fprintf(stderr, "Shader compile failed:\n%s\n", log);
        free(log);
        abort();
    }
    return s;
}

static GLuint linkProgram(GLuint vs, GLuint fs)
{
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if(!ok)
    {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        GLsizei log_size = (len > 1) ? len : 2;
        char *log = (char *)malloc((size_t)log_size);
        glGetProgramInfoLog(p, log_size, NULL, log);
        fprintf(stderr, "Program link failed:\n%s\n", log);
        free(log);
        abort();
    }
    return p;
}

@implementation EMEX64GLView
{
    NSTrackingArea *_trackingArea;
    BOOL _mouseGrabbed;
}

- (instancetype)initWithFrame:(NSRect)frame display:(emex64_display_t *)display
{
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 24,
        0
    };
    NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    
    if(!pf)
    {
        die("NSOpenGLPixelFormat failed");
    }

    self = [super initWithFrame:frame pixelFormat:pf];
    if (!self) die("NSOpenGLView init failed");

    _display = display;
    [self setWantsBestResolutionOpenGLSurface:YES];
    __weak typeof(self) weakSelf = self;
    _timer = [NSTimer scheduledTimerWithTimeInterval:EMEX64_FB_TICK_DT repeats:YES block:^(NSTimer *timer){
        [weakSelf setNeedsDisplay:YES];
    }];
    [[NSRunLoop currentRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];
    [self updateTrackingAreas];
    return self;
}

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];
    if(_trackingArea)
    {
        [self removeTrackingArea:_trackingArea];
        _trackingArea = nil;
    }
    NSTrackingAreaOptions options = NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect;
    _trackingArea = [[NSTrackingArea alloc] initWithRect:NSZeroRect options:options owner:self userInfo:nil];
    [self addTrackingArea:_trackingArea];
}

- (void)prepareOpenGL
{
    [super prepareOpenGL];

    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLContextParameterSwapInterval];

    const char *vsSrc =
        "#version 150 core\n"
        "in vec2 aPos;\n"
        "in vec2 aUV;\n"
        "out vec2 vUV;\n"
        "void main(){\n"
        "  vUV = aUV;\n"
        "  gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "}\n";

    const char *fsSrc =
        "#version 150 core\n"
        "in vec2 vUV;\n"
        "out vec4 FragColor;\n"
        "uniform sampler2D uIndexTex;\n"
        "uniform sampler2D uPalette;\n"
        "void main(){\n"
        "  float idxN = texture(uIndexTex, vUV).r;\n"
        "  float idx  = floor(idxN * 255.0 + 0.5);\n"
        "  float u    = (idx + 0.5) / 256.0;\n"
        "  vec3 rgb   = texture(uPalette, vec2(u, 0.5)).rgb;\n"
        "  FragColor  = vec4(rgb, 1.0);\n"
        "}\n";

    _prog = linkProgram(
        compileShader(GL_VERTEX_SHADER,   vsSrc),
        compileShader(GL_FRAGMENT_SHADER, fsSrc)
    );

    float verts[] = {
        -1.f,-1.f,  0.f,1.f,
         1.f,-1.f,  1.f,1.f,
         1.f, 1.f,  1.f,0.f,
        -1.f, 1.f,  0.f,0.f
    };
    UInt16 idxs[] = { 0,1,2, 2,3,0 };

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxs), idxs, GL_STATIC_DRAW);

    GLint locPos = glGetAttribLocation(_prog, "aPos");
    GLint locUV  = glGetAttribLocation(_prog, "aUV");
    glEnableVertexAttribArray((GLuint)locPos);
    glVertexAttribPointer((GLuint)locPos, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray((GLuint)locUV);
    glVertexAttribPointer((GLuint)locUV,  2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glBindVertexArray(0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &_texIndex);
    glBindTexture(GL_TEXTURE_2D, _texIndex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, _display->width, _display->height,
                 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenTextures(1, &_texPal);
    glBindTexture(GL_TEXTURE_2D, _texPal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 256, 1,
                 0, GL_RGB, GL_UNSIGNED_BYTE, _display->palette);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glUseProgram(_prog);
    glUniform1i(glGetUniformLocation(_prog, "uIndexTex"), 0);
    glUniform1i(glGetUniformLocation(_prog, "uPalette"), 1);
}

- (void)drawRect:(NSRect)dirtyRect
{
    NSOpenGLContext *ctx = [self openGLContext];
    [ctx makeCurrentContext];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texIndex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _display->width, _display->height, GL_RED, GL_UNSIGNED_BYTE, _display->fb);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _texPal);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 512, 1, GL_RGB, GL_UNSIGNED_BYTE, _display->palette);

    NSRect px = [self convertRectToBacking:[self bounds]];
    GLint winW = (GLint)px.size.width;
    GLint winH = (GLint)px.size.height;

    const float fbAspect  = (float)_display->width / (float)_display->height;
    const float winAspect = (float)winW / (float)winH;

    GLint vpX, vpY, vpW, vpH;
    if(winAspect > fbAspect)
    {
        vpH = winH;
        vpW = (GLint)lroundf((float)vpH * fbAspect);
        vpX = (winW - vpW) / 2;
        vpY = 0;
    }
    else
    {
        vpW = winW;
        vpH = (GLint)lroundf((float)vpW / fbAspect);
        vpX = 0;
        vpY = (winH - vpH) / 2;
    }

    glViewport(vpX, vpY, vpW, vpH);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(_prog);
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    [ctx flushBuffer];
}

- (void)reshape
{
    [super reshape];
    [[self openGLContext] update];
}

- (void)dealloc
{
    [_timer invalidate];
    _timer = nil;
    if(_trackingArea)
    {
        [self removeTrackingArea:_trackingArea];
        _trackingArea = nil;
    }
    [self ungrabMouse];
}

- (BOOL)windowShouldClose:(id)sender
{
    [NSApp terminate:nil];
    return YES;
}

static NSString *backuped_windowname;

- (void)grabMouse
{
    if(_mouseGrabbed)
    {
        return;
    }

    backuped_windowname = self.window.title;
    [self.window setTitle:[backuped_windowname stringByAppendingString:@" (Cmd + Option + G to release mouse)"]];

    _mouseGrabbed = YES;
    CGAssociateMouseAndMouseCursorPosition(NO);
    [NSCursor hide];
}

- (void)ungrabMouse
{
    if(!_mouseGrabbed)
    {
        return;
    }

    [self.window setTitle:backuped_windowname];

    _mouseGrabbed = NO;
    CGAssociateMouseAndMouseCursorPosition(YES);
    [NSCursor unhide];
}

- (void)windowDidResignKey:(NSNotification *)notification
{
    [self ungrabMouse];
}

- (void)sendMouseMovement:(NSEvent *)event
{
    emex64_display_t *d = _display;
    if(!d || !d->emex8042)
    {
        return;
    }

    NSUInteger pressed = [NSEvent pressedMouseButtons];
    UInt8 status = 0x08;

    if(pressed & 0x0001)
    {
        status |= 0x01;
    }
    if(pressed & 0x0002)
    {
        status |= 0x02;
    }
    if(pressed & 0x0004)
    {
        status |= 0x04;
    }

    CGFloat dx = event.deltaX;
    CGFloat dy = event.deltaY;

    SInt32 dx32 = (SInt32)lround(dx);
    SInt32 dy32 = (SInt32)lround(dy);

    if(dx32 > 127)
    {
        dx32 = 127;
    }
    if(dx32 < -128)
    {
        dx32 = -128;
    }
    if(dy32 > 127) 
    {
        dy32 = 127;
    }
    if(dy32 < -128)
    {
        dy32 = -128;
    }

    SInt8 dx8 = (SInt8)dx32;
    SInt8 dy8 = (SInt8)dy32;

    if(dx8 < 0)
    {
        status |= 0x10;
    }
    if(dy8 < 0)
    {
        status |= 0x20;
    }

    emex64_8042_send_mouse(d->emex8042, status);
    emex64_8042_send_mouse(d->emex8042, (UInt8)dx8);
    emex64_8042_send_mouse(d->emex8042, (UInt8)dy8);
}

- (void)mouseDown:(NSEvent *)event
{
    emex64_display_t *d = _display;
    if(!d || !d->emex8042)
    {
        return;
    }
    if(!_mouseGrabbed)
    {
        [self grabMouse];
    }
    [self sendMouseMovement:event];
}

- (void)mouseUp:(NSEvent *)event
{
    [self sendMouseMovement:event];
}

- (void)rightMouseDown:(NSEvent *)event
{
    emex64_display_t *d = _display;
    if(!d || !d->emex8042)
    {
        return;
    }
    if(!_mouseGrabbed)
    {
        [self grabMouse];
    }
    [self sendMouseMovement:event];
}

- (void)rightMouseUp:(NSEvent *)event
{
    [self sendMouseMovement:event];
}

- (void)otherMouseDown:(NSEvent *)event
{
    emex64_display_t *d = _display;
    if(!d || !d->emex8042)
    {
        return;
    }
    if(!_mouseGrabbed)
    {
        [self grabMouse];
    }
    [self sendMouseMovement:event];
}

- (void)otherMouseUp:(NSEvent *)event
{
    [self sendMouseMovement:event];
}

- (void)mouseMoved:(NSEvent *)event
{
    [self sendMouseMovement:event];
}

- (void)mouseDragged:(NSEvent *)event
{
    [self sendMouseMovement:event];
}

- (void)rightMouseDragged:(NSEvent *)event
{
    [self sendMouseMovement:event];
}

- (void)otherMouseDragged:(NSEvent *)event
{
    [self sendMouseMovement:event];
}

- (void)keyDown:(NSEvent *)event
{
    if((event.modifierFlags & (NSEventModifierFlagOption | NSEventModifierFlagCommand)) == (NSEventModifierFlagOption | NSEventModifierFlagCommand))
    {
        if(event.keyCode == 5)
        {
            [self ungrabMouse];
            return;
        }
    }

    emex64_display_t *d = _display;
    kEmexKeyPhys phys_key = mac_keycode_to_kEmexKeyPhys(event.keyCode);
    emex64_8042_send_keyboard_make(d->emex8042, phys_key);
}

- (void)keyUp:(NSEvent *)event
{
    emex64_display_t *d = _display;
    kEmexKeyPhys phys_key = mac_keycode_to_kEmexKeyPhys(event.keyCode);
    emex64_8042_send_keyboard_break(d->emex8042, phys_key);
}

- (void)flagsChanged:(NSEvent *)event
{
    emex64_display_t *d = _display;
    static NSUInteger lastFlags = 0;
    NSUInteger current = event.modifierFlags;

    if((current & NSEventModifierFlagShift) != (lastFlags & NSEventModifierFlagShift))
    {
        BOOL pressed = (current & NSEventModifierFlagShift) != 0;
        kEmexKeyPhys key = kEmexKeyPhysLeftShift;
        if(pressed)
        {
            emex64_8042_send_keyboard_make(d->emex8042, key);
        }
        else
        {
            emex64_8042_send_keyboard_break(d->emex8042, key);
        }
    }

    if((current & NSEventModifierFlagControl) != (lastFlags & NSEventModifierFlagControl))
    {
        BOOL pressed = (current & NSEventModifierFlagControl) != 0;
        kEmexKeyPhys key = kEmexKeyPhysLeftCtrl;
        if(pressed)
        {
            emex64_8042_send_keyboard_make(d->emex8042, key);
        }
        else
        {
            emex64_8042_send_keyboard_break(d->emex8042, key);
        }
    }

    if((current & NSEventModifierFlagOption) != (lastFlags & NSEventModifierFlagOption))
    {
        BOOL pressed = (current & NSEventModifierFlagOption) != 0;
        kEmexKeyPhys key = kEmexKeyPhysLeftAlt;
        if(pressed)
        {
            emex64_8042_send_keyboard_make(d->emex8042, key);
        }
        else
        {
            emex64_8042_send_keyboard_break(d->emex8042, key);
        }
    }

    if((current & NSEventModifierFlagCommand) != (lastFlags & NSEventModifierFlagCommand))
    {
        BOOL pressed = (current & NSEventModifierFlagCommand) != 0;
        kEmexKeyPhys key = kEmexKeyPhysLeftGUI;
        if(pressed)
        {
            emex64_8042_send_keyboard_make(d->emex8042, key);
        }
        else
        {
            emex64_8042_send_keyboard_break(d->emex8042, key);
        }
    }

    if((current & NSEventModifierFlagCapsLock) != (lastFlags & NSEventModifierFlagCapsLock))
    {
        BOOL pressed = (current & NSEventModifierFlagCapsLock) != 0;
        if(pressed)
        {
            emex64_8042_send_keyboard_make(d->emex8042, kEmexKeyPhysCapsLock);
        }
        else
        {
            emex64_8042_send_keyboard_break(d->emex8042, kEmexKeyPhysCapsLock);
        }
    }

    lastFlags = current;
}

@end

void *display_start(void *arg)
{
    @autoreleasepool
    {
        run_on_main(^{
            emex64_display_t *display = (emex64_display_t *)arg;
            if(!display)
            {
                return;
            }

            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            [NSApp activateIgnoringOtherApps:YES];

            NSRect r = NSMakeRect(100, 100, display->width, display->height);
            NSWindow *win = [[NSWindow alloc] initWithContentRect:r styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable) backing:NSBackingStoreBuffered defer:NO];
            [win setTitle:[NSString stringWithFormat:@"emex64vm %d x %d @ 60Hz", display->width, display->height]];
            [[win standardWindowButton:NSWindowZoomButton] setEnabled:NO];

            EMEX64GLView *glView = [[EMEX64GLView alloc] initWithFrame:r display:display];
            [win setContentView:glView];
            [win setDelegate:glView];

            [win setContentAspectRatio:NSMakeSize(display->width, display->height)];
            [win setContentMinSize:NSMakeSize(display->width, display->height)];

            [win makeFirstResponder:glView];
            [win makeKeyAndOrderFront:nil];
            [NSApp run];
        });
    }
    return NULL;
}

#endif /* __APPLE__ */
