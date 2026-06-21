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

enum kEmex64Opcode opcode_from_string(const char *name,
                                      bool *success)
{
    if(name == NULL)
    {
        *success = false;
        return kEmex64OpcodeHLT;
    }

    *success = true;

    switch(pack_name(name))
    {
        case PACK('b'): return kEmex64OpcodeB;
        case PACK('b','e'): return kEmex64OpcodeBE;
        case PACK('b','l'): return kEmex64OpcodeBL;
        case PACK('b','z'): return kEmex64OpcodeBZ;
        case PACK('o','r'): return kEmex64OpcodeOR;
        case PACK('a','d','d'): return kEmex64OpcodeADD;
        case PACK('a','n','d'): return kEmex64OpcodeAND;
        case PACK('b','g','e'): return kEmex64OpcodeBGE;
        case PACK('b','g','t'): return kEmex64OpcodeBGT;
        case PACK('b','l','e'): return kEmex64OpcodeBLE;
        case PACK('b','l','t'): return kEmex64OpcodeBLT;
        case PACK('b','l','w'): return kEmex64OpcodeBLW;
        case PACK('b','n','e'): return kEmex64OpcodeBNE;
        case PACK('b','n','z'): return kEmex64OpcodeBNZ;
        case PACK('c','l','r'): return kEmex64OpcodeCLR;
        case PACK('c','m','p'): return kEmex64OpcodeCMP;
        case PACK('d','e','c'): return kEmex64OpcodeDEC;
        case PACK('d','i','v'): return kEmex64OpcodeDIV;
        case PACK('h','l','t'): return kEmex64OpcodeHLT;
        case PACK('i','n','c'): return kEmex64OpcodeINC;
        case PACK('l','d','b'): return kEmex64OpcodeLDB;
        case PACK('l','d','d'): return kEmex64OpcodeLDD;
        case PACK('l','d','q'): return kEmex64OpcodeLDQ;
        case PACK('l','d','w'): return kEmex64OpcodeLDW;
        case PACK('m','u','l'): return kEmex64OpcodeMUL;
        case PACK('m','o','d'): return kEmex64OpcodeMOD;
        case PACK('m','o','v'): return kEmex64OpcodeMOV;
        case PACK('n','e','g'): return kEmex64OpcodeNEG;
        case PACK('n','o','p'): return kEmex64OpcodeNOP;
        case PACK('n','o','t'): return kEmex64OpcodeNOT;
        case PACK('p','o','p'): return kEmex64OpcodePOP;
        case PACK('r','e','t'): return kEmex64OpcodeRET;
        case PACK('r','o','l'): return kEmex64OpcodeROL;
        case PACK('r','o','r'): return kEmex64OpcodeROR;
        case PACK('s','a','r'): return kEmex64OpcodeSAR;
        case PACK('s','h','r'): return kEmex64OpcodeSHR;
        case PACK('s','h','l'): return kEmex64OpcodeSHL;
        case PACK('s','t','b'): return kEmex64OpcodeSTB;
        case PACK('s','t','d'): return kEmex64OpcodeSTD;
        case PACK('s','t','q'): return kEmex64OpcodeSTQ;
        case PACK('s','t','w'): return kEmex64OpcodeSTW;
        case PACK('s','u','b'): return kEmex64OpcodeSUB;
        case PACK('s','w','p'): return kEmex64OpcodeSWP;
        case PACK('x','o','r'): return kEmex64OpcodeXOR;
        case PACK('c','m','o','v'): return kEmex64OpcodeCMOV;
        case PACK('i','d','i','v'): return kEmex64OpcodeIDIV;
        case PACK('i','r','e','t'): return kEmex64OpcodeIRET;
        case PACK('p','d','e','p'): return kEmex64OpcodePDEP;
        case PACK('p','e','x','t'): return kEmex64OpcodePEXT;
        case PACK('p','u','s','h'): return kEmex64OpcodePUSH;
        case PACK('s','w','p','z'): return kEmex64OpcodeSWPZ;
        case PACK('w','r','e','t'): return kEmex64OpcodeWRET;
        case PACK('c','m','o','v','b'): return kEmex64OpcodeCMOVB;
        case PACK('b','s','w','a','p','d'): return kEmex64OpcodeBSWAPD;
        case PACK('b','s','w','a','p','q'): return kEmex64OpcodeBSWAPQ;
        case PACK('b','s','w','a','p','w'): return kEmex64OpcodeBSWAPW;
        default:
            *success = false;
            return kEmex64OpcodeHLT;
    }
}

bool opcode_arg_accepts_reg_only(const emex64_opfunc_entry_t *opce,
                                 uint8_t arg)
{
    if(opce == NULL)
    {
        return false;
    }

    /* lol how tiny that operation is */
    return (opce->argmask & (1u << (31 - arg))) != 0;
}
