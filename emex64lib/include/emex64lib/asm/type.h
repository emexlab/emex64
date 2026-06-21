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

#ifndef EMEX64ASM_TYPE_H
#define EMEX64ASM_TYPE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <emex64lib/support/virtual/vbitwalker.h>

#include <emex64lib/asm/lexer.h>

enum kAssemblerLineType: uint8_t {
    kAssemblerLineTypeNone,
    kAssemblerLineTypeIgnore,
    kAssemblerLineTypeAssembly,
    kAssemblerLineTypeExternLabel,
    kAssemblerLineTypeGlobalLabel,
    kAssemblerLineTypeLocalLabel,
    kAssemblerLineTypeSection,
    kAssemblerLineTypeSectionData,
    kAssemblerLineTypeDefinitionDirective,
    kAssemblerLineTypeConditionDirective,
    kAssemblerLineTypeIncludeDirective,     /* TODO: the preprocessor shall include */
};

typedef struct assembler_token {
    char *str;
    size_t column_num;                      /* start offset of column */
    struct assembler_line *al;              /* pointer back to compiler line */
    enum kAssemblerTokenType type;          /* OMG THAT IS AI?!?! the token type, WOAHHH AM I A AI, DID A AI GENERATE THIS TOKEN?!??!*/
} assembler_token_t;

typedef struct assembler_line {
    char *str;
    enum kAssemblerLineType type;           /* type of line */
    struct assembler_token **token;         /* subtokens */
    uint64_t token_cnt;                     /* count of subtokens */
    size_t line_num;                        /* line number in file */   
    size_t file_idx;                        /* index of file in compiler invocation */
    struct assembler_invocation *inv;       /* pointer back to compiler invocation */
} assembler_line_t;

typedef struct {
    char *name;                             /* name of resolved label */
    bool defined;                           /* label definitions are defined */
    uint64_t addr;                          /* address of resolved label */
    struct assembler_token *at_link;        /* link to the originator of the label */
} assembler_label_t;

typedef struct reloc_table_entry {
    char *name;                             /* resolved label name */
    bool local;                             /* must be resolved at assemble time */
    size_t byte_pos;                        /* position */
    struct assembler_token *at_link;        /* link to the originator of the entry */
    struct reloc_table_entry *next;         /* pointer to next entry */
} reloc_table_entry_t;

#endif /* EMEX64ASM_TYPE_H */
