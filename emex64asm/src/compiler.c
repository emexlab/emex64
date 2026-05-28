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
#include <emex64asm/compiler.h>
#include <emex64asm/code.h>
#include <emex64asm/label.h>
#include <emex64asm/emit.h>
#include <emex64asm/section.h>
#include <emex64asm/macro.h>
#include <emex64asm/diag.h>
#include <fcntl.h>
#include <unistd.h>

compiler_invocation_t *compiler_invocation_alloc(const char *output_path)
{
    /* open file */
    int fd = open(output_path, O_RDWR | O_CREAT | O_TRUNC, 0777);

    if(fd < 0)
    {
        return NULL;
    }

    compiler_invocation_t *ci = malloc(sizeof(compiler_invocation_t));

    if(ci == NULL)
    {
        close(fd);
        return NULL;
    }

    /* zero out invocation */
    bzero(ci, sizeof(compiler_invocation_t));

    ci->fdwalker = malloc(sizeof(fdwalker_t));

    if(ci->fdwalker == NULL)
    {
        free(ci);
        close(fd);
        return NULL;
    }

    fdwalker_init(ci->fdwalker, fd, BW_LITTLE_ENDIAN);
    fdwalker_seek(ci->fdwalker, 8, 0);

    ci->page_align = true;  /* default value */
    
    return ci;
}

void compiler_invocation_dealloc(compiler_invocation_t *ci)
{
    /* todo: this must be redone from scratch */
}