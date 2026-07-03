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

#ifndef EMEX64ASM_LEXER_H
#define EMEX64ASM_LEXER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <emex64lib/asm/type.h>

#define LEXTOK_LENGHT_MAX   2048    /* if anyone comes close to that size, bro pls fix your variable naming style O.O */

typedef struct {
    const char *token;
    size_t column;
    kAssemblerTokenType type;
} lextok_token_t;

lextok_token_t assembler_lexer_tok(const char *token);
bool assembler_lexer_classify(assembler_token_t *at);
const char *assembler_lexer_str_for_token_type(kAssemblerTokenType type);

#endif /* EMEX64ASM_LEXER_H */
