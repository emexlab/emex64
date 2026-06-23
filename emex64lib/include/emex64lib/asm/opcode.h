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

#ifndef EMEX64ASM_OPCODE_H
#define EMEX64ASM_OPCODE_H

#include <stdint.h>
#include <stdbool.h>
#include <emex64lib/support/parser.h>
#include <emex64lib/vm/core.h>
#include <emex64lib/asm/type.h>

typedef struct opcode_entry opcode_entry_t;

/* handler for emitting instruction */
typedef bool (*instruction_emit_handler)(const opcode_entry_t *opce, assembler_line_t *cl);

/* opcode entry gathering */
kEmex64Opcode opcode_from_string(const char *name, bool *success);
bool opcode_arg_accepts_reg_only(const emex64_opfunc_entry_t *opce, uint8_t arg);

#endif /* EMEX64ASM_OPCODE_H */
