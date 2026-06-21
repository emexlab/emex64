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

#ifndef EMEX64ASM_OPTIONS_H
#define EMEX64ASM_OPTIONS_H

#include <stdbool.h>

typedef struct assembler_driver_options {
    bool assemble_only;
    bool verbose;
    bool in_process;
} assembler_driver_options_t;

typedef struct assembler_invocation_options {
    /* features */
    bool page_align;                /* default: true */
    bool caret_diagnostics;         /* default: true */

    /* warnings */
    bool warning_error;             /* default: false */
    bool warning_deprecated;        /* default: true */
} assembler_invocation_options_t;

extern assembler_driver_options_t assembler_driver_options_default;
extern assembler_invocation_options_t assembler_invocation_options_default;

#endif /* EMEX64ASM_OPTIONS_H */
