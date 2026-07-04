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

#include <stdlib.h>
#include <strings.h>
#include <EmexToolchain/asm/preprocessor/condition.h>

static inline assembler_condition_frame_t *__assembler_condition_frame_alloc()
{
    return calloc(1, sizeof(assembler_condition_frame_t));
}

void assembler_condition_frame_dealloc(assembler_condition_frame_t **frame)
{
    assembler_condition_frame_t *tail = *frame;
    while(tail != NULL)
    {
        assembler_condition_frame_t *prev = tail->prev;
        free(tail);
        tail = prev;
    }
}

bool assembler_condition_frame_push(assembler_condition_frame_t **frame)
{
    assembler_condition_frame_t *new = __assembler_condition_frame_alloc();
    if(new == NULL)
    {
        goto out_failure;
    }
    
    new->prev = *frame;
    *frame = new;
    
    return true;
    
out_failure:
    assembler_condition_frame_dealloc(frame);
    return false;
}

void assembler_condition_frame_pop(assembler_condition_frame_t **frame)
{
    if(*frame == NULL)
    {
        return;
    }
    
    assembler_condition_frame_t *prev = (*frame)->prev;
    free(*frame);
    *frame = prev;
}

void assembler_condition_state_init(assembler_condition_state_t *state)
{
    bzero(state, sizeof(assembler_condition_state_t));
}

void assembler_condition_state_deinit(assembler_condition_state_t *state)
{
    assembler_condition_frame_dealloc(&state->frame);
    bzero(state, sizeof(assembler_condition_state_t));
}

bool assembler_condition_state_push(assembler_condition_state_t *state)
{
    if(!assembler_condition_frame_push(&state->frame))
    {
        return false;
    }
    
    state->frame->condition_met = state->condition_met;
    state->frame->in_a_condition = state->in_a_condition;
    state->frame->in_a_else_condition = state->in_a_else_condition;
    state->frame->finalized = state->finalized;
    state->frame->parent_active = state->parent_active;
    state->frame->last_condition_line = state->last_condition_line;
    
    state->condition_met = false;
    state->in_a_condition = false;
    state->in_a_else_condition = false;
    state->finalized = false;
    state->last_condition_line = NULL;
    
    return true;
}

void assembler_condition_state_pop(assembler_condition_state_t *state)
{
    state->condition_met = state->frame->condition_met;
    state->in_a_condition = state->frame->in_a_condition;
    state->in_a_else_condition = state->frame->in_a_else_condition;
    state->finalized = state->frame->finalized;
    state->parent_active = state->frame->parent_active;
    state->last_condition_line = state->frame->last_condition_line;
    
    assembler_condition_frame_pop(&state->frame);
}
