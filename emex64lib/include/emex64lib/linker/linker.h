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

#ifndef EMEX64LD_LINKER_H
#define EMEX64LD_LINKER_H

#include <emex64lib/linker/type.h>
#include <emex64lib/linker/header.h>
#include <emex64lib/linker/sym.h>
#include <emex64lib/linker/obj.h>

typedef struct {
    linker_global_symbol_t *sym;
} linker_invocation_t;

linker_invocation_t *linker_invocation_alloc(void);
void linker_invocation_dealloc(linker_invocation_t *inv);

bool linker_append_global_symbol_definition(linker_invocation_t *inv, const char *name, const char *object_path, uint64_t addr);
linker_global_symbol_t *linker_lookup_global_symbol(linker_invocation_t *inv, const char *name);

#endif /* EMEX64LD_LINKER_H */
