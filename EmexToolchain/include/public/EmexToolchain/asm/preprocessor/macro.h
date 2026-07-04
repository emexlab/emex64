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

#ifndef EMEX64ASM_MACRO_H
#define EMEX64ASM_MACRO_H

#include <stdint.h>
#include <stdbool.h>
#include <EmexToolchain/support/hashmap/hashmap.h>
#include <EmexToolchain/asm/type.h>

typedef struct assembler_macro {
    const char **inject_token;  /* borrowed */
    UInt64 inject_token_cnt;
} assembler_macro_t;

typedef struct assembler_macro_storage {
    hashmap_t *macro_map;
} assembler_macro_storage_t;

assembler_macro_t *assembler_macro_alloc(const char *match, const char **inject_token, UInt64 token_cnt);
void assembler_macro_dealloc(assembler_macro_t *macro);

assembler_macro_storage_t *assembler_macro_storage_alloc();
void assembler_macro_storage_dealloc(assembler_macro_storage_t *storage);

assembler_macro_t *assembler_macro_storage_lookup(assembler_macro_storage_t *storage, const char *match);
Boolean assembler_macro_storage_append_macro_char(assembler_macro_storage_t *storage, const char *match, const char **token, UInt64 token_cnt);
Boolean assembler_macro_storage_append_macro(assembler_macro_storage_t *storage, const char *match, assembler_token_t **token, UInt64 token_cnt);
void assembler_macro_storage_remove_macro(assembler_macro_storage_t *storage, const char *match);

#endif /* EMEX64ASM_MACRO_H */
