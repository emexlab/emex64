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
#include <fcntl.h>
#include <unistd.h>

#include <emex64lib/support/diag.h>

#include <emex64lib/asm/invocation.h>
#include <emex64lib/asm/code.h>
#include <emex64lib/asm/label.h>
#include <emex64lib/asm/emit.h>
#include <emex64lib/asm/section.h>
#include <emex64lib/asm/macro.h>

assembler_invocation_t *assembler_invocation_alloc(const char *output_path)
{
    /* open file */
    int fd = open(output_path, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if(fd < 0)
    {
        return NULL;
    }

    assembler_invocation_t *inv = calloc(1, sizeof(assembler_invocation_t));
    if(inv == NULL)
    {
        close(fd);
        return NULL;
    }

    /* zero out invocation */
    inv->fdwalker = malloc(sizeof(fdwalker_t));
    if(inv->fdwalker == NULL)
    {
        free(inv);
        close(fd);
        return NULL;
    }

    fdwalker_init(inv->fdwalker, fd, BW_LITTLE_ENDIAN);
    fdwalker_seek(inv->fdwalker, 8, 0);

    /* setting default values */
    inv->options = assembler_options_default();
    
    return inv;
}

assembler_invocation_t *assembler_invocation_alloc_with_options(const char *output_path,
                                                                assembler_options_t options)
{
    assembler_invocation_t *inv = assembler_invocation_alloc(output_path);
    if(inv == NULL)
    {
        return NULL;
    }

    inv->options = options;

    return inv;
}

void assembler_invocation_dealloc(assembler_invocation_t *inv)
{
    /* todo: this must be redone from scratch */
    diag_warn(NULL, "deallocation of assembler invocation is not implemented in this version of emex64lib\n");
}

bool assembler_invocation_emit(assembler_invocation_t *inv,
                               int filec,
                               char **filev)
{
    if(filec <= 0)
    {
        diag_error(NULL, "no input files provided\n");
        return false;
    }

    /* generating tokens,labels,sections out of the code */
    if(!assembler_code_parse(inv, (const char **)filev, filec) ||
       !assembler_label_prealloc(inv) ||
       !assembler_section_parse(inv) ||
       !assembler_macro_expand(inv))
    {
        return false;
    }

    /* finally compiling it to machine code */
    if(!assembler_emit(inv))
    {
        return false;
    }

    /* insert entry */
    if(!assembler_label_insert_start_entry(inv))
    {
        return false;
    }

    return true;
}
