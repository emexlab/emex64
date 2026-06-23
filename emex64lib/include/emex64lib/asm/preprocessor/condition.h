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

#ifndef EMEX64ASM_CONDITION_H
#define EMEX64ASM_CONDITION_H

#include <stdbool.h>
#include <emex64lib/asm/type.h>

typedef struct assembler_directive_condition_frame {
    bool in_a_condition;
    bool condition_met;
    bool in_a_else_condition;
    bool finalized;
    bool parent_active;
    assembler_line_t *last_condition_line;
    struct assembler_directive_condition_frame *prev;
} assembler_condition_frame_t;

typedef struct assembler_directive_condition_state {
    assembler_condition_frame_t *frame;
    bool in_a_condition;
    bool condition_met;
    bool in_a_else_condition;
    bool finalized;
    bool parent_active;
    assembler_line_t *last_condition_line;
} assembler_condition_state_t;

void assembler_condition_frame_dealloc(assembler_condition_frame_t **frame);
bool assembler_condition_frame_push(assembler_condition_frame_t **frame);
void assembler_condition_frame_pop(assembler_condition_frame_t **frame);

void assembler_condition_state_init(assembler_condition_state_t *state);
void assembler_condition_state_deinit(assembler_condition_state_t *state);
bool assembler_condition_state_push(assembler_condition_state_t *state);
void assembler_condition_state_pop(assembler_condition_state_t *state);

#endif /* EMEX64ASM_CONDITION_H */
