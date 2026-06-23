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

#ifndef EMEX64_DIAGNOSTIC_LEGACY_H
#define EMEX64_DIAGNOSTIC_LEGACY_H

#include <stdbool.h>
#include <emex64lib/asm/type.h>

#define diag_note(at, msg, ...) diag_log(DIAG_NOTE, at, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_warn(at, msg, ...) diag_log(DIAG_WARN, at, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_error(at, msg, ...) diag_log(DIAG_ERROR, at, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_fatal(at, msg, ...) diag_log(DIAG_FATAL, at, msg __VA_OPT__(,) __VA_ARGS__)

typedef struct diagnostic_logging_options {
    bool warning_error;
    bool caret_diagnostics;
    bool color_diagnostics;
} diagnostic_logging_options_t;

extern _Thread_local diagnostic_logging_options_t thread_log_diagnostic_options;

typedef enum {
    DIAG_NOTE,
    DIAG_WARN,
    DIAG_ERROR,
    DIAG_FATAL,
} diag_level_t;

void diag_log(diag_level_t level, assembler_token_t *at, const char *msg, ...);

#endif /* EMEX64_DIAGNOSTIC_LEGACY_H */
