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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/device/board/display.h>
#include <EmexToolchain/VM/device/board/pcie/gpu.h>

#if defined(__linux__) && EMEX64VM_DEVICE_DISPLAY

#include <GL/glew.h>
#include <GLFW/glfw3.h>

kEmexKeyPhys glfw_key_to_kEmexKeyPhys(SInt32 key)
{
    switch(key)
    {
        case GLFW_KEY_A: return kEmexKeyPhysA;
        case GLFW_KEY_B: return kEmexKeyPhysB;
        case GLFW_KEY_C: return kEmexKeyPhysC;
        case GLFW_KEY_D: return kEmexKeyPhysD;
        case GLFW_KEY_E: return kEmexKeyPhysE;
        case GLFW_KEY_F: return kEmexKeyPhysF;
        case GLFW_KEY_G: return kEmexKeyPhysG;
        case GLFW_KEY_H: return kEmexKeyPhysH;
        case GLFW_KEY_I: return kEmexKeyPhysI;
        case GLFW_KEY_J: return kEmexKeyPhysJ;
        case GLFW_KEY_K: return kEmexKeyPhysK;
        case GLFW_KEY_L: return kEmexKeyPhysL;
        case GLFW_KEY_M: return kEmexKeyPhysM;
        case GLFW_KEY_N: return kEmexKeyPhysN;
        case GLFW_KEY_O: return kEmexKeyPhysO;
        case GLFW_KEY_P: return kEmexKeyPhysP;
        case GLFW_KEY_Q: return kEmexKeyPhysQ;
        case GLFW_KEY_R: return kEmexKeyPhysR;
        case GLFW_KEY_S: return kEmexKeyPhysS;
        case GLFW_KEY_T: return kEmexKeyPhysT;
        case GLFW_KEY_U: return kEmexKeyPhysU;
        case GLFW_KEY_V: return kEmexKeyPhysV;
        case GLFW_KEY_W: return kEmexKeyPhysW;
        case GLFW_KEY_X: return kEmexKeyPhysX;
        case GLFW_KEY_Y: return kEmexKeyPhysY;
        case GLFW_KEY_Z: return kEmexKeyPhysZ;

        case GLFW_KEY_GRAVE_ACCENT: return kEmexKeyPhysGrave;
        case GLFW_KEY_WORLD_1: return kEmexKeyPhysNonUSBackslash;
        case GLFW_KEY_1: return kEmexKeyPhys1;
        case GLFW_KEY_2: return kEmexKeyPhys2;
        case GLFW_KEY_3: return kEmexKeyPhys3;
        case GLFW_KEY_4: return kEmexKeyPhys4;
        case GLFW_KEY_5: return kEmexKeyPhys5;
        case GLFW_KEY_6: return kEmexKeyPhys6;
        case GLFW_KEY_7: return kEmexKeyPhys7;
        case GLFW_KEY_8: return kEmexKeyPhys8;
        case GLFW_KEY_9: return kEmexKeyPhys9;
        case GLFW_KEY_0: return kEmexKeyPhys0;
        case GLFW_KEY_MINUS: return kEmexKeyPhysMinus;
        case GLFW_KEY_EQUAL: return kEmexKeyPhysEqual;
        case GLFW_KEY_BACKSPACE: return kEmexKeyPhysBackspace;

        case GLFW_KEY_TAB: return kEmexKeyPhysTab;
        case GLFW_KEY_LEFT_BRACKET: return kEmexKeyPhysLeftBracket;
        case GLFW_KEY_RIGHT_BRACKET: return kEmexKeyPhysRightBracket;
        case GLFW_KEY_BACKSLASH: return kEmexKeyPhysBackslash;
        case GLFW_KEY_SEMICOLON: return kEmexKeyPhysSemicolon;
        case GLFW_KEY_APOSTROPHE: return kEmexKeyPhysQuote;
        case GLFW_KEY_ENTER: return kEmexKeyPhysEnter;
        case GLFW_KEY_COMMA: return kEmexKeyPhysComma;
        case GLFW_KEY_PERIOD: return kEmexKeyPhysPeriod;
        case GLFW_KEY_SLASH: return kEmexKeyPhysSlash;

        case GLFW_KEY_LEFT_SHIFT: return kEmexKeyPhysLeftShift;
        case GLFW_KEY_RIGHT_SHIFT: return kEmexKeyPhysRightShift;
        case GLFW_KEY_LEFT_CONTROL: return kEmexKeyPhysLeftCtrl;
        case GLFW_KEY_RIGHT_CONTROL: return kEmexKeyPhysRightCtrl;
        case GLFW_KEY_LEFT_ALT: return kEmexKeyPhysLeftAlt;
        case GLFW_KEY_RIGHT_ALT: return kEmexKeyPhysRightAlt;
        case GLFW_KEY_LEFT_SUPER: return kEmexKeyPhysLeftGUI;
        case GLFW_KEY_RIGHT_SUPER: return kEmexKeyPhysRightGUI;
        case GLFW_KEY_MENU: return kEmexKeyPhysMenu;
        case GLFW_KEY_SPACE: return kEmexKeyPhysSpace;

        case GLFW_KEY_ESCAPE: return kEmexKeyPhysEsc;
        case GLFW_KEY_F1: return kEmexKeyPhysF1;
        case GLFW_KEY_F2: return kEmexKeyPhysF2;
        case GLFW_KEY_F3: return kEmexKeyPhysF3;
        case GLFW_KEY_F4: return kEmexKeyPhysF4;
        case GLFW_KEY_F5: return kEmexKeyPhysF5;
        case GLFW_KEY_F6: return kEmexKeyPhysF6;
        case GLFW_KEY_F7: return kEmexKeyPhysF7;
        case GLFW_KEY_F8: return kEmexKeyPhysF8;
        case GLFW_KEY_F9: return kEmexKeyPhysF9;
        case GLFW_KEY_F10: return kEmexKeyPhysF10;
        case GLFW_KEY_F11: return kEmexKeyPhysF11;
        case GLFW_KEY_F12: return kEmexKeyPhysF12;

        case GLFW_KEY_CAPS_LOCK: return kEmexKeyPhysCapsLock;
        case GLFW_KEY_NUM_LOCK: return kEmexKeyPhysNumLock;
        case GLFW_KEY_SCROLL_LOCK: return kEmexKeyPhysScrollLock;

        case GLFW_KEY_INSERT: return kEmexKeyPhysInsert;
        case GLFW_KEY_DELETE: return kEmexKeyPhysDelete;
        case GLFW_KEY_HOME: return kEmexKeyPhysHome;
        case GLFW_KEY_END: return kEmexKeyPhysEnd;
        case GLFW_KEY_PAGE_UP: return kEmexKeyPhysPageUp;
        case GLFW_KEY_PAGE_DOWN: return kEmexKeyPhysPageDown;
        case GLFW_KEY_UP: return kEmexKeyPhysArrowUp;
        case GLFW_KEY_DOWN: return kEmexKeyPhysArrowDown;
        case GLFW_KEY_LEFT: return kEmexKeyPhysArrowLeft;
        case GLFW_KEY_RIGHT: return kEmexKeyPhysArrowRight;

        case GLFW_KEY_KP_DIVIDE: return kEmexKeyPhysNumpadDivide;
        case GLFW_KEY_KP_MULTIPLY: return kEmexKeyPhysNumpadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return kEmexKeyPhysNumpadMinus;
        case GLFW_KEY_KP_ADD: return kEmexKeyPhysNumpadPlus;
        case GLFW_KEY_KP_ENTER: return kEmexKeyPhysNumpadEnter;
        case GLFW_KEY_KP_0: return kEmexKeyPhysNumpad0;
        case GLFW_KEY_KP_1: return kEmexKeyPhysNumpad1;
        case GLFW_KEY_KP_2: return kEmexKeyPhysNumpad2;
        case GLFW_KEY_KP_3: return kEmexKeyPhysNumpad3;
        case GLFW_KEY_KP_4: return kEmexKeyPhysNumpad4;
        case GLFW_KEY_KP_5: return kEmexKeyPhysNumpad5;
        case GLFW_KEY_KP_6: return kEmexKeyPhysNumpad6;
        case GLFW_KEY_KP_7: return kEmexKeyPhysNumpad7;
        case GLFW_KEY_KP_8: return kEmexKeyPhysNumpad8;
        case GLFW_KEY_KP_9: return kEmexKeyPhysNumpad9;
        case GLFW_KEY_KP_DECIMAL: return kEmexKeyPhysNumpadDot;

        case GLFW_KEY_PRINT_SCREEN: return kEmexKeyPhysPrintScreen;
        case GLFW_KEY_PAUSE: return kEmexKeyPhysPause;

        default: return kEmexKeyPhysUnknown;
    }
}

void uart_restore_mode(void);

static void key_callback(GLFWwindow* window,
                         SInt32 key,
                         SInt32 scancode,
                         SInt32 action,
                         SInt32 mods)
{
    emex64_display_t *display = (emex64_display_t*)glfwGetWindowUserPointer(window);
    if(!display || !display->emex8042)
    {
        return;
    }

    kEmexKeyPhys phys_key = glfw_key_to_kEmexKeyPhys(key);
    if(phys_key == kEmexKeyPhysUnknown)
    {
        return;
    }

    if(action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        emex64_8042_send_keyboard_make(display->emex8042, phys_key);
    }
    else if(action == GLFW_RELEASE)
    {
        emex64_8042_send_keyboard_break(display->emex8042, phys_key);
    }
}

static void display_close_callback(GLFWwindow* window)
{
    /* TODO: shutdown the VM instead */
    uart_restore_mode();
    exit(0);
}

static void maximize_callback(GLFWwindow* window,
                              SInt32 maximized)
{
    if(maximized)
    {
        glfwRestoreWindow(window);
    }
}

static void die(const char* msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

static GLuint compileShader(GLenum type,
                            const char* src)
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
        char *log = (char *)malloc(log_size);
        glGetShaderInfoLog(s, log_size, NULL, log);
        fprintf(stderr, "Shader compile failed:\n%s\n", log);
        exit(1);
    }
    return s;
}

static GLuint linkProgram(GLuint vs,
                          GLuint fs)
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
        char *log = (char *)malloc(log_size);
        glGetProgramInfoLog(p, log_size, NULL, log);
        fprintf(stderr, "Program link failed:\n%s\n", log);
        exit(1);
    }
    return p;
}

static void fit_viewport(SInt32 ww,
                         SInt32 wh,
                         UInt32 w,
                         UInt32 h)
{
    if(w == 0 || h == 0 || ww <= 0 || wh <= 0)
    {
        glViewport(0, 0, ww, wh);
        return;
    }
    Float64 img = (Float64)w / (Float64)h;
    Float64 win = (Float64)ww / (Float64)wh;
    if(win > img)
    {
        SInt32 vw = (SInt32)(wh * img + 0.5);
        glViewport((ww - vw) / 2, 0, vw, wh);
    }
    else
    {
        SInt32 vh = (SInt32)(ww / img + 0.5);
        glViewport(0, (wh - vh) / 2, ww, vh);
    }
}

void *display_start(void *arg)
{
    emex64_display_t *display = (emex64_display_t*)arg;

    if(display == NULL)
    {
        return NULL;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);

    char buf[64];
    snprintf(buf, 64, "emex64vm %d x %d @ 60Hz", display->width, display->height);

    GLFWwindow* win = glfwCreateWindow(display->width, display->height, buf, NULL, NULL);
    if(!win) die("glfwCreateWindow failed");
    glfwSetWindowCloseCallback(win, display_close_callback);
    glfwSetWindowUserPointer(win, display);
    glfwSetKeyCallback(win, key_callback);
    glfwSetWindowMaximizeCallback(win, maximize_callback);
    glfwSetWindowAspectRatio(win, display->width, display->height);
    glfwSetWindowSizeLimits(win, display->width, display->height, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    glewInit();
    while(glGetError() != GL_NO_ERROR) {}

    const char* vsSrc = R"GLSL(
        #version 330 core
        layout(location=0) in vec2 aPos;
        layout(location=1) in vec2 aUV;
        out vec2 vUV;
        void main()
        {
            vUV = aUV;
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )GLSL";

    const char* fsSrc = R"GLSL(
        #version 330 core
        in vec2 vUV;
        out vec4 FragColor;
        uniform sampler2D uImage;
        uniform sampler1D uPalette;
        uniform int uFormat;
        void main()
        {
            vec4 c = texture(uImage, vUV);
            if(uFormat == 1)
            {
                float idx = floor(c.r * 255.0 + 0.5);
                FragColor = vec4(texture(uPalette, (idx + 0.5) / 256.0).rgb, 1.0);
            }
            else
            {
                FragColor = vec4(c.rgb, 1.0);
            }
        }
    )GLSL";

    GLuint prog = linkProgram(
        compileShader(GL_VERTEX_SHADER, vsSrc),
        compileShader(GL_FRAGMENT_SHADER, fsSrc)
    );

    float verts[] = {
        -1.f,-1.f,  0.f,1.f,
        1.f,-1.f,  1.f,1.f,
        1.f, 1.f,  1.f,0.f,
        -1.f, 1.f,  0.f,0.f
    };

    UInt32 idxs[] = { 0,1,2, 2,3,0 };

    GLuint vao,vbo,ebo;
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    glGenBuffers(1,&ebo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(idxs),idxs,GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glBindVertexArray(0);

    GLuint texImage = 0, texPal;
    UInt32 texW = 0, texH = 0, texFormat = 0;

    glGenTextures(1,&texPal);
    glBindTexture(GL_TEXTURE_1D,texPal);
    glTexImage1D(GL_TEXTURE_1D,0,GL_RGB8,256,0,GL_RGB,GL_UNSIGNED_BYTE,NULL);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog,"uImage"),0);
    glUniform1i(glGetUniformLocation(prog,"uPalette"),1);
    GLint locFormat = glGetUniformLocation(prog,"uFormat");

    emex64_gpu_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.palette_gen = ~0U;
    UInt32 palGen = ~0U;

    while(!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        SInt32 ww,wh;
        glfwGetFramebufferSize(win,&ww,&wh);
        glViewport(0,0,ww,wh);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        emex64_gpu_t *gpu = (emex64_gpu_t *)display->gpu;
        if(gpu != NULL)
        {
            emex64_gpu_get_frame(gpu, &frame);
        }
        else
        {
            frame.active = false;
        }

        if(frame.active)
        {
            if(texImage == 0 || texW != frame.width || texH != frame.height || texFormat != frame.format)
            {
                if(texImage != 0)
                {
                    glDeleteTextures(1, &texImage);
                }
                glGenTextures(1, &texImage);
                glBindTexture(GL_TEXTURE_2D, texImage);
                glTexImage2D(GL_TEXTURE_2D, 0, frame.format == GPU_FORMAT_I8 ? GL_R8 : GL_RGBA8, frame.width, frame.height, 0, frame.format == GPU_FORMAT_I8 ? GL_RED : GL_BGRA, GL_UNSIGNED_BYTE, NULL);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
                texW = frame.width;
                texH = frame.height;
                texFormat = frame.format;
            }

            UInt32 bpp = frame.format == GPU_FORMAT_I8 ? 1 : 4;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texImage);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, frame.stride / bpp);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.width, frame.height, frame.format == GPU_FORMAT_I8 ? GL_RED : GL_BGRA, GL_UNSIGNED_BYTE, frame.pixels);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_1D, texPal);
            if(palGen != frame.palette_gen)
            {
                glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGB, GL_UNSIGNED_BYTE, frame.palette);
                palGen = frame.palette_gen;
            }

            fit_viewport(ww, wh, frame.width, frame.height);
            glUseProgram(prog);
            glUniform1i(locFormat, (GLint)frame.format);
            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);
        }

        glfwSwapBuffers(win);
        if(gpu != NULL)
        {
            emex64_gpu_vblank(gpu);
        }
    }

    glfwTerminate();
    return 0;
}

#endif /* __linux__ */

extern void *display_start(void *arg);

#if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
static void *display_vblank_thread(void *arg)
{
    emex64_display_t *display = (emex64_display_t *)arg;
    struct timespec tick = { 0, (long)(EMEX64_FB_TICK_DT * 1e9) };

    while(!display->stopping)
    {
        nanosleep(&tick, NULL);
        emex64_gpu_t *gpu = (emex64_gpu_t *)display->gpu;
        if(gpu != NULL && !display->stopping)
        {
            emex64_gpu_vblank(gpu);
        }
    }
    return NULL;
}
#endif /* EMEX64VM_DEVICE_DISPLAY */

emex64_display_t *emex64_display_alloc(E64MachineRef machine,
                                       Boolean install,
                                       UInt16 width,
                                       UInt16 height)
{
    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    if(install)
    {
        UInt16 max = ((__E64Machine)machine)->embedded ? 4095 : 1920;
        if(width > max || height > max)
        {
            diag_error(NULL, "display dimensions are too big");
            return NULL;
        }
        else if(width < 256 || height < 256)
        {
            diag_error(NULL, "display dimensions are too small");
            return NULL;
        }
    }
    #endif /* EMEX64VM_DEVICE_DISPLAY */

    emex64_display_t *display = calloc(1, sizeof(emex64_display_t));
    if(display == NULL)
    {
        return NULL;
    }

    display->width = width;
    display->height = height;
    display->emex8042 = machine->emex8042;

    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    display->enabled = install;
    display->embedded = ((__E64Machine)machine)->embedded || !EMEX64VM_DISPLAY_WINDOW;
    if(install && display->embedded)
    {
        pthread_create(&(display->pthread), NULL, display_vblank_thread, display);
        return display;
    }
    #if EMEX64VM_DISPLAY_WINDOW
    if(install)
    {
        #if defined(__linux__)
        if(!glfwInit())
        {
            diag_warn(NULL, "are we in a TTY? can't present window");
            display->enabled = false;
            return display;
        }
        #endif /* __linux__ */

        pthread_create(&(display->pthread), NULL, display_start, display);
    }
    #endif /* EMEX64VM_DISPLAY_WINDOW */
    #endif /* EMEX64VM_DEVICE_DISPLAY */

    return display;
}

void emex64_display_stop(emex64_display_t *display)
{
    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    if(display != NULL && display->embedded && display->enabled && !display->stopping)
    {
        display->stopping = true;
        pthread_join(display->pthread, NULL);
    }
    #endif /* EMEX64VM_DEVICE_DISPLAY */
}

void emex64_display_dealloc(emex64_display_t *display)
{
    /* null pointer check */
    if(display == NULL)
    {
        return;
    }

    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    if(display->embedded)
    {
        emex64_display_stop(display);
    }
    else if(display->enabled)
    {
        pthread_cancel(display->pthread);
    }
    #endif /* EMEX64VM_DEVICE_DISPLAY */

    free(display);
}
