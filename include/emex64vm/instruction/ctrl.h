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

#ifndef EMEX64VM_INSTRUCTION_CTRL_H
#define EMEX64VM_INSTRUCTION_CTRL_H

#include <emex64vm/core.h>

void la64_op_b(la64_core_t *core);
void la64_op_cmp(la64_core_t *core);
void la64_op_be(la64_core_t *core);
void la64_op_bne(la64_core_t *core);
void la64_op_blt(la64_core_t *core);
void la64_op_bgt(la64_core_t *core);
void la64_op_ble(la64_core_t *core);
void la64_op_bge(la64_core_t *core);
void la64_op_bz(la64_core_t *core);
void la64_op_bnz(la64_core_t *core);

void la64_push(la64_core_t *core, uint64_t value);
uint64_t la64_pop(la64_core_t *core);

void la64_op_bl(la64_core_t *core);
void la64_op_ret(la64_core_t *core);
void la64_op_iret(la64_core_t *core);

#endif /* EMEX64VM_INSTRUCTION_CTRL_H */