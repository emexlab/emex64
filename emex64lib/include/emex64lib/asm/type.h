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

#ifndef EMEX64ASM_TYPE_H
#define EMEX64ASM_TYPE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <emex64lib/support/fdwalker.h>

typedef enum assemblerLineType {
    kAssemblerLineTypeNone = 0b0000,
    kAssemblerLineTypeAssembly,
    kAssemblerLineTypeGlobalLabel,
    kAssemblerLineTypeLocalLabel,
    kAssemblerLineTypeSection,
    kAssemblerLineTypeSectionData,
    kAssemblerLineTypeMacroDef
} assembler_line_type_t;

typedef struct {
    char *str;
    size_t column_num;                      /* start offset of column */
    struct assembler_line *al;              /* pointer back to compiler line */
} compiler_token_t;

typedef struct assembler_line {
    char *str;
    assembler_line_type_t type;             /* type of line */
    compiler_token_t *token;                /* subtokens */
    uint64_t token_cnt;                     /* count of subtokens */
    size_t line_num;                        /* line number in file */   
    size_t file_idx;                        /* index of file in compiler invocation */
    struct assembler_invocation *inv;       /* pointer back to compiler invocation */
} assembler_line_t;

typedef struct {
    char *name;                             /* name of resolved label */
    uint64_t addr;                          /* address of resolved label */
    compiler_token_t *ctlink;               /* link to the originator of the label */
} compiler_label_t;

typedef struct reloc_table_entry reloc_table_entry_t;

struct reloc_table_entry {
    char *name;                             /* resolved label name */
    size_t byte_pos;                        /* position */
    uint8_t bit_idx;
    compiler_token_t *ctlink;               /* link to the originator of the entry */
    reloc_table_entry_t *next;              /* pointer to next entry */
};

#endif /* EMEX64ASM_TYPE_H */
