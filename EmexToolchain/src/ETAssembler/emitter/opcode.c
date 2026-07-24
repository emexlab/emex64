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

#include <EmexToolchain/Support/pack.h>
#include <EmexToolchain/ETAssembler/emitter/opcode.h>

E64Opcode opcode_from_string(const char *name)
{
    switch(pack_name(name))
    {
        case PACK('b'): return kE64OpcodeB;
        case PACK('b','e'): return kE64OpcodeBE;
        case PACK('b','l'): return kE64OpcodeBL;
        case PACK('b','z'): return kE64OpcodeBZ;
        case PACK('o','r'): return kE64OpcodeOR;
        case PACK('a','d','d'): return kE64OpcodeADD;
        case PACK('a','n','d'): return kE64OpcodeAND;
        case PACK('b','g','e'): return kE64OpcodeBGE;
        case PACK('b','g','t'): return kE64OpcodeBGT;
        case PACK('b','l','e'): return kE64OpcodeBLE;
        case PACK('b','l','t'): return kE64OpcodeBLT;
        case PACK('b','l','w'): return kE64OpcodeBLW;
        case PACK('b','n','e'): return kE64OpcodeBNE;
        case PACK('b','n','z'): return kE64OpcodeBNZ;
        case PACK('c','l','r'): return kE64OpcodeCLR;
        case PACK('c','m','p'): return kE64OpcodeCMP;
        case PACK('d','e','c'): return kE64OpcodeDEC;
        case PACK('d','i','v'): return kE64OpcodeDIV;
        case PACK('h','l','t'): return kE64OpcodeHLT;
        case PACK('i','n','c'): return kE64OpcodeINC;
        case PACK('l','d','b'): return kE64OpcodeLDB;
        case PACK('l','d','d'): return kE64OpcodeLDD;
        case PACK('l','d','q'): return kE64OpcodeLDQ;
        case PACK('l','d','w'): return kE64OpcodeLDW;
        case PACK('m','u','l'): return kE64OpcodeMUL;
        case PACK('m','o','d'): return kE64OpcodeMOD;
        case PACK('m','o','v'): return kE64OpcodeMOV;
        case PACK('n','e','g'): return kE64OpcodeNEG;
        case PACK('n','o','p'): return kE64OpcodeNOP;
        case PACK('n','o','t'): return kE64OpcodeNOT;
        case PACK('p','o','p'): return kE64OpcodePOP;
        case PACK('r','e','t'): return kE64OpcodeRET;
        case PACK('r','o','l'): return kE64OpcodeROL;
        case PACK('r','o','r'): return kE64OpcodeROR;
        case PACK('s','a','r'): return kE64OpcodeSAR;
        case PACK('s','h','r'): return kE64OpcodeSHR;
        case PACK('s','h','l'): return kE64OpcodeSHL;
        case PACK('s','t','b'): return kE64OpcodeSTB;
        case PACK('s','t','d'): return kE64OpcodeSTD;
        case PACK('s','t','q'): return kE64OpcodeSTQ;
        case PACK('s','t','w'): return kE64OpcodeSTW;
        case PACK('s','u','b'): return kE64OpcodeSUB;
        case PACK('s','w','p'): return kE64OpcodeSWP;
        case PACK('x','o','r'): return kE64OpcodeXOR;
        case PACK('b','b','z'): return kE64OpcodeBBZ;
        case PACK('c','m','o','v'): return kE64OpcodeCMOV;
        case PACK('i','d','i','v'): return kE64OpcodeIDIV;
        case PACK('i','r','e','t'): return kE64OpcodeIRET;
        case PACK('p','d','e','p'): return kE64OpcodePDEP;
        case PACK('p','e','x','t'): return kE64OpcodePEXT;
        case PACK('p','u','s','h'): return kE64OpcodePUSH;
        case PACK('m','o','v','z'): return kE64OpcodeMOVZ;
        case PACK('w','r','e','t'): return kE64OpcodeWRET;
        case PACK('b','b','n','z'): return kE64OpcodeBBNZ;
        case PACK('c','l','a','r'): return kE64OpcodeCLAR;
        case PACK('c','m','o','v','b'): return kE64OpcodeCMOVB;
        case PACK('b','s','w','a','p','d'): return kE64OpcodeBSWAPD;
        case PACK('b','s','w','a','p','q'): return kE64OpcodeBSWAPQ;
        case PACK('b','s','w','a','p','w'): return kE64OpcodeBSWAPW;
        default: return kE64OpcodeInvalid;
    }
}

Boolean opcode_arg_accepts_reg_only(const emex64_opfunc_entry_t *opce,
                                    UInt8 arg)
{
    return opce != NULL && (opce->argmask & (1u << (31 - arg))) != 0;
}

void assembler_emit_opcode(ETAssemblerInvocationRef inv,
                           E64Opcode op)
{
    EFBitWalkerWrite(inv->out_vbitwalker, op, 8);
}
