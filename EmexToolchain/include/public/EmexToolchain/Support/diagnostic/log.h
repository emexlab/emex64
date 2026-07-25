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

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>
#include <EmexToolchain/ETAssembler/type.h>
#include <EmexToolchain/Support/diagnostic/consumer.h>

/* legacy wrapper */
#define diag_log_legacy(severity, at, msg, ...) \
    do { \
        diagnostic_report(NULL, severity, NULL, msg __VA_OPT__(,) __VA_ARGS__); \
    } while(0);

#define diag_note(at, msg, ...) diag_log_legacy(kDiagnosticSeverityNote, at, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_warn(at, msg, ...) diag_log_legacy(kDiagnosticSeverityWarning, at, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_error(at, msg, ...) diag_log_legacy(kDiagnosticSeverityError, at, msg __VA_OPT__(,) __VA_ARGS__)
#define diag_fatal(at, msg, ...) diag_log_legacy(kDiagnosticSeverityFatal, at, msg __VA_OPT__(,) __VA_ARGS__)

#endif /* EMEX64_DIAGNOSTIC_LOG_H */
