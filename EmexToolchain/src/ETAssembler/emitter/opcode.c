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

#include <EmexFoundation/EmexFoundation.h>
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
        case PACK('s','v','c'): return kE64OpcodeSVC;
        case PACK('c','a','s'): return kE64OpcodeCAS;
        case PACK('e','r','e','t'): return kE64OpcodeERET;
        case PACK('c','s','w','p'): return kE64OpcodeCSWP;
        case PACK('x','c','h','g'): return kE64OpcodeXCHG;
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
        case PACK('m','o','v','m'): return kE64OpcodeMOVM;
        case PACK('c','p','y','m'): return kE64OpcodeCPYM;
        case PACK('s','e','t','m'): return kE64OpcodeSETM;
        case PACK('c','m','o','v','b'): return kE64OpcodeCMOVB;
        case PACK('r','d','r','n','d'): return kE64OpcodeRDRND;
        case PACK('b','s','w','a','p','d'): return kE64OpcodeBSWAPD;
        case PACK('b','s','w','a','p','q'): return kE64OpcodeBSWAPQ;
        case PACK('b','s','w','a','p','w'): return kE64OpcodeBSWAPW;
        case PACK('f','a','d','d'): return kE64OpcodeFADD;
        case PACK('f','s','u','b'): return kE64OpcodeFSUB;
        case PACK('f','m','u','l'): return kE64OpcodeFMUL;
        case PACK('f','d','i','v'): return kE64OpcodeFDIV;
        case PACK('f','s','q','r','t'): return kE64OpcodeFSQRT;
        case PACK('f','m','i','n'): return kE64OpcodeFMIN;
        case PACK('f','m','a','x'): return kE64OpcodeFMAX;
        case PACK('f','m','a','d','d'): return kE64OpcodeFMADD;
        case PACK('f','n','e','g'): return kE64OpcodeFNEG;
        case PACK('f','a','b','s'): return kE64OpcodeFABS;
        case PACK('f','c','m','p'): return kE64OpcodeFCMP;
        case PACK('f','a','d','d','s'): return kE64OpcodeFADDS;
        case PACK('f','s','u','b','s'): return kE64OpcodeFSUBS;
        case PACK('f','m','u','l','s'): return kE64OpcodeFMULS;
        case PACK('f','d','i','v','s'): return kE64OpcodeFDIVS;
        case PACK('f','s','q','r','t','s'): return kE64OpcodeFSQRTS;
        case PACK('f','m','i','n','s'): return kE64OpcodeFMINS;
        case PACK('f','m','a','x','s'): return kE64OpcodeFMAXS;
        case PACK('f','m','a','d','d','s'): return kE64OpcodeFMADDS;
        case PACK('f','n','e','g','s'): return kE64OpcodeFNEGS;
        case PACK('f','a','b','s','s'): return kE64OpcodeFABSS;
        case PACK('f','c','m','p','s'): return kE64OpcodeFCMPS;
        case PACK('f','c','v','t','d','s'): return kE64OpcodeFCVTDS;
        case PACK('f','c','v','t','s','d'): return kE64OpcodeFCVTSD;
        case PACK('f','c','v','t','d','i'): return kE64OpcodeFCVTDI;
        case PACK('f','c','v','t','d','u'): return kE64OpcodeFCVTDU;
        case PACK('f','c','v','t','i','d'): return kE64OpcodeFCVTID;
        case PACK('f','c','v','t','u','d'): return kE64OpcodeFCVTUD;
        case PACK('f','c','v','t','s','i'): return kE64OpcodeFCVTSI;
        case PACK('f','c','v','t','s','u'): return kE64OpcodeFCVTSU;
        case PACK('f','c','v','t','i','s'): return kE64OpcodeFCVTIS;
        case PACK('f','c','v','t','u','s'): return kE64OpcodeFCVTUS;
        case PACK('s','e','x','t'): return kE64OpcodeSEXT;
        case PACK('c','b','l','t'): return kE64OpcodeCBLT;
        case PACK('c','l','z'): return kE64OpcodeCLZ;
        case PACK('c','t','z'): return kE64OpcodeCTZ;
        case PACK('p','o','p','c','n','t'): return kE64OpcodePOPCNT;
        case PACK('m','u','l','h'): return kE64OpcodeMULH;
        case PACK('u','m','u','l','h'): return kE64OpcodeUMULH;
        case PACK('a','d','c'): return kE64OpcodeADC;
        case PACK('s','b','b'): return kE64OpcodeSBB;
        case PACK('p','a','c','i','a'): return kE64OpcodePACIA;
        case PACK('a','u','t','i','a'): return kE64OpcodeAUTIA;
        case PACK('p','a','c','d','a'): return kE64OpcodePACDA;
        case PACK('a','u','t','d','a'): return kE64OpcodeAUTDA;
        case PACK('x','p','a','c'): return kE64OpcodeXPAC;
        case PACK('p','a','c','r','e','t'): return kE64OpcodePACRET;
        case PACK('r','e','t','a'): return kE64OpcodeRETA;
        case PACK('r','d','t','i','m','e'): return kE64OpcodeRDTIME;
        case PACK('w','f','e'): return kE64OpcodeWFE;
        case PACK('s','e','v'): return kE64OpcodeSEV;
        case PACK('p','a','u','s','e'): return kE64OpcodePAUSE;
        case PACK('c','b','e','q'): return kE64OpcodeCBEQ;
        case PACK('c','b','n','e'): return kE64OpcodeCBNE;
        case PACK('c','b','g','e'): return kE64OpcodeCBGE;
        case PACK('c','b','l','t','u'): return kE64OpcodeCBLTU;
        case PACK('c','b','g','e','u'): return kE64OpcodeCBGEU;
        case PACK('l','d','s','b'): return kE64OpcodeLDSB;
        case PACK('l','d','s','w'): return kE64OpcodeLDSW;
        case PACK('l','d','s','d'): return kE64OpcodeLDSD;
        case PACK('l','d','a','d','d'): return kE64OpcodeLDADD;
        case PACK('l','d','a','n','d'): return kE64OpcodeLDAND;
        case PACK('l','d','o','r'): return kE64OpcodeLDOR;
        case PACK('l','d','x','o','r'): return kE64OpcodeLDXOR;
        case PACK('s','w','p','a','d','d'): return kE64OpcodeSWPADD;
        case PACK('l','d','a','d','d','d'): return kE64OpcodeLDADDD;
        case PACK('l','d','a','n','d','d'): return kE64OpcodeLDANDD;
        case PACK('l','d','o','r','d'): return kE64OpcodeLDORD;
        case PACK('l','d','x','o','r','d'): return kE64OpcodeLDXORD;
        case PACK('s','w','p','a','d','d','d'): return kE64OpcodeSWPADDD;
        case PACK('t','l','b','i'): return kE64OpcodeTLBI;
        case PACK('a','d','d','d'): return kE64OpcodeADDD;
        case PACK('i','a','d','d','d'): return kE64OpcodeIADDD;
        case PACK('s','u','b','d'): return kE64OpcodeSUBD;
        case PACK('i','s','u','b','d'): return kE64OpcodeISUBD;
        case PACK('m','u','l','d'): return kE64OpcodeMULD;
        case PACK('i','m','u','l','d'): return kE64OpcodeIMULD;
        case PACK('s','h','l','d'): return kE64OpcodeSHLD;
        case PACK('i','s','h','l','d'): return kE64OpcodeISHLD;
        case PACK('s','h','r','d'): return kE64OpcodeSHRD;
        case PACK('s','a','r','d'): return kE64OpcodeSARD;
        case PACK('d','i','v','d'): return kE64OpcodeDIVD;
        case PACK('i','d','i','v','d'): return kE64OpcodeIDIVD;
        case PACK('m','o','d','d'): return kE64OpcodeMODD;
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
