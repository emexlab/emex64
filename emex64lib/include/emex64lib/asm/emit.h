/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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

#ifndef EMEX64ASM_EMIT_H
#define EMEX64ASM_EMIT_H

#include <emex64lib/vm/core.h>
#include <emex64lib/vm/memory.h>
#include <emex64lib/asm/invocation.h>
#include <emex64lib/asm/type.h>
#include <emex64lib/asm/label.h>
#include <emex64lib/asm/opcode.h>
#include <emex64lib/asm/invocation.h>
#include <stdbool.h>

#include <emex64lib/support/fdwalker.h>

/* opcode emit */
void assembler_emit_opcode(assembler_invocation_t *inv, uint8_t op);

/* register emit */
void assembler_emit_register(assembler_invocation_t *inv, uint8_t reg);

/* intermediate emit */
void assembler_emit_imm8(assembler_invocation_t *inv, uint8_t imm);
void assembler_emit_imm16(assembler_invocation_t *inv, uint16_t imm);
void assembler_emit_imm32(assembler_invocation_t *inv, uint32_t imm);
void assembler_emit_imm64(assembler_invocation_t *inv, uint64_t imm);
void assembler_emit_imm(assembler_invocation_t *inv, uint64_t imm);

/* end emitter */
void assembler_emit_end(assembler_invocation_t *inv);

/* instruction emitter */
bool assembler_emit_instruction_inc(const opcode_entry_t *opce, assembler_line_t *al);
bool assembler_emit_instruction_dec(const opcode_entry_t *opce, assembler_line_t *al);
bool assembler_emit_instruction_clr(const opcode_entry_t *opce, assembler_line_t *al);
bool assembler_emit_instruction_generic(const opcode_entry_t *opce, assembler_line_t *al);

/* automised code emitting */
bool assembler_emit_line(assembler_line_t *al);
bool assembler_emit(assembler_invocation_t *inv);

#endif /* EMEX64ASM_EMIT_H */
