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

#include <emex64vm/core.h>
#include <emex64vm/memory.h>
#include <emex64asm/type.h>
#include <emex64asm/label.h>
#include <emex64asm/opcode.h>
#include <stdbool.h>

#include <emexutils/fdwalker.h>

/* opcode emit */
void la64_compiler_emit_opcode(fdwalker_t *fw, uint8_t op);

/* register emit */
void la64_compiler_emit_reg(fdwalker_t *fw, uint8_t reg);

/* intermediate emit */
void la64_compiler_emit_imm8(fdwalker_t *fw, uint8_t imm);
void la64_compiler_emit_imm16(fdwalker_t *fw, uint16_t imm);
void la64_compiler_emit_imm32(fdwalker_t *fw, uint32_t imm);
void la64_compiler_emit_imm64(fdwalker_t *fw, uint64_t imm);
void la64_compiler_emit_imm(fdwalker_t *fw, uint64_t imm);

/* end emitter */
void la64_compiler_emit_end(fdwalker_t *fw);

/* instruction emitter */
bool la64_compiler_emit_instr_inc(const opcode_entry_t *opce, compiler_line_t *cl);
bool la64_compiler_emit_instr_dec(const opcode_entry_t *opce, compiler_line_t *cl);
bool la64_compiler_emit_instr_clr(const opcode_entry_t *opce, compiler_line_t *cl);
bool la64_compiler_emit_instr_default(const opcode_entry_t *opce, compiler_line_t *cl);

/* automised code emitting */
bool la64_compiler_emit(compiler_line_t *cl);
bool la64_compiler_emit_all(compiler_invocation_t *ci);

#endif /* EMEX64ASM_EMIT_H */
