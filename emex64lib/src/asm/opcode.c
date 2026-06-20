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

#include <emex64lib/support/pack.h>

#include <emex64lib/asm/opcode.h>
#include <emex64lib/asm/emit.h>

const opcode_entry_t opcode_table[] = {
    /* core operations */
    [kEmex64OpcodeHLT] = { .opcode = kEmex64OpcodeHLT,      .minargs = 0, .maxargs = 0,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeNOP] = { .opcode = kEmex64OpcodeNOP,      .minargs = 0, .maxargs = 0,                 .argmask = 0b00000000000000000000000000000000 },

    /* data operations */
    [kEmex64OpcodeMOV] = { .opcode = kEmex64OpcodeMOV,      .minargs = 2, .maxargs = 2,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeSWP] = { .opcode = kEmex64OpcodeSWP,      .minargs = 2, .maxargs = 2,                 .argmask = 0b11000000000000000000000000000000 },
    [kEmex64OpcodeSWPZ] = { .opcode = kEmex64OpcodeSWPZ,    .minargs = 2, .maxargs = 2,                 .argmask = 0b11000000000000000000000000000000 },
    [kEmex64OpcodePUSH] = { .opcode = kEmex64OpcodePUSH,    .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodePOP] = { .opcode = kEmex64OpcodePOP,      .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b11111111111111111111111111111111 },
    [kEmex64OpcodeLDB] = { .opcode = kEmex64OpcodeLDB,      .minargs = 2, .maxargs = 2,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeLDW] = { .opcode = kEmex64OpcodeLDW,      .minargs = 2, .maxargs = 2,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeLDD] = { .opcode = kEmex64OpcodeLDD,      .minargs = 2, .maxargs = 2,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeLDQ] = { .opcode = kEmex64OpcodeLDQ,      .minargs = 2, .maxargs = 2,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeSTB] = { .opcode = kEmex64OpcodeSTB,      .minargs = 2, .maxargs = 2,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeSTW] = { .opcode = kEmex64OpcodeSTW,      .minargs = 2, .maxargs = 2,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeSTD] = { .opcode = kEmex64OpcodeSTD,      .minargs = 2, .maxargs = 2,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeSTQ] = { .opcode = kEmex64OpcodeSTQ,      .minargs = 2, .maxargs = 2,                 .argmask = 0b00000000000000000000000000000000 },

    /* alu operations */
    [kEmex64OpcodeADD] = { .opcode = kEmex64OpcodeADD,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeSUB] = { .opcode = kEmex64OpcodeSUB,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeMUL] = { .opcode = kEmex64OpcodeMUL,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeDIV] = { .opcode = kEmex64OpcodeDIV,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeIDIV] = { .opcode = kEmex64OpcodeIDIV,    .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeMOD] = { .opcode = kEmex64OpcodeMOD,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeNOT] = { .opcode = kEmex64OpcodeNOT,      .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b11111111111111111111111111111111 },
    [kEmex64OpcodeNEG] = { .opcode = kEmex64OpcodeNEG,      .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b11111111111111111111111111111111 },
    [kEmex64OpcodeAND] = { .opcode = kEmex64OpcodeAND,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeOR] = { .opcode = kEmex64OpcodeOR,        .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeXOR] = { .opcode = kEmex64OpcodeXOR,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeSHR] = { .opcode = kEmex64OpcodeSHR,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeSHL] = { .opcode = kEmex64OpcodeSHL,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeSAR] = { .opcode = kEmex64OpcodeSAR,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeROR] = { .opcode = kEmex64OpcodeROR,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeROL] = { .opcode = kEmex64OpcodeROL,      .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodePDEP] = { .opcode = kEmex64OpcodePDEP,    .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodePEXT] = { .opcode = kEmex64OpcodePEXT,    .minargs = 2, .maxargs = 3,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeBSWAPW] = { .opcode = kEmex64OpcodeBSWAPW,.minargs = 1, .maxargs = 1,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeBSWAPD] = { .opcode = kEmex64OpcodeBSWAPD,.minargs = 1, .maxargs = 1,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeBSWAPQ] = { .opcode = kEmex64OpcodeBSWAPQ,.minargs = 1, .maxargs = 1,                 .argmask = 0b10000000000000000000000000000000 },
    [kEmex64OpcodeINC] = { .opcode = kEmex64OpcodeINC,      .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b11111111111111111111111111111111 },
    [kEmex64OpcodeDEC] = { .opcode = kEmex64OpcodeDEC,      .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b11111111111111111111111111111111 },

    /* contol flow operations */
    [kEmex64OpcodeB] = { .opcode = kEmex64OpcodeB,          .minargs = 1, .maxargs = 1,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeCMP] = { .opcode = kEmex64OpcodeCMP,      .minargs = 2, .maxargs = 2,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeBE] = { .opcode = kEmex64OpcodeBE,        .minargs = 1, .maxargs = 1,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeBNE] = { .opcode = kEmex64OpcodeBNE,      .minargs = 1, .maxargs = 1,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeBLT] = { .opcode = kEmex64OpcodeBLT,      .minargs = 1, .maxargs = 1,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeBGT] = { .opcode = kEmex64OpcodeBGT,      .minargs = 1, .maxargs = 1,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeBLE] = { .opcode = kEmex64OpcodeBLE,      .minargs = 1, .maxargs = 1,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeBGE] = { .opcode = kEmex64OpcodeBGE,      .minargs = 1, .maxargs = 1,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeBZ] = { .opcode = kEmex64OpcodeBZ,        .minargs = 2, .maxargs = 2,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeBNZ] = { .opcode = kEmex64OpcodeBNZ,      .minargs = 2, .maxargs = 2,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeBLW] = { .opcode = kEmex64OpcodeBLW,      .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeWRET] = { .opcode = kEmex64OpcodeWRET,    .minargs = 0, .maxargs = 0,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeIRET] = { .opcode = kEmex64OpcodeIRET,    .minargs = 0, .maxargs = 0,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeBL] = { .opcode = kEmex64OpcodeBL,        .minargs = 1, .maxargs = 1,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeRET] = { .opcode = kEmex64OpcodeRET,      .minargs = 0, .maxargs = 0,                 .argmask = 0b00000000000000000000000000000000 },

    /* data operations v2 */
    [kEmex64OpcodeCLR] = { .opcode = kEmex64OpcodeCLR,      .minargs = 1, .maxargs = EMEX64_MAX_ARGS,   .argmask = 0b11111111111111111111111111111111 },
    [kEmex64OpcodeCMOV] = { .opcode = kEmex64OpcodeCMOV,    .minargs = 2, .maxargs = 2,                 .argmask = 0b00000000000000000000000000000000 },
    [kEmex64OpcodeCMOVB] = { .opcode = kEmex64OpcodeCMOVB,  .minargs = 2, .maxargs = 2,                 .argmask = 0b10000000000000000000000000000000 },
};

const opcode_entry_t *opcode_from_string(const char *name)
{
    if(name == NULL)
    {
        return NULL;
    }

    switch(pack_name(name))
    {
        case PACK1('b'): return &opcode_table[kEmex64OpcodeB];
        case PACK2('b','e'): return &opcode_table[kEmex64OpcodeBE];
        case PACK2('b','l'): return &opcode_table[kEmex64OpcodeBL];
        case PACK2('b','z'): return &opcode_table[kEmex64OpcodeBZ];
        case PACK2('o','r'): return &opcode_table[kEmex64OpcodeOR];
        case PACK3('a','d','d'): return &opcode_table[kEmex64OpcodeADD];
        case PACK3('a','n','d'): return &opcode_table[kEmex64OpcodeAND];
        case PACK3('b','g','e'): return &opcode_table[kEmex64OpcodeBGE];
        case PACK3('b','g','t'): return &opcode_table[kEmex64OpcodeBGT];
        case PACK3('b','l','e'): return &opcode_table[kEmex64OpcodeBLE];
        case PACK3('b','l','t'): return &opcode_table[kEmex64OpcodeBLT];
        case PACK3('b','l','w'): return &opcode_table[kEmex64OpcodeBLW];
        case PACK3('b','n','e'): return &opcode_table[kEmex64OpcodeBNE];
        case PACK3('b','n','z'): return &opcode_table[kEmex64OpcodeBNZ];
        case PACK3('c','l','r'): return &opcode_table[kEmex64OpcodeCLR];
        case PACK3('c','m','p'): return &opcode_table[kEmex64OpcodeCMP];
        case PACK3('d','e','c'): return &opcode_table[kEmex64OpcodeDEC];
        case PACK3('d','i','v'): return &opcode_table[kEmex64OpcodeDIV];
        case PACK3('h','l','t'): return &opcode_table[kEmex64OpcodeHLT];
        case PACK3('i','n','c'): return &opcode_table[kEmex64OpcodeINC];
        case PACK3('l','d','b'): return &opcode_table[kEmex64OpcodeLDB];
        case PACK3('l','d','d'): return &opcode_table[kEmex64OpcodeLDD];
        case PACK3('l','d','q'): return &opcode_table[kEmex64OpcodeLDQ];
        case PACK3('l','d','w'): return &opcode_table[kEmex64OpcodeLDW];
        case PACK3('m','u','l'): return &opcode_table[kEmex64OpcodeMUL];
        case PACK3('m','o','d'): return &opcode_table[kEmex64OpcodeMOD];
        case PACK3('m','o','v'): return &opcode_table[kEmex64OpcodeMOV];
        case PACK3('n','e','g'): return &opcode_table[kEmex64OpcodeNEG];
        case PACK3('n','o','p'): return &opcode_table[kEmex64OpcodeNOP];
        case PACK3('n','o','t'): return &opcode_table[kEmex64OpcodeNOT];
        case PACK3('p','o','p'): return &opcode_table[kEmex64OpcodePOP];
        case PACK3('r','e','t'): return &opcode_table[kEmex64OpcodeRET];
        case PACK3('r','o','l'): return &opcode_table[kEmex64OpcodeROL];
        case PACK3('r','o','r'): return &opcode_table[kEmex64OpcodeROR];
        case PACK3('s','a','r'): return &opcode_table[kEmex64OpcodeSAR];
        case PACK3('s','h','r'): return &opcode_table[kEmex64OpcodeSHR];
        case PACK3('s','h','l'): return &opcode_table[kEmex64OpcodeSHL];
        case PACK3('s','t','b'): return &opcode_table[kEmex64OpcodeSTB];
        case PACK3('s','t','d'): return &opcode_table[kEmex64OpcodeSTD];
        case PACK3('s','t','q'): return &opcode_table[kEmex64OpcodeSTQ];
        case PACK3('s','t','w'): return &opcode_table[kEmex64OpcodeSTW];
        case PACK3('s','u','b'): return &opcode_table[kEmex64OpcodeSUB];
        case PACK3('s','w','p'): return &opcode_table[kEmex64OpcodeSWP];
        case PACK3('x','o','r'): return &opcode_table[kEmex64OpcodeXOR];
        case PACK4('c','m','o','v'): return &opcode_table[kEmex64OpcodeCMOV];
        case PACK4('i','d','i','v'): return &opcode_table[kEmex64OpcodeIDIV];
        case PACK4('i','r','e','t'): return &opcode_table[kEmex64OpcodeIRET];
        case PACK4('p','d','e','p'): return &opcode_table[kEmex64OpcodePDEP];
        case PACK4('p','e','x','t'): return &opcode_table[kEmex64OpcodePEXT];
        case PACK4('p','u','s','h'): return &opcode_table[kEmex64OpcodePUSH];
        case PACK4('s','w','p','z'): return &opcode_table[kEmex64OpcodeSWPZ];
        case PACK4('w','r','e','t'): return &opcode_table[kEmex64OpcodeWRET];
        case PACK5('c','m','o','v','b'): return &opcode_table[kEmex64OpcodeCMOVB];
        case PACK6('b','s','w','a','p','d'): return &opcode_table[kEmex64OpcodeBSWAPD];
        case PACK6('b','s','w','a','p','q'): return &opcode_table[kEmex64OpcodeBSWAPQ];
        case PACK6('b','s','w','a','p','w'): return &opcode_table[kEmex64OpcodeBSWAPW];
        default: return NULL;
    }
}

bool opcode_arg_accepts_reg_only(const opcode_entry_t *opce,
                                 uint8_t arg)
{
    if(opce == NULL)
    {
        return false;
    }

    /* lol how tiny that operation is */
    return (opce->argmask & (1u << (31 - arg))) != 0;
}
