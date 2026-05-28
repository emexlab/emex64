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

#include <emex64asm/macro.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    const char *name;
    const char *value;
} compiler_macro_t;

void code_token_macro(compiler_invocation_t *ci)
{
    /* count the amount of macros */
    uint64_t c = 0;
    for(uint64_t i = 0; i < ci->line_cnt; i++)
    {
        if(ci->line[i].type == COMPILER_LINE_TYPE_MACRODEF)
        {
            c++;
        }
    }

    /* allocating */
    compiler_macro_t *cm = calloc(c, sizeof(compiler_macro_t));

    /* adding stuff */
    c = 0;
    for(uint64_t i = 0; i < ci->line_cnt; i++)
    {
        if(ci->line[i].type == COMPILER_LINE_TYPE_MACRODEF)
        {
            cm[c].name = ci->line[i].token[1].str;
            cm[c].value = ci->line[i].token[2].str;
            c++;
        }
    }

    /* now replacing */
    for(uint64_t i = 0; i < ci->line_cnt; i++)
    {
        if(ci->line[i].type == COMPILER_LINE_TYPE_ASM)
        {
            for(uint64_t a = 0; a < ci->line[i].token_cnt; a++)
            {
                for(uint64_t b = 0; b < c; b++)
                {
                    if(strcmp(ci->line[i].token[a].str, cm[b].name) == 0)
                    {
                        free(ci->line[i].token[a].str);
                        ci->line[i].token[a].str = strdup(cm[b].value);
                    }
                }
            }
        }
    }
}