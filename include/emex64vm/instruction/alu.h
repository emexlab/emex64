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

#ifndef EMEX64VM_INSTRUCTION_ALU_H
#define EMEX64VM_INSTRUCTION_ALU_H

#include <emex64vm/core.h>

void la64_op_add(la64_core_t *core);
void la64_op_sub(la64_core_t *core);
void la64_op_mul(la64_core_t *core);
void la64_op_div(la64_core_t *core);
void la64_op_idiv(la64_core_t *core);
void la64_op_mod(la64_core_t *core);
void la64_op_not(la64_core_t *core);
void la64_op_neg(la64_core_t *core);
void la64_op_and(la64_core_t *core);
void la64_op_or(la64_core_t *core);
void la64_op_xor(la64_core_t *core);
void la64_op_shr(la64_core_t *core);
void la64_op_shl(la64_core_t *core);
void la64_op_sar(la64_core_t *core);
void la64_op_ror(la64_core_t *core);
void la64_op_rol(la64_core_t *core);

void la64_op_pdep(la64_core_t *core);
void la64_op_pext(la64_core_t *core);
void la64_op_bswapw(la64_core_t *core);
void la64_op_bswapd(la64_core_t *core);
void la64_op_bswapq(la64_core_t *core);

#endif /* EMEX64VM_INSTRUCTION_ALU_H */
