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

#ifndef EMEX64_DIAGNOSTIC_LOG_H
#define EMEX64_DIAGNOSTIC_LOG_H

#include <stdbool.h>
#include <EmexToolchain/asm/type.h>
#include <EmexToolchain/asm/invocation.h>
#include <EmexToolchain/support/diagnostic/consumer.h>

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

#define diag_note(at, msg, ...) diag_log_legacy(kDiagnosticSeverityNote, at, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_warn(at, msg, ...) diag_log_legacy(kDiagnosticSeverityWarning, at, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_error(at, msg, ...) diag_log_legacy(kDiagnosticSeverityError, at, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_fatal(at, msg, ...) diag_log_legacy(kDiagnosticSeverityFatal, at, msg __VA_OPT__(,) __VA_ARGS__)

#endif /* EMEX64_DIAGNOSTIC_LOG_H */
