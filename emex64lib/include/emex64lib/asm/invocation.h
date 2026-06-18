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

#ifndef EMEX64ASM_INVOCATION_H
#define EMEX64ASM_INVOCATION_H

#include <stdbool.h>

#include <emex64lib/support/file.h>

#include <emex64lib/asm/type.h>
#include <emex64lib/asm/options.h>

typedef struct {
    char *match;
    char *value;
} assembler_macro_definition_t;

typedef struct assembler_invocation {
    assembler_options_t options;                /* borrowed */
    
    char **file;
    size_t file_cnt;

    assembler_line_t **line;
    uint64_t line_cnt;

    char *label_scope;
    assembler_label_t *label;
    uint64_t label_cnt;

    uint64_t definition_cnt;                    /* borrowed */
    assembler_macro_definition_t *definition;   /* borrowed */

    char **include_dirs;                        /* borrowed */
    size_t include_dir_cnt;                     /* borrowed */

    reloc_table_entry_t *rtbe;
    fdwalker_t *fdwalker;

    uint64_t data_section_start;
    uint64_t data_section_end;
    uint64_t bss_section_start;
    uint64_t bss_section_size;
} assembler_invocation_t;

assembler_invocation_t *assembler_invocation_alloc(assembler_options_t options);
void assembler_invocation_dealloc(assembler_invocation_t *inv);

bool assembler_invocation_emit(assembler_invocation_t *inv, emex_file_t *input, emex_file_t *output);

#endif /* EMEX64ASM_INVOCATION_H */
