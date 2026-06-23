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

#ifndef EMEX64ASM_LABEL_LABEL_H
#define EMEX64ASM_LABEL_LABEL_H

#include <stdbool.h>
#include <emex64lib/asm/type.h>

typedef struct assembler_invocation assembler_invocation_t;

typedef struct {
    char *name;                             /* name of resolved label */
    bool defined;                           /* label definitions are defined */
    uint64_t addr;                          /* address of resolved label */
    assembler_token_t *at_link;             /* link to the originator of the label */
} assembler_label_t;

bool assembler_label_prealloc(assembler_invocation_t *inv);
bool assembler_label_append(assembler_token_t *at);

assembler_label_t *assembler_label_lookup(assembler_invocation_t *inv, const char *name);

#endif /* EMEX64ASM_LABEL_LABEL_H */
