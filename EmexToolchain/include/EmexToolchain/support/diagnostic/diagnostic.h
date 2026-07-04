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

#ifndef EMEX64_DIAGNOSTIC_DIAGNOSTIC_H
#define EMEX64_DIAGNOSTIC_DIAGNOSTIC_H

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

typedef enum: uint8_t {
    kDiagnosticSeverityNote,
    kDiagnosticSeverityWarning,
    kDiagnosticSeverityError,
    kDiagnosticSeverityFatal,
} kDiagnosticSeverity;

typedef struct diagnostic_location_text_range {
    uint64_t start_col;
    uint64_t end_col;
} diagnostic_location_text_range_t;

typedef struct diagnostic_location {
    char *file_name;
    char *line;
    uint64_t ln;
    uint64_t col;
    diagnostic_location_text_range_t range;
} diagnostic_location_t;

typedef struct diagnostic {
    char *str;
    kDiagnosticSeverity severity;
    diagnostic_location_t *location;
} diagnostic_t;

diagnostic_t *diagnostic_allocv(kDiagnosticSeverity severity, diagnostic_location_t *location, char *str, va_list args);
diagnostic_t *diagnostic_alloc(kDiagnosticSeverity severity, diagnostic_location_t *location, char *str, ...);
void diagnostic_dealloc(diagnostic_t *diagnostic);

#endif /* EMEX64_DIAGNOSTIC_DIAGNOSTIC_H */
