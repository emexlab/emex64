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

#ifndef EMEX64ASM_DIAGNOSTIC_CONSUMER_H
#define EMEX64ASM_DIAGNOSTIC_CONSUMER_H

#include <stdio.h>
#include <stdlib.h>
#include <emex64lib/support/virtual/vfd.h>
#include <emex64lib/support/diagnostic/consumer.h>
#include <emex64lib/asm/options.h>

#define AT_TO_DLOC(at) &(diagnostic_location_t){ \
                .file_name = (at)->al->inv->file[(at)->al->file_idx]->path, \
                .line = (at)->al->str, \
                .ln = (at)->al->line_num, \
                .col = (at)->column_num, \
                .range = (diagnostic_location_text_range_t){ \
                    .start_col = (at)->column_num, \
                    .end_col = (at)->column_num + (at)->real_len } \
            }

typedef diagnostic_consumer_t assembler_diagnostic_consumer_t;

typedef struct assembler_diagnostic_consumer_context {
    assembler_diagnostic_options_t options;
    diagnostic_t **diagnostic;
    uint64_t diagnostic_cnt;
    vfd_t *d;
} assembler_diagnostic_consumer_context_t;

assembler_diagnostic_consumer_t *assembler_diagnostic_consumer_alloc(assembler_diagnostic_options_t options);
void assembler_diagnostic_consumer_dealloc(assembler_diagnostic_consumer_t *consumer);

#endif /* EMEX64ASM_DIAGNOSTIC_CONSUMER_H */
