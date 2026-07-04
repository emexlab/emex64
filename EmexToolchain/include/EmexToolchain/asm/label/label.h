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

#ifndef EMEX64ASM_LABEL_LABEL_H
#define EMEX64ASM_LABEL_LABEL_H

#include <stdbool.h>
#include <EmexToolchain/asm/type.h>

typedef struct assembler_invocation assembler_invocation_t;

typedef struct {
    char *name;                             /* name of resolved label */
    bool defined;                           /* label definitions are defined */
    uint64_t addr;                          /* address of resolved label */
    assembler_token_t *at_link;             /* link to the originator of the label */
} assembler_label_t;

bool assembler_label_append(assembler_token_t *at);

assembler_label_t *assembler_label_lookup(assembler_invocation_t *inv, const char *name);

#endif /* EMEX64ASM_LABEL_LABEL_H */
