/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EMEX64ASM_EXPR_H
#define EMEX64ASM_EXPR_H

#include <stdint.h>
#include <stdbool.h>
#include <EmexToolchain/asm/type.h>

typedef struct {
    assembler_token_t **tok;
    UInt64 count;
    UInt64 pos;
    Boolean error;
    assembler_token_t *blame;
    const char *why;
} assembler_expr_t;

assembler_token_t *expr_peek(assembler_expr_t *e);
int64_t expr_primary(assembler_expr_t *e);
int64_t expr_term(assembler_expr_t *e);
int64_t expr_addsub(assembler_expr_t *e);
Boolean assembler_eval_const(assembler_token_t **tok, UInt64 count, int64_t *out);

#endif /* EMEX64ASM_EXPR_H */
