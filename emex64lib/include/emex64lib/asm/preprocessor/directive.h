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

#ifndef EMEX64ASM_DIRECTIVE_H
#define EMEX64ASM_DIRECTIVE_H

#include <stdint.h>

typedef enum: uint8_t {
    /* sentinel */
    kAssemblerPreprocessorDirectiveTypeUnknown, /* the preprocessor shall flag this */

    /* importer  */
    kAssemblerPreprocessorDirectiveTypeInclude,

    /* macro */
    kAssemblerPreprocessorDirectiveTypeDefine,
    kAssemblerPreprocessorDirectiveTypeUndefine,

    /* conditions */
    kAssemblerPreprocessorDirectiveTypeIf,
    kAssemblerPreprocessorDirectiveTypeIfDefined,
    kAssemblerPreprocessorDirectiveTypeIfNotDefined,
    kAssemblerPreprocessorDirectiveTypeElseIf,
    kAssemblerPreprocessorDirectiveTypeElse,
    kAssemblerPreprocessorDirectiveTypeEndIf,
} kAssemblerPreprocessorDirectiveType;

kAssemblerPreprocessorDirectiveType assembler_directive_type_for_str(const char *str);

#endif /* EMEX64ASM_DIRECTIVE_H */
