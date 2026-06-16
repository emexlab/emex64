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

#ifndef EMEX64LD_OBJ_H
#define EMEX64LD_OBJ_H

#include <emex64lib/support/file.h>

#include <emex64lib/linker/type.h>
#include <emex64lib/linker/header.h>

#define linker_object_text_size(obj) ((obj)->idx_text >= 0 ? (obj)->shdrs[(obj)->idx_text].sh_size : 0)
#define linker_object_data_size(obj) ((obj)->idx_data >= 0 ? (obj)->shdrs[(obj)->idx_data].sh_size : 0)
#define linker_object_bss_size(obj) ((obj)->idx_bss >= 0 ? (obj)->shdrs[(obj)->idx_bss].sh_size : 0)

typedef struct linker_invocation linker_invocation_t;

typedef struct linker_object {
    emex_file_t *file;

    ELF64_Ehdr *ehdr;
    ELF64_Shdr *shdrs;
    char *shstrtab;

    int32_t idx_text;
    int32_t idx_data;
    int32_t idx_bss;
    int32_t idx_rela_text;
    int32_t idx_rela_data;
    int32_t idx_symtab;
    int32_t idx_strtab;

    uint64_t base_text; /* text base inside final object */
    uint64_t base_data; /* data base inside final object */
    uint64_t base_bss;  /* bss base inside final object */

    struct linker_object *next;
} linker_object_t;

linker_object_t *linker_object_alloc(const char *object_path);
void linker_object_dealloc(linker_object_t *obj);

bool linker_load_object(linker_invocation_t *inv, const char *object_path);

#endif /* EMEX64LD_OBJ_H */
