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

#ifndef EMEX64ASM_OPCODE_H
#define EMEX64ASM_OPCODE_H

#include <emex64vm/core.h>

#include <emex64asm/type.h>

#include <emexutils/parser.h>
#include <emexutils/bitwalker.h>

#include <stdint.h>
#include <stdbool.h>

typedef struct opcode_entry opcode_entry_t;

/* handler for emitting instruction */
typedef bool (*instruction_emit_handler)(const opcode_entry_t *opce, compiler_line_t *cl);

typedef struct opcode_entry {
    const char *name;                   /* name to match with of opcode */
    uint8_t opcode;                     /* opcode in machine code */
    uint8_t minargs;                    /* minimum arguments count */
    uint8_t maxargs;                    /* maximum arguments count */
    uint32_t argmask;                   /* argument mask (0 means it doesnt matter what the operand is, 1 means it must be a register) */
    const char *dnstr;                  /* deprecation string if deprecated */
    instruction_emit_handler handler;   /* handler for emitting code */
} opcode_entry_t;

typedef struct {
    const opcode_entry_t *opce; /* pointer to opcode entry */
    uint8_t curarg;             /* current argument in validation */
} opcode_validator_t;

typedef struct {
    bool null;                  /* for null pointer exceptions */
    bool reg_only;              /* if only registers are allowed */
} opcode_validator_return_t;

/* opcode entry gathering */
const opcode_entry_t *opcode_from_string(const char *name);
bool opcode_arg_accepts_reg_only(const opcode_entry_t *opce, uint8_t arg);

#endif /* EMEX64ASM_OPCODE_H */
