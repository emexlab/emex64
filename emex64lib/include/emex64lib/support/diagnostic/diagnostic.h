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
