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
#include <emex64asm/label.h>
#include <emex64asm/diag.h>
#include <unistd.h>

void code_token_label(compiler_invocation_t *ci)
{
    /* counting labels caught at token parsing */
    ci->label_cnt = 1;
    for(int i = 0; i < ci->line_cnt; i++)
    {
        if(ci->line[i].type == COMPILER_LINE_TYPE_GLOBAL_LABEL ||
           ci->line[i].type == COMPILER_LINE_TYPE_LOCAL_LABEL ||
           ci->line[i].type == COMPILER_LINE_TYPE_SECTION_DATA)
        {
            (ci->label_cnt)++;
        }
    }

    /* allocating memory for those */
    ci->label = calloc(ci->label_cnt, sizeof(compiler_label_t));

    /* reset label count for compiler */
    ci->label_cnt = 0;
}

compiler_label_t *label_lookup_internal(compiler_invocation_t *ci,
                                        const char *name)
{
    /* iterating through all labels */
    for(int i = 0; i < ci->label_cnt; i++)
    {
        /* checking if request name matches */
        if(strcmp(ci->label[i].name, name) == 0)
        {
            /* returning label address*/
            return &(ci->label[i]);
        }
    }

    return NULL;
}

void code_token_label_append(compiler_token_t *ct)
{
    /* accessing compiler line and invocation */
    compiler_line_t *cl = ct->cl;
    compiler_invocation_t *ci = cl->ci;

    /* assign address to label */
    ci->label[ci->label_cnt].addr = fdwalker_bytes_used(ct->cl->ci->fdwalker);

    /* copying label name */
    size_t size = strlen(ct->str);
    char *name = strdup(ct->str);
    name[size - 1] = '\0';

    /* checking if its in scope */
    if(ct->cl->type == COMPILER_LINE_TYPE_LOCAL_LABEL)
    {
        /* null poiner checking scope */
        if(ci->label_scope == NULL)
        {
            diag_error(ct, "defining a local label out of any global label is illegal \"%s\"\n", name);
        }

        /* adjust size */
        size += strlen(ci->label_scope);

        /* reallocate buffer */
        name = realloc(name, (size) + 1);

        /* recopy */
        sprintf(name, "%s%s", ci->label_scope, cl->str);
        name[size - 1] = '\0';
    }
    else
    {
        /* set it as scope */
        ci->label_scope = name;
    }

    /* checking for duplicated labels */
    compiler_label_t *label = label_lookup_internal(ci, name);

    /* here we go */
    if(label != NULL)
    {
        diag_note(label->ctlink, "label \"%s\" already defined here\n", name);
        diag_error(ct, "duplicated label \"%s\"\n", name);
    }

    ci->label[ci->label_cnt].ctlink = ct;
    ci->label[ci->label_cnt++].name = name;
}

uint64_t label_lookup(compiler_invocation_t *ci,
                      const char *name)
{
    /* using internal implementation of the symbol */
    compiler_label_t *label = label_lookup_internal(ci, name);
    return (label == NULL) ? COMPILER_LABEL_NOT_FOUND : label->addr;
}

void code_token_label_insert_start(compiler_invocation_t *ci)
{
    /* finding start label */
    uint64_t addr = label_lookup(ci, ci->start_entry_name);

    if(addr == COMPILER_LABEL_NOT_FOUND)
    {
        diag_error(NULL, "\"%s\" label not found, cannot produce boot image\n", ci->start_entry_name);
    }

    /* writing start address into the start of the image */
    fdwalker_t fw = *(ci->fdwalker);
    fdwalker_seek(&fw, 0, 0);
    fdwalker_write(&fw, addr, 64);
}
