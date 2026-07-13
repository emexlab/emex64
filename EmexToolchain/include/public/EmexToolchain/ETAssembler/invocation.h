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

#ifndef EMEX64ASM_INVOCATION_H
#define EMEX64ASM_INVOCATION_H

#include <EmexToolchain/Support/file.h>
#include <EmexToolchain/Support/hashmap/hashmap.h>
#include <EmexToolchain/ETAssembler/diagnostic/consumer.h>
#include <EmexToolchain/ETAssembler/label/label.h>
#include <EmexToolchain/ETAssembler/label/relocate.h>
#include <EmexToolchain/ETAssembler/type.h>
#include <EmexToolchain/ETAssembler/ETAssemblerOptions.h>

typedef struct {
    char *match;
    char *value;
} assembler_macro_definition_t;

typedef struct assembler_invocation {
    assembler_diagnostic_consumer_t *consumer;  /* borrowed */

    emex_file_t **file;
    size_t file_cnt;

    assembler_line_t **line;
    UInt64 line_cnt;

    char *label_scope;
    hashmap_t *label_hashmap;

    UInt64 definition_cnt;                      /* borrowed */
    assembler_macro_definition_t *definition;   /* borrowed */

    char **include_dirs;                        /* borrowed */
    size_t include_dir_cnt;                     /* borrowed */

    reloc_table_entry_t *rtbe;
    vbitwalker_t *out_vbitwalker;

    UInt64 data_section_start;
    UInt64 data_section_end;
    UInt64 bss_section_start;
    UInt64 bss_section_size;
} assembler_invocation_t;

assembler_invocation_t *assembler_invocation_alloc(assembler_diagnostic_consumer_t *consumer);
void assembler_invocation_dealloc(assembler_invocation_t *inv);

Boolean assembler_invocation_emit(assembler_invocation_t *inv, emex_file_t *input, emex_file_t *output);

#endif /* EMEX64ASM_INVOCATION_H */
