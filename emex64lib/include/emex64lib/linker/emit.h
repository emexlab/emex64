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

#ifndef EMEX64LD_EMIT_H
#define EMEX64LD_EMIT_H

#include <emex64lib/support/virtual/vbitwalker.h>
#include <emex64lib/support/file.h>
#include <emex64lib/linker/type.h>
#include <emex64lib/linker/options.h>
#include <emex64lib/linker/diagnostic/consumer.h>

typedef struct linker_invocation linker_invocation_t;

bool linker_link(linker_options_t options, linker_diagnostic_consumer_t *diagnostic_consumer, emex_file_t **input_file, uint64_t input_file_cnt, emex_file_t **linker_script_file, uint64_t linker_script_file_cnt, emex_file_t *output);

#endif /* EMEX64LD_EMIT_H */
