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
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Core.h>

typedef struct __ETAssemblerInvocation *ETAssemblerInvocationRef;

typedef enum: UInt8 {
    kETAssemblerTokenTypeInvalid,
    kETAssemblerTokenTypeTooLong,

    kETAssemblerTokenTypeIdentifier,        /* in the lextok step everything becomes a identifier at first */
    kETAssemblerTokenTypeInteger,
    kETAssemblerTokenTypeString,
    kETAssemblerTokenTypeHeaderName,        /* shouldn't be classified after tokenization */
    kETAssemblerTokenTypeRegister,
    kETAssemblerTokenTypeRegisterExtended,
    kETAssemblerTokenTypeInstruction,
    kETAssemblerTokenTypeKeyword,
    kETAssemblerTokenTypeComma,
    kETAssemblerTokenTypeColon,
    kETAssemblerTokenTypeLParen,
    kETAssemblerTokenTypeRParen,
    kETAssemblerTokenTypePlus,
    kETAssemblerTokenTypeMinus,
    kETAssemblerTokenTypeMultiply,
    kETAssemblerTokenTypeDivide,
    kETAssemblerTokenTypeLPack,
    kETAssemblerTokenTypeRPack,
    kETAssemblerTokenTypeBitwiseOr,
    kETAssemblerTokenTypeBitwiseAnd,
    kETAssemblerTokenTypeLogicalOr,
    kETAssemblerTokenTypeLogicalAnd,
} ETAssemblerTokenType;

typedef enum: UInt8 {
    kETAssemblerKeywordSection,
    kETAssemblerKeywordExtern,
    kETAssemblerKeywordInvalid,
} ETAssemblerKeyword;

typedef enum: UInt8 {
    kETAssemblerLineTypeNone,
    kETAssemblerLineTypeIgnore,
    kETAssemblerLineTypeAssembly,
    kETAssemblerLineTypeExternSymbol,
    kETAssemblerLineTypeSymbol,
    kETAssemblerLineTypeLabel,                  /* automatically marked as local label */
    kETAssemblerLineTypeSection,
    kETAssemblerLineTypeSectionData,
    kETAssemblerLineTypePreprocessorDirective,
} ETAssemblerLineType;

typedef struct assembler_token {
    char *str;
    EFSize column_num;                          /* start offset of the token in the text file */
    EFSize real_len;                            /* real lenght in text file */
    struct assembler_line *al;                  /* pointer back to compiler line */
    ETAssemblerTokenType type;                  /* OMG THAT IS AI?!?! the token type, WOAHHH AM I A AI, DID A AI GENERATE THIS TOKEN?!??!*/

    union {
        struct {
            UInt64 v;                           /* signed by default */
        } integer_literal;
        struct {
            char *buf;
            EFSize len;
        } string_literal;
        struct {
            Boolean increment;
            Boolean actuallyDecrement;
            union {
                E64Register v;
                E64RegisterExtended v_extended;
            };
        } register_identifier;
        struct {
            E64Opcode v;
        } instruction_identifier;
        struct {
            ETAssemblerKeyword v;
        } keyword;
    };
} assembler_token_t;

typedef struct assembler_line {
    char *str;
    ETAssemblerLineType type;       /* type of line */
    struct assembler_token **token; /* subtokens */
    UInt64 token_cnt;               /* count of subtokens */
    EFSize line_num;                /* line number in file */   
    EFSize file_idx;                /* index of file in compiler invocation */
    ETAssemblerInvocationRef inv;   /* pointer back to compiler invocation */
} assembler_line_t;

#endif /* EMEX64ASM_TYPE_H */
