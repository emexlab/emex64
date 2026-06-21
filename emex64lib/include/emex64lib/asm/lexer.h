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

#ifndef EMEX64ASM_LEXER_H
#define EMEX64ASM_LEXER_H

#include <stdint.h>
#include <stddef.h>

#define CMPTOK_LENGHT_MAX               2048    /* if anyone comes close to that size, bro pls fix your variable naming style O.O */

enum kCmptokTokenMode: uint8_t {
    /* the nothing mode */
    kCmptokTokenModeNone,

    /*
     * string mode, means it parses the
     * next characters as a character
     * buffer sequence
     */
    kCmptokTokenModeString,

    /*
     * character mode, means it parses the next characters as
     * a character, if its not a valid character next steps in
     * compilation will fail, but thats not responsibility
     * of this parser!
     */
    kCmptokTokenModeCharacter,

    /*
     * expression mode is for things like (SOME_MACRO + 4 + (NYA + 10))
     */
    kCmptokTokenModeExpression,
};

enum kAssemblerTokenType: uint8_t {
    kAssemblerTokenTypeInvalid,
    kAssemblerTokenTypeStandard,
    kAssemblerTokenTypeStaticExpression,
};

typedef struct {
    const char *token;
    size_t column;
    enum kAssemblerTokenType type;
} cmptok_token_t;

cmptok_token_t cmptok(const char *token);

#endif /* EMEX64ASM_LEXER_H */
