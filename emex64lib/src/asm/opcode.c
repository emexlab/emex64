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
#include <string.h>

#include <emex64lib/asm/opcode.h>
#include <emex64lib/asm/emit.h>

const opcode_entry_t opcode_table[] = {
    /* core operations */
    { .name = "hlt",    .opcode = kEmex64OpcodeHLT,         .minargs = 0, .maxargs = 0,                 .argmask = 0b0000000000000000 },
    { .name = "nop",    .opcode = kEmex64OpcodeNOP,         .minargs = 0, .maxargs = 0,                 .argmask = 0b0000000000000000 },

    /* data operations */
    { .name = "mov",    .opcode = kEmex64OpcodeMOV,         .minargs = 2, .maxargs = 2,                 .argmask = 0b1000000000000000 },
    { .name = "swp",    .opcode = kEmex64OpcodeSWP,         .minargs = 2, .maxargs = 2,                 .argmask = 0b1100000000000000 },
    { .name = "swpz",   .opcode = kEmex64OpcodeSWPZ,        .minargs = 2, .maxargs = 2,                 .argmask = 0b1100000000000000 },
    { .name = "push",   .opcode = kEmex64OpcodePUSH,        .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b0000000000000000 },
    { .name = "pop",    .opcode = kEmex64OpcodePOP,         .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b1111111111111111 },
    { .name = "ldb",    .opcode = kEmex64OpcodeLDB,         .minargs = 2, .maxargs = 2,                 .argmask = 0b1000000000000000 },
    { .name = "ldw",    .opcode = kEmex64OpcodeLDW,         .minargs = 2, .maxargs = 2,                 .argmask = 0b1000000000000000 },
    { .name = "ldd",    .opcode = kEmex64OpcodeLDD,         .minargs = 2, .maxargs = 2,                 .argmask = 0b1000000000000000 },
    { .name = "ldq",    .opcode = kEmex64OpcodeLDQ,         .minargs = 2, .maxargs = 2,                 .argmask = 0b1000000000000000 },
    { .name = "stb",    .opcode = kEmex64OpcodeSTB,         .minargs = 2, .maxargs = 2,                 .argmask = 0b0000000000000000 },
    { .name = "stw",    .opcode = kEmex64OpcodeSTW,         .minargs = 2, .maxargs = 2,                 .argmask = 0b0000000000000000 },
    { .name = "std",    .opcode = kEmex64OpcodeSTD,         .minargs = 2, .maxargs = 2,                 .argmask = 0b0000000000000000 },
    { .name = "stq",    .opcode = kEmex64OpcodeSTQ,         .minargs = 2, .maxargs = 2,                 .argmask = 0b0000000000000000 },

    /* alu operations */
    { .name = "add",    .opcode = kEmex64OpcodeADD,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "sub",    .opcode = kEmex64OpcodeSUB,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "mul",    .opcode = kEmex64OpcodeMUL,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "div",    .opcode = kEmex64OpcodeDIV,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "idiv",   .opcode = kEmex64OpcodeIDIV,        .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "mod",    .opcode = kEmex64OpcodeMOD,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "not",    .opcode = kEmex64OpcodeNOT,         .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b1111111111111111 },
    { .name = "neg",    .opcode = kEmex64OpcodeNEG,         .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b1111111111111111 },
    { .name = "and",    .opcode = kEmex64OpcodeAND,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "or",     .opcode = kEmex64OpcodeOR,          .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "xor",    .opcode = kEmex64OpcodeXOR,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "shr",    .opcode = kEmex64OpcodeSHR,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "shl",    .opcode = kEmex64OpcodeSHL,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "sar",    .opcode = kEmex64OpcodeSAR,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "ror",    .opcode = kEmex64OpcodeROR,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "rol",    .opcode = kEmex64OpcodeROL,         .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "pdep",   .opcode = kEmex64OpcodePDEP,        .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "pext",   .opcode = kEmex64OpcodePEXT,        .minargs = 2, .maxargs = 3,                 .argmask = 0b1000000000000000 },
    { .name = "bswapw", .opcode = kEmex64OpcodeBSWAPW,      .minargs = 1, .maxargs = 1,                 .argmask = 0b1000000000000000 },
    { .name = "bswapd", .opcode = kEmex64OpcodeBSWAPD,      .minargs = 1, .maxargs = 1,                 .argmask = 0b1000000000000000 },
    { .name = "bswapq", .opcode = kEmex64OpcodeBSWAPQ,      .minargs = 1, .maxargs = 1,                 .argmask = 0b1000000000000000 },
    { .name = "inc",    .opcode = kEmex64OpcodeINC,         .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b1111111111111111 },
    { .name = "dec",    .opcode = kEmex64OpcodeDEC,         .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b1111111111111111 },

    /* contol flow operations */
    { .name = "b",      .opcode = kEmex64OpcodeB,           .minargs = 1, .maxargs = 1,                 .argmask = 0b0000000000000000 },
    { .name = "cmp",    .opcode = kEmex64OpcodeCMP,         .minargs = 2, .maxargs = 2,                 .argmask = 0b0000000000000000 },
    { .name = "be",     .opcode = kEmex64OpcodeBE,          .minargs = 1, .maxargs = 1,                 .argmask = 0b0000000000000000 },
    { .name = "bne",    .opcode = kEmex64OpcodeBNE,         .minargs = 1, .maxargs = 1,                 .argmask = 0b0000000000000000 },
    { .name = "blt",    .opcode = kEmex64OpcodeBLT,         .minargs = 1, .maxargs = 1,                 .argmask = 0b0000000000000000 },
    { .name = "bgt",    .opcode = kEmex64OpcodeBGT,         .minargs = 1, .maxargs = 1,                 .argmask = 0b0000000000000000 },
    { .name = "ble",    .opcode = kEmex64OpcodeBLE,         .minargs = 1, .maxargs = 1,                 .argmask = 0b0000000000000000 },
    { .name = "bge",    .opcode = kEmex64OpcodeBGE,         .minargs = 1, .maxargs = 1,                 .argmask = 0b0000000000000000 },
    { .name = "bz",     .opcode = kEmex64OpcodeBZ,          .minargs = 2, .maxargs = 2,                 .argmask = 0b0000000000000000 },
    { .name = "bnz",    .opcode = kEmex64OpcodeBNZ,         .minargs = 2, .maxargs = 2,                 .argmask = 0b0000000000000000 },
    { .name = "blw",    .opcode = kEmex64OpcodeBLW,         .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b0000000000000000 },
    { .name = "wret",   .opcode = kEmex64OpcodeWRET,        .minargs = 0, .maxargs = 0,                 .argmask = 0b0000000000000000 },
    { .name = "iret",   .opcode = kEmex64OpcodeIRET,        .minargs = 0, .maxargs = 0,                 .argmask = 0b0000000000000000 },
    { .name = "bl",     .opcode = kEmex64OpcodeBL,          .minargs = 1, .maxargs = 1,                 .argmask = 0b0000000000000000 },
    { .name = "ret",    .opcode = kEmex64OpcodeRET,         .minargs = 0, .maxargs = 0,                 .argmask = 0b0000000000000000 },

    /* data operations v2 */
    { .name = "clr",    .opcode = kEmex64OpcodeCLR,         .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b1111111111111111 },
    { .name = "cmov",   .opcode = kEmex64OpcodeCMOV,        .minargs = 2, .maxargs = 2,                 .argmask = 0b1000000000000000 },
    { .name = "cmovb",  .opcode = kEmex64OpcodeCMOVB,       .minargs = 2, .maxargs = 2,                 .argmask = 0b1000000000000000 },
};

const opcode_entry_t *opcode_from_string(const char *name)
{
    /* null pointer check */
    if(name == NULL)
    {
        return NULL;
    }

    /* iterating through table */
    for(unsigned char opcode = 0x00; opcode < (sizeof(opcode_table) / sizeof(opcode_table[0])); opcode++)
    {
        /* check if opcode name matches */
        if(strcmp(opcode_table[opcode].name, name) == 0)
        {
            return &opcode_table[opcode];
        }
    }

    /* shouldnt happen if code is correct */
    return NULL;
}

bool opcode_arg_accepts_reg_only(const opcode_entry_t *opce,
                                 uint8_t arg)
{
    /* null pointer check */
    if(opce == NULL)
    {
        return false;
    }

    /* lol how tiny that operation is */
    return (opce->argmask & (1u << (31 - arg))) != 0;
}
