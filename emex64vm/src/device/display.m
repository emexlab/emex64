/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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

#if defined(__APPLE__)

#define GL_SILENCE_DEPRECATION 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#include <emexutils/bitwalker.h>
#include <emex64vm/device/display.h>
#include <emex64vm/core.h>

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

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
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

@implementation LA64GLView

- (instancetype)initWithFrame:(NSRect)frame display:(la64_display_t *)display
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
    _timer = [NSTimer scheduledTimerWithTimeInterval:LA64_FB_TICK_DT repeats:YES block:^(NSTimer *timer){
        [weakSelf setNeedsDisplay:YES];
    }];
    return self;
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
    uint16_t idxs[] = { 0,1,2, 2,3,0 };

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, LA64_FB_WIDTH, LA64_FB_HEIGHT,
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
    glUniform1i(glGetUniformLocation(_prog, "uPalette"),  1);
}

- (void)drawRect:(NSRect)dirtyRect
{
    NSOpenGLContext *ctx = [self openGLContext];
    [ctx makeCurrentContext];

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texIndex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, LA64_FB_WIDTH, LA64_FB_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, _display->fb);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _texPal);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGB, GL_UNSIGNED_BYTE, _display->palette);

    NSRect px = [self convertRectToBacking:[self bounds]];
    GLint winW = (GLint)px.size.width;
    GLint winH = (GLint)px.size.height;

    const float fbAspect  = (float)LA64_FB_WIDTH / (float)LA64_FB_HEIGHT;
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
}

- (BOOL)windowShouldClose:(id)sender
{
    [NSApp terminate:nil];
    return YES;
}

@end

void *display_start(void *arg)
{
    @autoreleasepool
    {
        run_on_main(^{
            la64_display_t *display = (la64_display_t *)arg;
            if (!display) return;

            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            [NSApp activateIgnoringOtherApps:YES];

            NSRect r = NSMakeRect(100, 100, 500, 500);
            NSWindow *win = [[NSWindow alloc] initWithContentRect:r styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |  NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable) backing:NSBackingStoreBuffered defer:NO];
            [win setTitle:@"EMEX64LCD @ 60Hz"];

            LA64GLView *glView = [[LA64GLView alloc] initWithFrame:r display:display];
            [win setContentView:glView];
            [win setDelegate:glView];

            [win makeKeyAndOrderFront:nil];
            [NSApp run];
        });
    }
    return NULL;
}

#endif /* __APPLE__ */
