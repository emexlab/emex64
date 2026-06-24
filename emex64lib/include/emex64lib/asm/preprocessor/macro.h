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

#ifndef EMEX64ASM_MACRO_H
#define EMEX64ASM_MACRO_H

#include <stdint.h>
#include <stdbool.h>
#include <emex64lib/support/hashmap/hashmap.h>
#include <emex64lib/asm/type.h>

typedef struct assembler_macro {
    const char **inject_token;  /* borrowed */
    uint64_t inject_token_cnt;
} assembler_macro_t;

typedef struct assembler_macro_storage {
    hashmap_t *macro_map;
} assembler_macro_storage_t;

assembler_macro_t *assembler_macro_alloc(const char *match, const char **inject_token, uint64_t token_cnt);
void assembler_macro_dealloc(assembler_macro_t *macro);

assembler_macro_storage_t *assembler_macro_storage_alloc();
void assembler_macro_storage_dealloc(assembler_macro_storage_t *storage);

assembler_macro_t *assembler_macro_storage_lookup(assembler_macro_storage_t *storage, const char *match);
bool assembler_macro_storage_append_macro_char(assembler_macro_storage_t *storage, const char *match, const char **token, uint64_t token_cnt);
bool assembler_macro_storage_append_macro(assembler_macro_storage_t *storage, const char *match, assembler_token_t **token, uint64_t token_cnt);
void assembler_macro_storage_remove_macro(assembler_macro_storage_t *storage, const char *match);

#endif /* EMEX64ASM_MACRO_H */
