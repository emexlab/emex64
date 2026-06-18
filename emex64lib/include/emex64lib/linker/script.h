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

#ifndef EMEX64LD_SCRIPT_H
#define EMEX64LD_SCRIPT_H

#include <stdint.h>
#include <stdbool.h>

#include <emex64lib/support/file.h>

#include <emex64lib/linker/type.h>

typedef struct linker_invocation linker_invocation_t;

typedef struct {
    const char *script_path;    /* borrowed */
    char *name;
    char *expr;
} script_sym_t;

bool linker_script_parse(linker_invocation_t *inv, emex_file_t *script_file);
bool linker_script_apply(linker_invocation_t *inv, uint64_t image_end, uint64_t text_start, uint64_t data_start, uint64_t bss_start);

#endif /* EMEX64LD_SCRIPT_H */
