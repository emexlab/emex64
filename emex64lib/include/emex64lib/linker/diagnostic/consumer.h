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

#ifndef EMEX64LD_DIAGNOSTIC_CONSUMER_H
#define EMEX64LD_DIAGNOSTIC_CONSUMER_H

#include <stdio.h>
#include <stdlib.h>
#include <emex64lib/support/virtual/vfd.h>
#include <emex64lib/support/diagnostic/consumer.h>

typedef diagnostic_consumer_t linker_diagnostic_consumer_t;

typedef struct linker_diagnostic_consumer_context {
    diagnostic_t **diagnostic;
    uint64_t diagnostic_cnt;
    vfd_t *d;
} linker_diagnostic_consumer_context_t;

linker_diagnostic_consumer_t *linker_diagnostic_consumer_alloc();
void linker_diagnostic_consumer_dealloc(linker_diagnostic_consumer_t *consumer);

void linker_diagnostic_consumer_emit(linker_diagnostic_consumer_t *consumer);

#endif /* EMEX64LD_DIAGNOSTIC_CONSUMER_H */
