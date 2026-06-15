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

#ifndef EMEX64VM_INSTRUCTION_ALU_H
#define EMEX64VM_INSTRUCTION_ALU_H

#include <emex64lib/vm/core.h>

void emex64_op_add(emex64_core_t *core);
void emex64_op_sub(emex64_core_t *core);
void emex64_op_mul(emex64_core_t *core);
void emex64_op_div(emex64_core_t *core);
void emex64_op_idiv(emex64_core_t *core);
void emex64_op_mod(emex64_core_t *core);
void emex64_op_not(emex64_core_t *core);
void emex64_op_neg(emex64_core_t *core);
void emex64_op_and(emex64_core_t *core);
void emex64_op_or(emex64_core_t *core);
void emex64_op_xor(emex64_core_t *core);
void emex64_op_shr(emex64_core_t *core);
void emex64_op_shl(emex64_core_t *core);
void emex64_op_sar(emex64_core_t *core);
void emex64_op_ror(emex64_core_t *core);
void emex64_op_rol(emex64_core_t *core);

void emex64_op_pdep(emex64_core_t *core);
void emex64_op_pext(emex64_core_t *core);
void emex64_op_bswapw(emex64_core_t *core);
void emex64_op_bswapd(emex64_core_t *core);
void emex64_op_bswapq(emex64_core_t *core);
void emex64_op_inc(emex64_core_t *core);
void emex64_op_dec(emex64_core_t *core);

#endif /* EMEX64VM_INSTRUCTION_ALU_H */
