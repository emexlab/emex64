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

#ifndef DIAGNOSTIC_STORAGE_H
#define DIAGNOSTIC_STORAGE_H

#include <emex64lib/support/diagnostic/message.h>

typedef struct {
    diagnostic_message_t *tail;
} diagnostic_storage_t;

diagnostic_storage_t *diagnostic_storage_alloc();
void diagnostic_storage_dealloc(diagnostic_storage_t *storage);

void diagnostic_storage_append_internal(diagnostic_storage_t *storage, kDiagnosticMessageType type, char *msg);
void diagnostic_storage_append_file(diagnostic_storage_t *storage, kDiagnosticMessageType type, char *file, char *msg, uint64_t ln, uint64_t col);

#endif /* DIAGNOSTIC_STORAGE_H */
