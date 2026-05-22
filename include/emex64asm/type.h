/*
 * MIT License
 *
 * Copyright (c) 2024 cr4zyengineer
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

#ifndef LA64ASM_TYPE_H
#define LA64ASM_TYPE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <emexutils/fdwalker.h>

#define COMPILER_LINE_TYPE_NONE                 0b0000
#define COMPILER_LINE_TYPE_ASM                  0b0001
#define COMPILER_LINE_TYPE_GLOBAL_LABEL         0b0010
#define COMPILER_LINE_TYPE_LOCAL_LABEL          0b0011
#define COMPILER_LINE_TYPE_SECTION              0b0100
#define COMPILER_LINE_TYPE_SECTION_DATA         0b0101
#define COMPILER_LINE_TYPE_MACRODEF             0b0110

typedef unsigned char compiler_line_type_t;
typedef struct compiler_invocation compiler_invocation_t;
typedef struct compiler_line compiler_line_t;

typedef struct {
    char *str;
    size_t column_num;                      /* start offset of column */
    compiler_line_t *cl;                    /* pointer back to compiler line */
} compiler_token_t;

typedef struct compiler_line {
    char *str;
    compiler_line_type_t type;              /* type of line */
    compiler_token_t *token;                /* subtokens */
    uint64_t token_cnt;                     /* count of subtokens */
    size_t line_num;                        /* line number in file */   
    size_t file_idx;                        /* index of file in compiler invocation */
    compiler_invocation_t *ci;              /* pointer back to compiler invocation */
} compiler_line_t;

typedef struct {
    char *path;
    char *code;
    size_t len;
} compiler_file_t;

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

typedef struct compiler_invocation {
    compiler_file_t *file;                  /* code files */
    size_t file_cnt;                        /* count of files */
    compiler_line_t *line;                  /* token array */
    uint64_t line_cnt;                      /* count of tokens */
    char *label_scope;                      /* current resolved label scope */
    compiler_label_t *label;                /* label array */
    uint64_t label_cnt;                     /* count of labels */
    reloc_table_entry_t *rtbe;              /* relocation table root entry */
    fdwalker_t *fdwalker;

    /* options */
    bool page_align;                        /* default: true */
    const char *start_entry_name;           /* default: _start */
} compiler_invocation_t;

#endif /* LA64ASM_TYPE_H */
