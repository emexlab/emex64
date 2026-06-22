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

#include <stdlib.h>
#include <strings.h>

#include <emex64lib/asm/preprocessor/condition.h>

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
