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

#ifndef EMEX64ASM_EXPR_H
#define EMEX64ASM_EXPR_H

#include <stdint.h>
#include <stdbool.h>
#include <emex64lib/asm/type.h>

typedef struct {
    assembler_token_t **tok;
    uint64_t count;
    uint64_t pos;
    bool error;
    assembler_token_t *blame;
    const char *why;
} assembler_expr_t;

assembler_token_t *expr_peek(assembler_expr_t *e);
int64_t expr_primary(assembler_expr_t *e);
int64_t expr_term(assembler_expr_t *e);
int64_t expr_addsub(assembler_expr_t *e);
bool assembler_eval_const(assembler_token_t **tok, uint64_t count, int64_t *out);

#endif /* EMEX64ASM_EXPR_H */
