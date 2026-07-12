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

#ifndef EMEX64ASM_CONDITION_H
#define EMEX64ASM_CONDITION_H

#include <EmexToolchain/ETAssembler/type.h>

typedef struct assembler_directive_condition_frame {
    Boolean in_a_condition;
    Boolean condition_met;
    Boolean in_a_else_condition;
    Boolean finalized;
    Boolean parent_active;
    assembler_line_t *last_condition_line;
    struct assembler_directive_condition_frame *prev;
} assembler_condition_frame_t;

typedef struct assembler_directive_condition_state {
    assembler_condition_frame_t *frame;
    Boolean in_a_condition;
    Boolean condition_met;
    Boolean in_a_else_condition;
    Boolean finalized;
    Boolean parent_active;
    assembler_line_t *last_condition_line;
} assembler_condition_state_t;

void assembler_condition_frame_dealloc(assembler_condition_frame_t **frame);
Boolean assembler_condition_frame_push(assembler_condition_frame_t **frame);
void assembler_condition_frame_pop(assembler_condition_frame_t **frame);

void assembler_condition_state_init(assembler_condition_state_t *state);
void assembler_condition_state_deinit(assembler_condition_state_t *state);
Boolean assembler_condition_state_push(assembler_condition_state_t *state);
void assembler_condition_state_pop(assembler_condition_state_t *state);

#endif /* EMEX64ASM_CONDITION_H */
