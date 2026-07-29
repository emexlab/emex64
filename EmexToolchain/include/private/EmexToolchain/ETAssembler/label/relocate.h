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

#ifndef EMEX64ASM_LABEL_RELOCATE_H
#define EMEX64ASM_LABEL_RELOCATE_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETAssembler/ETAssemblerType.h>

typedef struct __ETAssemblerInvocation *ETAssemblerInvocationRef;

typedef struct reloc_table_entry {
    char *name;                         /* resolved label name */
    Boolean local;                      /* must be resolved at assemble time */
    EFSize byte_pos;                    /* position */
    assembler_token_t *at_link;         /* link to the originator of the entry */
    struct reloc_table_entry *next;     /* pointer to next entry */
} reloc_table_entry_t;

Boolean assembler_label_relocate_append(ETAssemblerInvocationRef inv, char *label_str, Boolean local, assembler_token_t *at_link);

#endif /* EMEX64ASM_LABEL_RELOCATE_H */
