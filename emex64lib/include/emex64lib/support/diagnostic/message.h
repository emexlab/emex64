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

#ifndef DIAGNOSTIC_MESSAGE_H
#define DIAGNOSTIC_MESSAGE_H

#include <stdint.h>

typedef enum: uint8_t {
    kDiagnosticMessageSourceTypeInternal,
    kDiagnosticMessageSourceTypeFile
} kDiagnosticMessageSourceType;

typedef enum: uint8_t {
    kDiagnosticMessageTypeNote,
    kDiagnosticMessageTypeRemark,
    kDiagnosticMessageTypeWarning,
    kDiagnosticMessageTypeError,
    kDiagnosticMessageTypeFatal,
} kDiagnosticMessageType;

typedef struct diagnostic_message_source {
    kDiagnosticMessageSourceType type;

    union {
        struct {
            char *file;
            uint64_t ln;
            uint64_t col;
        } file;
    };
} diagnostic_message_source_t;

typedef struct diagnostic_message {
    kDiagnosticMessageType type;
    diagnostic_message_source_t source;
    char *str;
    struct diagnostic_message *prev;
    struct diagnostic_message *next;
} diagnostic_message_t;

diagnostic_message_t *diagnostic_message_alloc_internal(kDiagnosticMessageType type, char *msg);
diagnostic_message_t *diagnostic_message_alloc_file(kDiagnosticMessageType type, char *file, char *msg, uint64_t ln, uint64_t col);
void diagnostic_message_dealloc(diagnostic_message_t *message);

#endif /* DIAGNOSTIC_MESSAGE_H */
