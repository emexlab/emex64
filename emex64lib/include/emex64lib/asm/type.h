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
#include <emex64lib/vm/core.h>

typedef enum: uint8_t {
    kAssemblerTokenTypeInvalid,
    kAssemblerTokenTypeTooLong,

    kAssemblerTokenTypeIdentifier,  /* in the lextok step everything becomes a identifier at first */
    kAssemblerTokenTypeInteger,
    kAssemblerTokenTypeString,
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

typedef enum: uint8_t {
    kAssemblerKeywordSection,
    kAssemblerKeywordExtern,
    kAssemblerKeywordInvalid,
} kAssemblerKeyword;

typedef enum: uint8_t {
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
            uint64_t v; /* signed by default */
        } integer_literal;
        struct {
            char *buf;
            size_t len;
        } string_literal;
        struct {
            kEmex64Register v;
        } register_identifier;
        struct {
            kEmex64Opcode v;
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
    uint64_t token_cnt;                     /* count of subtokens */
    size_t line_num;                        /* line number in file */   
    size_t file_idx;                        /* index of file in compiler invocation */
    struct assembler_invocation *inv;       /* pointer back to compiler invocation */
} assembler_line_t;

#endif /* EMEX64ASM_TYPE_H */
