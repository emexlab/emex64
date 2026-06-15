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
    size_t name_len = strlen(name);

    if(name_len == 1)
    {
        if(name[0] == 'b')
        {
            return &opcode_table[kEmex64OpcodeB];
        }
        return NULL;
    }
    else if(name_len == 2)
    {
        switch(name[0])
        {
            case 'b':
                switch(name[1])
                {
                    case 'e': return &opcode_table[kEmex64OpcodeBE];
                    case 'l': return &opcode_table[kEmex64OpcodeBL];
                    case 'z': return &opcode_table[kEmex64OpcodeBZ];
                }
                return NULL;
            case 'o':
                switch(name[1])
                {
                    case 'r': return &opcode_table[kEmex64OpcodeOR];
                }
                return NULL;
        }
        return NULL;
    }
    else if(name_len == 3)
    {
        switch(name[0])
        {
            case 'a':
                switch(name[1])
                {
                    case 'd':
                        switch(name[2])
                        {
                            case 'd': return &opcode_table[kEmex64OpcodeADD];
                        }
                        return NULL;
                    case 'n':
                        switch(name[2])
                        {
                            case 'd': return &opcode_table[kEmex64OpcodeAND];
                        }
                        return NULL;
                }
                return NULL;
            case 'b':
                switch(name[1])
                {
                    case 'g':
                        switch(name[2])
                        {
                            case 'e': return &opcode_table[kEmex64OpcodeBGE];
                            case 't': return &opcode_table[kEmex64OpcodeBGT];
                        }
                        return NULL;
                    case 'l':
                        switch(name[2])
                        {
                            case 'e': return &opcode_table[kEmex64OpcodeBLE];
                            case 't': return &opcode_table[kEmex64OpcodeBLT];
                            case 'w': return &opcode_table[kEmex64OpcodeBLW];
                        }
                        return NULL;
                    case 'n':
                        switch(name[2])
                        {
                            case 'e': return &opcode_table[kEmex64OpcodeBNE];
                            case 'z': return &opcode_table[kEmex64OpcodeBNZ];
                        }
                        return NULL;
                }
                return NULL;
            case 'c':
                switch(name[1])
                {
                    case 'l':
                        switch(name[2])
                        {
                            case 'r': return &opcode_table[kEmex64OpcodeCLR];
                        }
                        return NULL;
                    case 'm':
                        switch(name[2])
                        {
                            case 'p': return &opcode_table[kEmex64OpcodeCMP];
                        }
                        return NULL;
                }
                return NULL;
            case 'd':
                switch(name[1])
                {
                    case 'e':
                        switch(name[2])
                        {
                            case 'c': return &opcode_table[kEmex64OpcodeDEC];
                        }
                        return NULL;
                    case 'i':
                        switch(name[2])
                        {
                            case 'v': return &opcode_table[kEmex64OpcodeDIV];
                        }
                        return NULL;
                }
                return NULL;
            case 'h':
                switch(name[1])
                {
                    case 'l':
                        switch(name[2])
                        {
                            case 't': return &opcode_table[kEmex64OpcodeHLT];
                        }
                        return NULL;
                }
                return NULL;
            case 'i':
                switch(name[1])
                {
                    case 'n':
                        switch(name[2])
                        {
                            case 'c': return &opcode_table[kEmex64OpcodeINC];
                        }
                        return NULL;
                }
                return NULL;
            case 'l':
                switch(name[1])
                {
                    case 'd':
                        switch(name[2])
                        {
                            case 'b': return &opcode_table[kEmex64OpcodeLDB];
                            case 'd': return &opcode_table[kEmex64OpcodeLDD];
                            case 'q': return &opcode_table[kEmex64OpcodeLDQ];
                            case 'w': return &opcode_table[kEmex64OpcodeLDW];
                        }
                        return NULL;
                }
                return NULL;
            case 'm':
                switch(name[1])
                {
                    case 'u':
                        switch(name[2])
                        {
                            case 'l': return &opcode_table[kEmex64OpcodeMUL];
                        }
                        return NULL;
                    case 'o':
                        switch(name[2])
                        {
                            case 'd': return &opcode_table[kEmex64OpcodeMOD];
                            case 'v': return &opcode_table[kEmex64OpcodeMOV];
                        }
                        return NULL;
                }
                return NULL;
            case 'n':
                switch(name[1])
                {
                    case 'e':
                        switch(name[2])
                        {
                            case 'g': return &opcode_table[kEmex64OpcodeNEG];
                        }
                        return NULL;
                    case 'o':
                        switch(name[2])
                        {
                            case 'p': return &opcode_table[kEmex64OpcodeNOP];
                            case 't': return &opcode_table[kEmex64OpcodeNOT];
                        }
                        return NULL;
                }
                return NULL;
            case 'p':
                switch(name[1])
                {
                    case 'o':
                        switch(name[2])
                        {
                            case 'p': return &opcode_table[kEmex64OpcodePOP];
                        }
                        return NULL;
                }
                return NULL;
            case 'r':
                switch(name[1])
                {
                    case 'e':
                        switch(name[2])
                        {
                            case 't': return &opcode_table[kEmex64OpcodeRET];
                        }
                        return NULL;
                    case 'o':
                        switch(name[2])
                        {
                            case 'l': return &opcode_table[kEmex64OpcodeROL];
                            case 'r': return &opcode_table[kEmex64OpcodeROR];
                        }
                        return NULL;
                }
                return NULL;
            case 's':
                switch(name[1])
                {
                    case 'a':
                        switch(name[2])
                        {
                            case 'r': return &opcode_table[kEmex64OpcodeSAR];
                        }
                        return NULL;
                    case 'h':
                        switch(name[2])
                        {
                            case 'r': return &opcode_table[kEmex64OpcodeSHR];
                            case 'l': return &opcode_table[kEmex64OpcodeSHL];
                        }
                        return NULL;
                    case 't':
                        switch(name[2])
                        {
                            case 'b': return &opcode_table[kEmex64OpcodeSTB];
                            case 'd': return &opcode_table[kEmex64OpcodeSTD];
                            case 'q': return &opcode_table[kEmex64OpcodeSTQ];
                            case 'w': return &opcode_table[kEmex64OpcodeSTW];
                        }
                        return NULL;
                    case 'u':
                        switch(name[2])
                        {
                            case 'b': return &opcode_table[kEmex64OpcodeSUB];
                        }
                        return NULL;
                    case 'w':
                        switch(name[2])
                        {
                            case 'p': return &opcode_table[kEmex64OpcodeSWP];
                        }
                        return NULL;
                }
                return NULL;
            case 'x':
                switch(name[1])
                {
                    case 'o':
                        switch(name[2])
                        {
                            case 'r': return &opcode_table[kEmex64OpcodeXOR];
                        }
                        return NULL;
                }
                return NULL;
        }
        return NULL;
    }
    else if(name_len == 4)
    {
        switch(name[0])
        {
            case 'c':
                switch(name[1])
                {
                    case 'm':
                        switch(name[2])
                        {
                            case 'o':
                                switch(name[3])
                                {
                                    case 'v': return &opcode_table[kEmex64OpcodeCMOV];
                                }
                                return NULL;
                        }
                        return NULL;
                }
                return NULL;
            case 'i':
                switch(name[1])
                {
                    case 'd':
                        switch(name[2])
                        {
                            case 'i':
                                switch(name[3])
                                {
                                    case 'v': return &opcode_table[kEmex64OpcodeIDIV];
                                }
                                return NULL;
                        }
                        return NULL;
                    case 'r':
                        switch(name[2])
                        {
                            case 'e':
                                switch(name[3])
                                {
                                    case 't': return &opcode_table[kEmex64OpcodeIRET];
                                }
                                return NULL;
                        }
                        return NULL;
                }
                return NULL;
            case 'p':
                switch(name[1])
                {
                    case 'd':
                        switch(name[2])
                        {
                            case 'e':
                                switch(name[3])
                                {
                                    case 'p': return &opcode_table[kEmex64OpcodePDEP];
                                }
                                return NULL;
                        }
                        return NULL;
                    case 'e':
                        switch(name[2])
                        {
                            case 'x':
                                switch(name[3])
                                {
                                    case 't': return &opcode_table[kEmex64OpcodePEXT];
                                }
                                return NULL;
                        }
                        return NULL;
                    case 'u':
                        switch(name[2])
                        {
                            case 's':
                                switch(name[3])
                                {
                                    case 'h': return &opcode_table[kEmex64OpcodePUSH];
                                }
                                return NULL;
                        }
                        return NULL;
                }
                return NULL;
            case 's':
                switch(name[1])
                {
                    case 'w':
                        switch(name[2])
                        {
                            case 'p':
                                switch(name[3])
                                {
                                    case 'z': return &opcode_table[kEmex64OpcodeSWPZ];
                                }
                                return NULL;
                        }
                        return NULL;
                }
                return NULL;
            case 'w':
                switch(name[1])
                {
                    case 'r':
                        switch(name[2])
                        {
                            case 'e':
                                switch(name[3])
                                {
                                    case 't': return &opcode_table[kEmex64OpcodeWRET];
                                }
                                return NULL;
                        }
                        return NULL;
                }
                return NULL;
        }
        return NULL;
    }
    else if(name_len == 5)
    {
        switch(name[0])
        {
            case 'c':
                switch(name[1])
                {
                    case 'm':
                        switch(name[2])
                        {
                            case 'o':
                                switch(name[3])
                                {
                                    case 'v':
                                        switch(name[4])
                                        {
                                            case 'b': return &opcode_table[kEmex64OpcodeCMOVB];
                                        }
                                        return NULL;
                                }
                                return NULL;
                        }
                        return NULL;
                }
                return NULL;
        }
        return NULL;
    }
    else if(name_len == 6)
    {
        switch(name[0])
        {
            case 'b':
                switch(name[1])
                {
                    case 's':
                        switch(name[2])
                        {
                            case 'w':
                                switch(name[3])
                                {
                                    case 'a':
                                        switch(name[4])
                                        {
                                            case 'p':
                                                switch(name[5])
                                                {
                                                    case 'd': return &opcode_table[kEmex64OpcodeBSWAPD];
                                                    case 'q': return &opcode_table[kEmex64OpcodeBSWAPQ];
                                                    case 'w': return &opcode_table[kEmex64OpcodeBSWAPW];
                                                }
                                        }
                                        return NULL;
                                }
                                return NULL;
                        }
                        return NULL;
                }
                return NULL;
        }
        return NULL;
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
