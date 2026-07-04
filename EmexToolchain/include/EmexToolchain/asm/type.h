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

#ifndef EMEX64ASM_TYPE_H
#define EMEX64ASM_TYPE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <EmexToolchain/support/virtual/vbitwalker.h>
#include <EmexToolchain/vm/core.h>

typedef enum: UInt8 {
    kAssemblerTokenTypeInvalid,
    kAssemblerTokenTypeTooLong,

    kAssemblerTokenTypeIdentifier,  /* in the lextok step everything becomes a identifier at first */
    kAssemblerTokenTypeInteger,
    kAssemblerTokenTypeString,
    kAssemblerTokenTypeHeaderName,  /* shouldn't be classified after tokenization */
    kAssemblerTokenTypeRegister,
    kAssemblerTokenTypeInstruction,
    kAssemblerTokenTypeKeyword,
    kAssemblerTokenTypeComma,
    kAssemblerTokenTypeColon,
    kAssemblerTokenTypeLParen,
    kAssemblerTokenTypeRParen,
    kAssemblerTokenTypePlus,
    kAssemblerTokenTypeMinus,
    kAssemblerTokenTypeMultiply,
    kAssemblerTokenTypeDivide,
} kAssemblerTokenType;

typedef enum: UInt8 {
    kAssemblerKeywordSection,
    kAssemblerKeywordExtern,
    kAssemblerKeywordInvalid,
} kAssemblerKeyword;

typedef enum: UInt8 {
    kAssemblerLineTypeNone,
    kAssemblerLineTypeIgnore,
    kAssemblerLineTypeAssembly,
    kAssemblerLineTypeExternLabel,
    kAssemblerLineTypeGlobalLabel,
    kAssemblerLineTypeLocalLabel,
    kAssemblerLineTypeSection,
    kAssemblerLineTypeSectionData,
    kAssemblerLineTypePreprocessorDirective,
} kAssemblerLineType;

typedef struct assembler_token {
    char *str;
    size_t column_num;                      /* start offset of the token in the text file */
    size_t real_len;                        /* real lenght in text file */
    struct assembler_line *al;              /* pointer back to compiler line */
    kAssemblerTokenType type;               /* OMG THAT IS AI?!?! the token type, WOAHHH AM I A AI, DID A AI GENERATE THIS TOKEN?!??!*/

    union {
        struct {
            UInt64 v; /* signed by default */
        } integer_literal;
        struct {
            char *buf;
            size_t len;
        } string_literal;
        struct {
            E64Register v;
        } register_identifier;
        struct {
            E64Opcode v;
        } instruction_identifier;
        struct {
            kAssemblerKeyword v;
        } keyword;
    };
} assembler_token_t;

typedef struct assembler_line {
    char *str;
    kAssemblerLineType type;                /* type of line */
    struct assembler_token **token;         /* subtokens */
    UInt64 token_cnt;                     /* count of subtokens */
    size_t line_num;                        /* line number in file */   
    size_t file_idx;                        /* index of file in compiler invocation */
    struct assembler_invocation *inv;       /* pointer back to compiler invocation */
} assembler_line_t;

#endif /* EMEX64ASM_TYPE_H */
