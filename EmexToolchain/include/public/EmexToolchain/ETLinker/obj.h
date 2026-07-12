/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EMEX64LD_OBJ_H
#define EMEX64LD_OBJ_H

#include <EmexToolchain/Support/file.h>
#include <EmexToolchain/ETLinker/type.h>
#include <EmexToolchain/ETLinker/header.h>

#define linker_object_text_size(obj) ((obj)->idx_text >= 0 ? (obj)->shdrs[(obj)->idx_text].sh_size : 0)
#define linker_object_data_size(obj) ((obj)->idx_data >= 0 ? (obj)->shdrs[(obj)->idx_data].sh_size : 0)
#define linker_object_bss_size(obj) ((obj)->idx_bss >= 0 ? (obj)->shdrs[(obj)->idx_bss].sh_size : 0)

typedef struct linker_invocation linker_invocation_t;

typedef struct linker_object {
    emex_file_t *file;  /* borrowed */

    ELF64_Ehdr *ehdr;
    ELF64_Shdr *shdrs;
    char *shstrtab;

    SInt32 idx_text;
    SInt32 idx_data;
    SInt32 idx_bss;
    SInt32 idx_rela_text;
    SInt32 idx_rela_data;
    SInt32 idx_symtab;
    SInt32 idx_strtab;

    UInt64 base_text; /* text base inside final object */
    UInt64 base_data; /* data base inside final object */
    UInt64 base_bss;  /* bss base inside final object */

    struct linker_object *next;
} linker_object_t;

linker_object_t *linker_object_alloc(emex_file_t *object_file);
void linker_object_dealloc(linker_object_t *obj);

Boolean linker_load_object(linker_invocation_t *inv, emex_file_t *object_file);
void linker_layout(linker_invocation_t *inv);

#endif /* EMEX64LD_OBJ_H */
