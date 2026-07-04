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
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>
#include <EmexToolchain/support/diagnostic/log.h>
#include <EmexToolchain/vm/E64Machine.h>
#include <EmexToolchain/vm/device/board/display.h>

#if defined(__linux__) && EMEX64VM_DEVICE_DISPLAY

#include <GL/glew.h>
#include <GLFW/glfw3.h>

kEmexKeyPhys glfw_key_to_kEmexKeyPhys(int key)
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

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
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

static void maximize_callback(GLFWwindow* window, int maximized)
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
        char *log = (char *)malloc(log_size);
        glGetShaderInfoLog(s, log_size, NULL, log);
        fprintf(stderr, "Shader compile failed:\n%s\n", log);
        exit(1);
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
        char *log = (char *)malloc(log_size);
        glGetProgramInfoLog(p, log_size, NULL, log);
        fprintf(stderr, "Program link failed:\n%s\n", log);
        exit(1);
    }
    return p;
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
        uniform sampler2D uIndexTex;
        uniform sampler1D uPalette;
        void main()
        {
            float idxN = texture(uIndexTex, vUV).r;
            float idx  = floor(idxN * 255.0 + 0.5);
            float t    = (idx + 0.5) / 256.0;
            vec3 rgb   = texture(uPalette, t).rgb;
            FragColor  = vec4(rgb, 1.0);
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

    uint32_t idxs[] = { 0,1,2, 2,3,0 };

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

    GLuint texIndex, texPal;
    glGenTextures(1,&texIndex);
    glBindTexture(GL_TEXTURE_2D,texIndex);
    glTexStorage2D(GL_TEXTURE_2D,1,GL_R8,display->width,display->height);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    glGenTextures(1,&texPal);
    glBindTexture(GL_TEXTURE_1D,texPal);
    glTexImage1D(GL_TEXTURE_1D,0,GL_RGB8,256,0,GL_RGB,GL_UNSIGNED_BYTE,display->palette);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    GLuint pbo[2];
    glGenBuffers(2,pbo);

    for(int i=0;i<2;i++)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbo[i]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER,display->fb_size,NULL,GL_STREAM_DRAW);
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);

    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog,"uIndexTex"),0);
    glUniform1i(glGetUniformLocation(prog,"uPalette"),1);

    int pboIdx = 0;

    while(!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbo[pboIdx]);
        uint8_t* ptr = (uint8_t*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,0,display->fb_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        memcpy(ptr, display->fb, display->fb_size);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

        glBindTexture(GL_TEXTURE_2D,texIndex);
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,display->width,display->height,GL_RED,GL_UNSIGNED_BYTE,NULL);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
        pboIdx ^= 1;

        int ww,wh;
        glfwGetFramebufferSize(win,&ww,&wh);
        glViewport(0,0,ww,wh);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,texIndex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_1D,texPal);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_INT,0);

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}

#endif /* __linux__ */

extern void *display_start(void *arg);

emex64_display_t *emex64_display_alloc(E64MachineRef machine,
                                       bool install,
                                       uint16_t width,
                                       uint16_t height)
{
    if(width > 1920 || height > 1920)
    {
        diag_error(NULL, "display dimensions are too big\n");
        return NULL;
    }
    else if(width < 256 || height < 256)
    {
        diag_error(NULL, "display dimensions are too small\n");
        return NULL;
    }

    emex64_display_t *display = malloc(sizeof(emex64_display_t));
    if(display == NULL)
    {
        return NULL;
    }

    display->width = width;
    display->height = height;
    display->fb_size = width * height;

    E64MMIORegionRef FBRegion = E64MMIORegionCreate(kEFAllocatorDefault, EMEX64_FB_BASE, EMEX64_FB_FRAMEBUFFER + display->fb_size, display, emex64_fb_read, emex64_fb_write);
    if(FBRegion == NULL)
    {
        free(display);
        return NULL;
    }

    bool success = E64MMIOBusRegisterRegion(machine->mmio_bus, FBRegion);
    EFRelease(FBRegion);
    if(!success)
    {
        free(display);
        return NULL;
    }

    /* allocate palette */
    display->palette = calloc(3, 256);
    if(display->palette == NULL)
    {
        free(display);
        return NULL;
    }

    /* setting up by default with grayscale */
    for (int i = 0; i < 256; i++)
    {
        uint8_t gray = (uint8_t)i;

        display->palette[i*3 + 0] = gray;
        display->palette[i*3 + 1] = gray;
        display->palette[i*3 + 2] = gray;
    }

    display->fb = calloc(1, display->fb_size);
    if(display->fb == NULL)
    {
        free(display->palette);
        free(display);
        return NULL;
    }

    display->emex8042 = machine->emex8042;

    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    display->enabled = install;
    if(install)
    {
        #if defined(__linux__)
        if(!glfwInit())
        {
            diag_warn(NULL, "are we in a TTY? can't present window\n");
            display->enabled = false;
            return display;
        }
        #endif /* __linux__ */

        pthread_create(&(display->pthread), NULL, display_start, display);
    }
    #endif /* EMEX64VM_DEVICE_DISPLAY */

    return display;
}

void emex64_display_dealloc(emex64_display_t *display)
{
    /* null pointer check */
    if(display == NULL)
    {
        return;
    }

    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    if(display->enabled)
    {
        pthread_cancel(display->pthread);
    }
    #endif /* EMEX64VM_DEVICE_DISPLAY */

    if(display->palette != NULL)
    {
        free(display->palette);
    }

    if(display->fb != NULL)
    {
        free(display->fb);
    }
}

uint64_t emex64_fb_read(emex64_core_t *core,
                      void *device,
                      uint64_t offset,
                      int size)
{

    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    emex64_display_t *display = (emex64_display_t*)device;
    if(display->enabled)
    {
        if(offset >= EMEX64_FB_FRAMEBUFFER)
        {
            uint64_t outvalue;
            EMEX64_MEMORY_READ_HELPER(display->fb, offset - EMEX64_FB_FRAMEBUFFER, size, outvalue);
            return outvalue;
        }
        else if(offset == EMEX64_FB_REG_HEIGHT)
        {
            return display->height;
        }
        else if(offset == EMEX64_FB_REG_WIDTH)
        {
            return display->width;
        }
        else
        {
            return display->enabled;
        }
    }
    else
    {
        return 0;
    }
    #else
    return 0;
    #endif /* EMEX64VM_DEVICE_DISPLAY */
}

void emex64_fb_write(emex64_core_t *core,
                   void *device,
                   uint64_t offset,
                   uint64_t value,
                   int size)
{
    #if EMEX64VM_DEVICE_DISPLAY && (defined(__linux__) || defined(__APPLE__))
    emex64_display_t *display = (emex64_display_t*)device;

    if(display->enabled)
    {
        if(offset >= EMEX64_FB_FRAMEBUFFER)
        {
            EMEX64_MEMORY_WRITE_HELPER(display->fb, offset - EMEX64_FB_FRAMEBUFFER, size, value);
            return;
        }
    }
    #endif /* EMEX64VM_DEVICE_DISPLAY */
}
