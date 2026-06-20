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
        case PACK1('b'): return kEmex64OpcodeB;
        case PACK2('b','e'): return kEmex64OpcodeBE;
        case PACK2('b','l'): return kEmex64OpcodeBL;
        case PACK2('b','z'): return kEmex64OpcodeBZ;
        case PACK2('o','r'): return kEmex64OpcodeOR;
        case PACK3('a','d','d'): return kEmex64OpcodeADD;
        case PACK3('a','n','d'): return kEmex64OpcodeAND;
        case PACK3('b','g','e'): return kEmex64OpcodeBGE;
        case PACK3('b','g','t'): return kEmex64OpcodeBGT;
        case PACK3('b','l','e'): return kEmex64OpcodeBLE;
        case PACK3('b','l','t'): return kEmex64OpcodeBLT;
        case PACK3('b','l','w'): return kEmex64OpcodeBLW;
        case PACK3('b','n','e'): return kEmex64OpcodeBNE;
        case PACK3('b','n','z'): return kEmex64OpcodeBNZ;
        case PACK3('c','l','r'): return kEmex64OpcodeCLR;
        case PACK3('c','m','p'): return kEmex64OpcodeCMP;
        case PACK3('d','e','c'): return kEmex64OpcodeDEC;
        case PACK3('d','i','v'): return kEmex64OpcodeDIV;
        case PACK3('h','l','t'): return kEmex64OpcodeHLT;
        case PACK3('i','n','c'): return kEmex64OpcodeINC;
        case PACK3('l','d','b'): return kEmex64OpcodeLDB;
        case PACK3('l','d','d'): return kEmex64OpcodeLDD;
        case PACK3('l','d','q'): return kEmex64OpcodeLDQ;
        case PACK3('l','d','w'): return kEmex64OpcodeLDW;
        case PACK3('m','u','l'): return kEmex64OpcodeMUL;
        case PACK3('m','o','d'): return kEmex64OpcodeMOD;
        case PACK3('m','o','v'): return kEmex64OpcodeMOV;
        case PACK3('n','e','g'): return kEmex64OpcodeNEG;
        case PACK3('n','o','p'): return kEmex64OpcodeNOP;
        case PACK3('n','o','t'): return kEmex64OpcodeNOT;
        case PACK3('p','o','p'): return kEmex64OpcodePOP;
        case PACK3('r','e','t'): return kEmex64OpcodeRET;
        case PACK3('r','o','l'): return kEmex64OpcodeROL;
        case PACK3('r','o','r'): return kEmex64OpcodeROR;
        case PACK3('s','a','r'): return kEmex64OpcodeSAR;
        case PACK3('s','h','r'): return kEmex64OpcodeSHR;
        case PACK3('s','h','l'): return kEmex64OpcodeSHL;
        case PACK3('s','t','b'): return kEmex64OpcodeSTB;
        case PACK3('s','t','d'): return kEmex64OpcodeSTD;
        case PACK3('s','t','q'): return kEmex64OpcodeSTQ;
        case PACK3('s','t','w'): return kEmex64OpcodeSTW;
        case PACK3('s','u','b'): return kEmex64OpcodeSUB;
        case PACK3('s','w','p'): return kEmex64OpcodeSWP;
        case PACK3('x','o','r'): return kEmex64OpcodeXOR;
        case PACK4('c','m','o','v'): return kEmex64OpcodeCMOV;
        case PACK4('i','d','i','v'): return kEmex64OpcodeIDIV;
        case PACK4('i','r','e','t'): return kEmex64OpcodeIRET;
        case PACK4('p','d','e','p'): return kEmex64OpcodePDEP;
        case PACK4('p','e','x','t'): return kEmex64OpcodePEXT;
        case PACK4('p','u','s','h'): return kEmex64OpcodePUSH;
        case PACK4('s','w','p','z'): return kEmex64OpcodeSWPZ;
        case PACK4('w','r','e','t'): return kEmex64OpcodeWRET;
        case PACK5('c','m','o','v','b'): return kEmex64OpcodeCMOVB;
        case PACK6('b','s','w','a','p','d'): return kEmex64OpcodeBSWAPD;
        case PACK6('b','s','w','a','p','q'): return kEmex64OpcodeBSWAPQ;
        case PACK6('b','s','w','a','p','w'): return kEmex64OpcodeBSWAPW;
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
