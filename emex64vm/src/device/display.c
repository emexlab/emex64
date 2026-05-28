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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#include <emexutils/bitwalker.h>

#include <emex64vm/machine.h>
#include <emex64vm/device/display.h>

#if defined(__linux__)

#include <GL/glew.h>
#include <GLFW/glfw3.h>

static void die(const char* msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
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
    la64_display_t *display = (la64_display_t*)arg;

    if(display == NULL)
    {
        return NULL;
    }

    if(!glfwInit()) die("glfwInit failed");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(800, 800, "LA64LCD @ 60Hz", NULL, NULL);
    if(!win) die("glfwCreateWindow failed");
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
    glTexStorage2D(GL_TEXTURE_2D,1,GL_R8,LA64_FB_WIDTH,LA64_FB_HEIGHT);
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
        glBufferData(GL_PIXEL_UNPACK_BUFFER,LA64_FB_SIZE,NULL,GL_STREAM_DRAW);
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);

    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog,"uIndexTex"),0);
    glUniform1i(glGetUniformLocation(prog,"uPalette"),1);

    double prev = now_sec();
    double acc  = 0.0;
    int pboIdx = 0;

    while(!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        double now = now_sec();
        acc += now - prev;
        prev = now;

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbo[pboIdx]);
        uint8_t* ptr = (uint8_t*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,0,LA64_FB_SIZE, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
        memcpy(ptr, display->fb, LA64_FB_SIZE);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

        glBindTexture(GL_TEXTURE_2D,texIndex);
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,LA64_FB_WIDTH,LA64_FB_HEIGHT,GL_RED,GL_UNSIGNED_BYTE,NULL);
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

la64_display_t *la64_display_alloc(la64_machine_t *machine)
{
    la64_display_t *display = malloc(sizeof(la64_display_t));

    /* null pointer check */
    if(display == NULL)
    {
        return NULL;
    }

    if(!la64_mmio_register(machine->mmio_bus, LA64_FB_BASE, LA64_FB_SIZE, display, la64_fb_read, la64_fb_write))
    {
        free(display);
        return NULL;
    }

    /* allocate palette */
    display->palette = calloc(3, 256);

    /* null pointer check */
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

    display->fb = calloc(1, LA64_FB_SIZE);

    /* null pointer check */
    if(display->fb == NULL)
    {
        free(display->palette);
        free(display);
        return NULL;
    }

    return display;
}

void la64_display_dealloc(la64_display_t *display)
{
    /* null pointer check */
    if(display == NULL)
    {
        return;
    }

    if(display->enabled)
    {
        pthread_cancel(display->pthread);
    }

    if(display->palette != NULL)
    {
        free(display->palette);
    }

    if(display->fb != NULL)
    {
        free(display->fb);
    }
}

uint64_t la64_fb_read(la64_core_t *core,
                      void *device,
                      uint64_t offset,
                      int size)
{
    la64_display_t *display = (la64_display_t*)device;

    if(offset >= LA64_FB_FRAMEBUFFER)
    {
        offset -= LA64_FB_FRAMEBUFFER;

        bitwalker_t bw;
        bitwalker_init_read(&bw, &(display->palette[offset]), size, BW_LITTLE_ENDIAN);
        return bitwalker_read(&bw, size * 8);
    }
    else if(offset >= LA64_FB_PALLETE)
    {
        offset -= LA64_FB_PALLETE;

        bitwalker_t bw;
        bitwalker_init_read(&bw, &(display->palette[offset]), size, BW_LITTLE_ENDIAN);
        return bitwalker_read(&bw, size * 8);
    }
    else
    {
        return display->enabled;
    }
}

void la64_fb_write(la64_core_t *core,
                   void *device,
                   uint64_t offset,
                   uint64_t value,
                   int size)
{
    la64_display_t *display = (la64_display_t*)device;

    if(offset >= LA64_FB_FRAMEBUFFER)
    {
        offset -= LA64_FB_FRAMEBUFFER;

        bitwalker_t bw;
        bitwalker_init(&bw, &(display->fb[offset]), size, BW_LITTLE_ENDIAN);
        bitwalker_write(&bw, value, size * 8);
        return;
    }
    else if(offset >= LA64_FB_PALLETE)
    {
        offset -= LA64_FB_PALLETE;

        bitwalker_t bw;
        bitwalker_init(&bw, &(display->palette[offset]), size, BW_LITTLE_ENDIAN);
        bitwalker_write(&bw, value, size * 8);
        return;
    }
    else
    {
        if((uint8_t)value)
        {
            pthread_create(&(display->pthread), NULL, display_start, device);
        }
        else
        {
            pthread_cancel(display->pthread);
        }

        display->enabled = (uint8_t)value;
    }
}
