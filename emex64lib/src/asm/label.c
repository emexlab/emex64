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
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <emex64lib/asm/label.h>
#include <emex64lib/asm/diag.h>

void assembler_label_prealloc(assembler_invocation_t *inv)
{
    /*
     * counting labels caught at token parsing
     * the reason it starts at 1 is because of
     * a inbuilt relocation value called
     * __la64_exec_img_end which is appended in
     * the end of code emitting.
     */
    inv->label_cnt = 1;
    for(uint64_t i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i].type == ASSEMBLER_LINE_TYPE_GLOBAL_LABEL ||
           inv->line[i].type == ASSEMBLER_LINE_TYPE_LOCAL_LABEL ||
           inv->line[i].type == ASSEMBLER_LINE_TYPE_SECTION_DATA)
        {
            (inv->label_cnt)++;
        }
    }

    /* allocating memory for those */
    inv->label = calloc(inv->label_cnt, sizeof(compiler_label_t));
    inv->label_cnt = 0;
}

compiler_label_t *assembler_label_lookup(assembler_invocation_t *inv,
                                         const char *name)
{
    for(uint64_t i = 0; i < inv->label_cnt; i++)
    {
        if(strcmp(inv->label[i].name, name) == 0)
        {
            return &(inv->label[i]);
        }
    }

    return NULL;
}

void assembler_label_append(compiler_token_t *ct)
{
    /* accessing compiler line and invocation */
    compiler_line_t *cl = ct->cl;
    compiler_invocation_t *ci = cl->ci;

    /* assign address to label */
    ci->label[ci->label_cnt].addr = fdwalker_bytes_used(ci->fdwalker);

    char *name = NULL;

    /* copying label name */
    if(ct->cl->type == ASSEMBLER_LINE_TYPE_LOCAL_LABEL)
    {
        /* constructing scoped label */
        size_t label_scope_len = strlen(ci->label_scope);
        size_t ct_len = strlen(ct->str);
        size_t size = label_scope_len + ct_len;
        name = malloc(size);
        memcpy(name, ci->label_scope, label_scope_len);
        memcpy(name + label_scope_len, ct->str, ct_len - 1); /* minus 1 to ommit the ':' character */
        name[size - 1] = '\0';

        /* checking if we are in a scope */
        if(ci->label_scope == NULL)
        {
            diag_error(ct, "defining a local label out of any global label is illegal \"%s\"\n", name);
        }
    }
    else
    {
        /* constructing global label */
        size_t size = strlen(ct->str);
        name = malloc(size);
        memcpy(name, ct->str, size - 1);
        name[size - 1] = '\0';

        /* set it as scope */
        ci->label_scope = name;
    }

    /* checking for duplicated labels */
    compiler_label_t *label = assembler_label_lookup(ci, name);
    if(label != NULL)
    {
        diag_note(label->ctlink, "label \"%s\" already defined here\n", name);
        diag_error(ct, "duplicated label \"%s\"\n", name);
    }

    ci->label[ci->label_cnt].ctlink = ct;
    ci->label[ci->label_cnt++].name = name;
}

void assembler_label_insert_start_entry(assembler_invocation_t *inv)
{
    /* finding start label */
    compiler_label_t *label = assembler_label_lookup(inv, inv->options.start_entry_name);
    if(label == NULL)
    {
        diag_error(NULL, "\"%s\" label not found, cannot produce boot image\n", inv->options.start_entry_name);
    }

    /* writing start address into the start of the image */
    fdwalker_t fw = *(inv->fdwalker);
    fdwalker_seek(&fw, 0, 0);
    fdwalker_write(&fw, label->addr, 64);
}
