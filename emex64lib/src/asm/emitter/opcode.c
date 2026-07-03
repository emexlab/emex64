/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <emex64lib/support/pack.h>
#include <emex64lib/asm/emitter/opcode.h>

kEmex64Opcode opcode_from_string(const char *name)
{
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
        default: return kEmex64OpcodeInvalid;
    }
}

bool opcode_arg_accepts_reg_only(const emex64_opfunc_entry_t *opce,
                                 uint8_t arg)
{
    return opce != NULL && (opce->argmask & (1u << (31 - arg))) != 0;
}

void assembler_emit_opcode(assembler_invocation_t *inv,
                           kEmex64Opcode op)
{
    vbitwalker_write(inv->out_vbitwalker, op, 8);
}
