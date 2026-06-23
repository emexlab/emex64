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
#include <emex64lib/support/diagnostic/legacy.h>
#include <emex64lib/support/file.h>
#include <emex64lib/asm/preprocessor/preprocessor.h>
#include <emex64lib/asm/label/label.h>
#include <emex64lib/asm/emitter/emitter.h>
#include <emex64lib/asm/emitter/elf.h>
#include <emex64lib/asm/invocation.h>
#include <emex64lib/asm/code.h>
#include <emex64lib/asm/section.h>

assembler_invocation_t *assembler_invocation_alloc(assembler_invocation_options_t options)
{
    /* apply warning_error local thread variable */
    warning_error = options.warning_error;
    caret_diagnostics = options.caret_diagnostics;

    assembler_invocation_t *inv = calloc(1, sizeof(assembler_invocation_t));
    if(inv == NULL)
    {
        return NULL;
    }

    inv->options = options;
    inv->data_section_start = UINT64_MAX;
    inv->data_section_end = UINT64_MAX;
    inv->bss_section_start = UINT64_MAX;

    return inv;
}

void assembler_invocation_dealloc(assembler_invocation_t *inv)
{
    /* options have to be freed by who allocated them */

    /* skipping the tool managed input file object */
    for(size_t i = 1; i < inv->file_cnt; i++)
    {
        emex_file_dealloc(inv->file[i]);
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

    vbitwalker_dealloc(inv->out_vbitwalker);
    free(inv);
}

bool assembler_invocation_emit(assembler_invocation_t *inv,
                               emex_file_t *input,
                               emex_file_t *output)
{
    /* need output */
    inv->out_vbitwalker = emex_file_dup_vbitwalker(output, BW_LITTLE_ENDIAN);
    if(inv->out_vbitwalker == NULL)
    {
        diag_fatal(NULL, "couldn't allocate fdwalker\n");
        return false;
    }

    vbitwalker_seek(inv->out_vbitwalker, 10, 0);

    if(!assembler_code_preparse(inv, input) ||
       !assembler_preprocessor_run(inv) ||
       !assembler_code_postparse(inv) ||
       !assembler_label_prealloc(inv) ||
       !assembler_section_parse(inv) ||
       !assembler_emit(inv) ||
       !assembler_elf_emit(inv))
    {
        emex_file_unlink(output);
        return false;
    }
    
    return true;
}
