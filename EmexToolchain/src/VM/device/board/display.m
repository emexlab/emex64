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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <dlfcn.h>
#include <CoreGraphics/CoreGraphics.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/board/display.h>
#include <EmexToolchain/VM/device/board/pcie/gpu.h>
#include <EmexToolchain/VM/E64Core.h>

typedef struct {
    UInt8 sc;
    Boolean e0;
} ps2_code_t;

static void mac_system_hotkeys(BOOL enabled)
{
    static int (*main_connection)(void);
    static int (*set_mode)(int connection, int mode);
    static BOOL looked_up;
    if(!looked_up)
    {
        looked_up = YES;
        main_connection = (int (*)(void))dlsym(RTLD_DEFAULT, "CGSMainConnectionID");
        set_mode = (int (*)(int, int))dlsym(RTLD_DEFAULT, "CGSSetGlobalHotKeyOperatingMode");
    }
    if(main_connection != NULL && set_mode != NULL)
    {
        set_mode(main_connection(), enabled ? 0 : 2);
    }
}

static BOOL mac_keyboard_is_iso(void)
{
    static int iso = -1;
    if(iso < 0)
    {
        unsigned char (*get_kbd_type)(void) = (unsigned char (*)(void))dlsym(RTLD_DEFAULT, "LMGetKbdType");
        unsigned int (*get_layout_type)(short) = (unsigned int (*)(short))dlsym(RTLD_DEFAULT, "KBGetLayoutType");
        iso = 0;
        if(get_kbd_type != NULL && get_layout_type != NULL)
        {
            iso = get_layout_type((short)get_kbd_type()) == 0x49534F20;
        }
    }
    return iso;
}

kEmexKeyPhys mac_keycode_to_kEmexKeyPhys(UInt16 keyCode)
{
    switch(keyCode)
    {
        case 10: return mac_keyboard_is_iso() ? kEmexKeyPhysGrave : kEmexKeyPhysNonUSBackslash;
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

        case 50: return mac_keyboard_is_iso() ? kEmexKeyPhysNonUSBackslash : kEmexKeyPhysGrave;
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
        case 55: return kEmexKeyPhysLeftGUI;
        case 54: return kEmexKeyPhysRightGUI;

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

static NSString *const shader_source =
    @"#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct VOut { float4 pos [[position]]; float2 uv; };\n"
    "constant float2 quad_pos[4] = { float2(-1, -1), float2(1, -1), float2(-1, 1), float2(1, 1) };\n"
    "constant float2 quad_uv[4]  = { float2(0, 1),   float2(1, 1),  float2(0, 0),  float2(1, 0) };\n"
    "vertex VOut vmain(uint vid [[vertex_id]])\n"
    "{\n"
    "    VOut o;\n"
    "    o.pos = float4(quad_pos[vid], 0.0, 1.0);\n"
    "    o.uv = quad_uv[vid];\n"
    "    return o;\n"
    "}\n"
    "fragment float4 fmain(VOut in [[stage_in]],\n"
    "                      texture2d<float> image [[texture(0)]],\n"
    "                      texture2d<float> palette [[texture(1)]],\n"
    "                      constant uint &format [[buffer(0)]])\n"
    "{\n"
    "    constexpr sampler s(coord::normalized, filter::nearest, address::clamp_to_edge);\n"
    "    float4 c = image.sample(s, in.uv);\n"
    "    if(format == 1)\n"
    "    {\n"
    "        uint idx = uint(c.r * 255.0 + 0.5);\n"
    "        return float4(palette.read(uint2(idx, 0)).rgb, 1.0);\n"
    "    }\n"
    "    return float4(c.rgb, 1.0);\n"
    "}\n";

@implementation EMEX64MetalView
{
    emex64_display_t *_display;

    id<MTLCommandQueue> _queue;
    id<MTLRenderPipelineState> _pipeline;
    id<MTLTexture> _image;
    id<MTLTexture> _palette;
    UInt32 _imageWidth, _imageHeight, _imageFormat;
    UInt32 _paletteGen;
    emex64_gpu_frame_t _frame;

    NSTimer *_timer;
    NSTrackingArea *_trackingArea;
    BOOL _mouseGrabbed;
    BOOL _keyDown[kEmexKeyPhysUnknown + 1];
    BOOL _guestCapsLock;
}

static const struct {
    UInt16 keyCode;
    NSUInteger deviceMask;
    NSEventModifierFlags flag;
} emex64_modifiers[] = {
    { 56, 0x00000002, NSEventModifierFlagShift },
    { 60, 0x00000004, NSEventModifierFlagShift },
    { 59, 0x00000001, NSEventModifierFlagControl },
    { 62, 0x00002000, NSEventModifierFlagControl },
    { 58, 0x00000020, NSEventModifierFlagOption },
    { 61, 0x00000040, NSEventModifierFlagOption },
    { 55, 0x00000008, NSEventModifierFlagCommand },
    { 54, 0x00000010, NSEventModifierFlagCommand },
};

- (instancetype)initWithFrame:(NSRect)frame
                      display:(emex64_display_t *)display
{
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if(device == nil)
    {
        die("no Metal device");
    }

    self = [super initWithFrame:frame device:device];
    if(!self)
    {
        die("MTKView init failed");
    }

    _display = display;
    memset(&_frame, 0, sizeof(_frame));
    _frame.palette_gen = ~0U;
    _paletteGen = ~0U;

    self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    self.enableSetNeedsDisplay = NO;
    self.paused = YES;
    self.delegate = self;

    _queue = [device newCommandQueue];

    NSError *error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:shader_source options:nil error:&error];
    if(library == nil)
    {
        fprintf(stderr, "Metal shader compile failed: %s\n", error.localizedDescription.UTF8String);
        abort();
    }

    MTLRenderPipelineDescriptor *pd = [[MTLRenderPipelineDescriptor alloc] init];
    pd.vertexFunction = [library newFunctionWithName:@"vmain"];
    pd.fragmentFunction = [library newFunctionWithName:@"fmain"];
    pd.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    _pipeline = [device newRenderPipelineStateWithDescriptor:pd error:&error];
    if(_pipeline == nil)
    {
        fprintf(stderr, "Metal pipeline failed: %s\n", error.localizedDescription.UTF8String);
        abort();
    }

    MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:256 height:1 mipmapped:NO];
    td.usage = MTLTextureUsageShaderRead;
    _palette = [device newTextureWithDescriptor:td];

    [self updateTrackingAreas];

    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowResignedKey:) name:NSWindowDidResignKeyNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowBecameKey:) name:NSWindowDidBecomeKeyNotification object:nil];

    __weak EMEX64MetalView *weakSelf = self;
    _timer = [NSTimer timerWithTimeInterval:EMEX64_FB_TICK_DT repeats:YES block:^(NSTimer *timer) {
        [weakSelf draw];
    }];
    [[NSRunLoop currentRunLoop] addTimer:_timer forMode:NSRunLoopCommonModes];
    return self;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
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

- (void)uploadFrame
{
    BOOL indexed = _frame.format == GPU_FORMAT_I8;

    if(_image == nil || _imageWidth != _frame.width || _imageHeight != _frame.height || _imageFormat != _frame.format)
    {
        MTLTextureDescriptor *td = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:(indexed ? MTLPixelFormatR8Unorm : MTLPixelFormatBGRA8Unorm) width:_frame.width height:_frame.height mipmapped:NO];
        td.usage = MTLTextureUsageShaderRead;
        _image = [self.device newTextureWithDescriptor:td];
        _imageWidth = _frame.width;
        _imageHeight = _frame.height;
        _imageFormat = _frame.format;
    }

    [_image replaceRegion:MTLRegionMake2D(0, 0, _frame.width, _frame.height) mipmapLevel:0 withBytes:_frame.pixels bytesPerRow:_frame.stride];
    if(_paletteGen != _frame.palette_gen)
    {
        UInt8 rgba[GPU_PALETTE_ENTRIES * 4];
        for(int i = 0; i < GPU_PALETTE_ENTRIES; i++)
        {
            rgba[i * 4 + 0] = _frame.palette[i * 3 + 0];
            rgba[i * 4 + 1] = _frame.palette[i * 3 + 1];
            rgba[i * 4 + 2] = _frame.palette[i * 3 + 2];
            rgba[i * 4 + 3] = 0xFF;
        }
        [_palette replaceRegion:MTLRegionMake2D(0, 0, GPU_PALETTE_ENTRIES, 1) mipmapLevel:0 withBytes:rgba bytesPerRow:GPU_PALETTE_ENTRIES * 4];
        _paletteGen = _frame.palette_gen;
    }
}

- (void)drawInMTKView:(MTKView *)view
{
    emex64_gpu_t *gpu = (emex64_gpu_t *)_display->gpu;
    if(gpu != NULL)
    {
        emex64_gpu_get_frame(gpu, &_frame);
    }
    else
    {
        _frame.active = false;
    }

    MTLRenderPassDescriptor *rp = view.currentRenderPassDescriptor;
    id<CAMetalDrawable> drawable = view.currentDrawable;
    if(rp == nil || drawable == nil)
    {
        return;
    }

    id<MTLCommandBuffer> cb = [_queue commandBuffer];
    id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:rp];

    if(_frame.active && _frame.pixels != NULL)
    {
        [self uploadFrame];

        CGSize ds = view.drawableSize;
        double img = (double)_frame.width / (double)_frame.height;
        double win = ds.width / ds.height;
        MTLViewport vp;
        if(win > img)
        {
            vp.width = ds.height * img;
            vp.height = ds.height;
            vp.originX = (ds.width - vp.width) / 2.0;
            vp.originY = 0.0;
        }
        else
        {
            vp.width = ds.width;
            vp.height = ds.width / img;
            vp.originX = 0.0;
            vp.originY = (ds.height - vp.height) / 2.0;
        }
        vp.znear = 0.0;
        vp.zfar = 1.0;

        uint32_t format = _frame.format;
        [enc setViewport:vp];
        [enc setRenderPipelineState:_pipeline];
        [enc setFragmentTexture:_image atIndex:0];
        [enc setFragmentTexture:_palette atIndex:1];
        [enc setFragmentBytes:&format length:sizeof(format) atIndex:0];
        [enc drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    }

    [enc endEncoding];
    [cb presentDrawable:drawable];
    [cb commit];

    if(gpu != NULL)
    {
        emex64_gpu_vblank(gpu);
    }
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size
{
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
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

- (void)guestKey:(kEmexKeyPhys)key down:(BOOL)down
{
    emex64_display_t *d = _display;
    if(!d || !d->emex8042 || key >= kEmexKeyPhysUnknown)
    {
        return;
    }
    if(down)
    {
        emex64_8042_send_keyboard_make(d->emex8042, key);
        _keyDown[key] = YES;
    }
    else if(_keyDown[key])
    {
        emex64_8042_send_keyboard_break(d->emex8042, key);
        _keyDown[key] = NO;
    }
}

- (void)syncCapsLock:(NSEventModifierFlags)flags
{
    BOOL on = (flags & NSEventModifierFlagCapsLock) != 0;
    if(on != _guestCapsLock)
    {
        [self guestKey:kEmexKeyPhysCapsLock down:YES];
        [self guestKey:kEmexKeyPhysCapsLock down:NO];
        _guestCapsLock = on;
    }
}

- (void)releaseModifiers:(NSEventModifierFlags)flags
{
    for(size_t i = 0; i < sizeof(emex64_modifiers) / sizeof(emex64_modifiers[0]); i++)
    {
        if(!(flags & emex64_modifiers[i].flag))
        {
            [self guestKey:mac_keycode_to_kEmexKeyPhys(emex64_modifiers[i].keyCode) down:NO];
        }
    }
}

- (void)windowResignedKey:(NSNotification *)note
{
    if(note.object != self.window)
    {
        return;
    }
    mac_system_hotkeys(YES);
    for(int k = 0; k < kEmexKeyPhysUnknown; k++)
    {
        [self guestKey:(kEmexKeyPhys)k down:NO];
    }
}

- (void)windowBecameKey:(NSNotification *)note
{
    if(note.object == self.window)
    {
        mac_system_hotkeys(NO);
        [self syncCapsLock:[NSEvent modifierFlags]];
    }
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

    [self releaseModifiers:event.modifierFlags];
    [self syncCapsLock:event.modifierFlags];
    [self guestKey:mac_keycode_to_kEmexKeyPhys(event.keyCode) down:YES];
}

- (void)keyUp:(NSEvent *)event
{
    [self releaseModifiers:event.modifierFlags];
    [self guestKey:mac_keycode_to_kEmexKeyPhys(event.keyCode) down:NO];
}

- (void)flagsChanged:(NSEvent *)event
{
    NSUInteger current = event.modifierFlags;
    for(size_t i = 0; i < sizeof(emex64_modifiers) / sizeof(emex64_modifiers[0]); i++)
    {
        kEmexKeyPhys key = mac_keycode_to_kEmexKeyPhys(emex64_modifiers[i].keyCode);
        BOOL down = (current & emex64_modifiers[i].deviceMask) != 0;
        if(emex64_modifiers[i].keyCode == event.keyCode || (!down && _keyDown[key]))
        {
            [self guestKey:key down:down];
        }
    }
    [self syncCapsLock:current];
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

            static id activity;
            activity = [[NSProcessInfo processInfo] beginActivityWithOptions:(NSActivityUserInitiated | NSActivityLatencyCritical) reason:@"emex64vm runs a machine"];
#if !__has_feature(objc_arc)
            [activity retain];
#endif

            NSRect r = NSMakeRect(100, 100, display->width, display->height);
            NSWindow *win = [[NSWindow alloc] initWithContentRect:r styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable) backing:NSBackingStoreBuffered defer:NO];
            [win setTitle:[NSString stringWithFormat:@"emex64vm %d x %d @ 60Hz", display->width, display->height]];
            [[win standardWindowButton:NSWindowZoomButton] setEnabled:NO];

            EMEX64MetalView *view = [[EMEX64MetalView alloc] initWithFrame:r display:display];
            [win setContentView:view];
            [win setDelegate:view];

            [win setContentAspectRatio:NSMakeSize(display->width, display->height)];
            [win setContentMinSize:NSMakeSize(display->width, display->height)];

            [win makeFirstResponder:view];
            [win makeKeyAndOrderFront:nil];
            [NSApp run];
        });
    }
    return NULL;
}

#endif /* __APPLE__ */
