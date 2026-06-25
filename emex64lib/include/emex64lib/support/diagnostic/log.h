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

#ifndef EMEX64_DIAGNOSTIC_LOG_H
#define EMEX64_DIAGNOSTIC_LOG_H

#include <stdbool.h>
#include <emex64lib/asm/type.h>
#include <emex64lib/asm/invocation.h>
#include <emex64lib/support/diagnostic/consumer.h>

/* legacy wrapper */

#define diag_log_legacy(severity, at, msg, ...) \
    do { \
        _Pragma("GCC warning \"diag_log_* API is deprecated; call diagnostic_report() directly and use the diagnostic_consumer_t infrastructure\"") \
        if((at) == NULL) \
        { \
            diagnostic_report(NULL, severity, NULL, msg __VA_OPT__(,) __VA_ARGS__); \
        } \
        else \
        { \
            assembler_token_t *_diag_at = (assembler_token_t *)(at); \
            diagnostic_report(NULL, severity, &(diagnostic_location_t){ \
                .file_name = _diag_at->al->inv->file[_diag_at->al->file_idx]->path, \
                .line = _diag_at->al->str, \
                .ln = _diag_at->al->line_num, \
                .col = _diag_at->column_num, \
                .range = (diagnostic_location_text_range_t){ \
                    .start_col = _diag_at->column_num, \
                    .end_col = _diag_at->column_num + _diag_at->real_len } \
            }, msg __VA_OPT__(,) __VA_ARGS__); \
        } \
    } while(0);

#define diag_note(at, msg, ...) diagnostic_report(NULL, kDiagnosticSeverityNote, NULL, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_warn(at, msg, ...) diagnostic_report(NULL, kDiagnosticSeverityWarning, NULL, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_error(at, msg, ...) diag_log_legacy(kDiagnosticSeverityError, at, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_fatal(at, msg, ...) diagnostic_report(NULL, kDiagnosticSeverityFatal, NULL, msg __VA_OPT__(,) __VA_ARGS__)

#endif /* EMEX64_DIAGNOSTIC_LOG_H */
