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

#ifndef EMEX64ASM_INVOCATION_H
#define EMEX64ASM_INVOCATION_H

#include <stdbool.h>

#include <emex64lib/support/file.h>

#include <emex64lib/asm/type.h>
#include <emex64lib/asm/options.h>

typedef struct assembler_invocation {
    emex_file_t **file;                     /* code files */
    size_t file_cnt;                        /* count of files */

    assembler_line_t *line;                 /* code lines */
    uint64_t line_cnt;                      /* count of lines */

    char *label_scope;                      /* current resolved label scope (the global label used for the local ones) */
    assembler_label_t *label;               /* label array */

    uint64_t label_cnt;                     /* count of labels */
    reloc_table_entry_t *rtbe;              /* relocation table root entry */
    fdwalker_t *fdwalker;

    assembler_options_t options;
} assembler_invocation_t;

assembler_invocation_t *assembler_invocation_alloc(const char *output_path);
assembler_invocation_t *assembler_invocation_alloc_with_options(const char *output_path, assembler_options_t options);
void assembler_invocation_dealloc(assembler_invocation_t *inv);

bool assembler_invocation_emit(assembler_invocation_t *inv, int filec, char **filev);

#endif /* EMEX64ASM_INVOCATION_H */
