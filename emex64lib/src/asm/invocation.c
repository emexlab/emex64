/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
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
#include <stdint.h>

#include <emex64lib/support/diag.h>
#include <emex64lib/support/file.h>

#include <emex64lib/asm/invocation.h>
#include <emex64lib/asm/code.h>
#include <emex64lib/asm/label.h>
#include <emex64lib/asm/emit.h>
#include <emex64lib/asm/section.h>
#include <emex64lib/asm/macro.h>
#include <emex64lib/asm/elf.h>

assembler_invocation_t *assembler_invocation_alloc(assembler_options_t *options)
{
    /* apply warning_error local thread variable */
    warning_error = options->warning_error;

    assembler_invocation_t *inv = calloc(1, sizeof(assembler_invocation_t));
    if(inv == NULL)
    {
        return NULL;
    }

    inv->fdwalker = NULL;
    inv->data_section_start = UINT64_MAX;
    inv->data_section_end = UINT64_MAX;
    inv->bss_section_start = UINT64_MAX;
    inv->bss_section_size = 0;

    inv->options = options;

    return inv;
}

void assembler_invocation_dealloc(assembler_invocation_t *inv)
{
    /* options have to be freed by who allocated them */

    for(size_t i = 0; i < inv->file_cnt; i++)
    {
        free(inv->file[i]);
    }
    free(inv->file);

    for(uint64_t i = 0; i < inv->line_cnt; i++)
    {
        free(inv->line[i]->str);
        for(uint64_t j = 0; j < inv->line[i]->token_cnt; j++)
        {
            free(inv->line[i]->token[j]->str);
        }
        free(inv->line[i]->token);
    }
    free(inv->line);

    for(uint64_t i = 0; i < inv->label_cnt; i++)
    {
        free(inv->label[i].name);
    }
    free(inv->label);

    /* definitions and include directories have to be released by who allocated them */

    reloc_table_entry_t *rtbe = inv->rtbe;
    while(rtbe != NULL)
    {
        free(rtbe->name);
        reloc_table_entry_t *next = rtbe->next;
        free(rtbe);
        rtbe = next;
    }

    fdwalker_dealloc(inv->fdwalker);
    free(inv);
}

bool assembler_invocation_emit(assembler_invocation_t *inv,
                               emex_file_t *input,
                               emex_file_t *output)
{
    /* need input */
    if(input == NULL)
    {
        diag_error(NULL, "no input file provided\n");
        return false;
    }

    /* need output */
    inv->fdwalker = emex_file_dup_fdwalker(output, BW_LITTLE_ENDIAN);
    if(inv->fdwalker == NULL)
    {
        diag_fatal(NULL, "couldn't allocate fdwalker\n");
        return false;
    }
    fdwalker_seek(inv->fdwalker, 10, 0);

    if(!assembler_code_preparse(inv, input) ||
       !assembler_macro_expand(inv) ||
       !assembler_code_parse(inv) ||
       !assembler_label_prealloc(inv) ||
       !assembler_section_parse(inv) ||
       !assembler_emit(inv) ||
       !assembler_elf_emit(inv))
    {
        unlink(output->path);
        return false;
    }
    
    return true;
}
