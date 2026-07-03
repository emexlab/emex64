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

#ifndef EMEX64LD_SYM_H
#define EMEX64LD_SYM_H

#include <emex64lib/linker/type.h>

typedef struct {
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} __attribute__((packed)) ELF64_Sym;

typedef struct {
    uint64_t r_offset;
    uint64_t r_info;
    int64_t r_addend;
} __attribute__((packed)) ELF64_Rela;

typedef struct linker_symbol {
    char *name;
    char *object_path;
    uint64_t addr;
    bool defined;
    struct linker_symbol *next;
} linker_symbol_t;

linker_symbol_t *linker_symbol_alloc(const char *name, const char *object_path, uint64_t addr, bool defined);
void linker_symbol_dealloc(linker_symbol_t *sym);

#endif /* EMEX64LD_SYM_H */
