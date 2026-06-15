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

#include <emex64lib/linker/elf.h>

typedef struct {
    const char *object_path;
    uint8_t *data;
    size_t size;

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

    uint64_t base_text;
    uint64_t base_data;
    uint64_t base_bss;
} Obj;

#endif /* EMEX64LD_OBJ_H */
