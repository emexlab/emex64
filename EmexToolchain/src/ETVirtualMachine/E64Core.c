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

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETVirtualMachine/E64Core.h>
#include <EmexToolchain/ETVirtualMachine/E64Memory.h>
#include <EmexToolchain/ETVirtualMachine/E64Machine.h>
#include <EmexToolchain/ETVirtualMachine/device/internal/controller/E64IC.h>
#include <EmexToolchain/ETVirtualMachine/device/internal/timer.h>
#include <EmexToolchain/ETVirtualMachine/instruction/core.h>
#include <EmexToolchain/ETVirtualMachine/instruction/data.h>
#include <EmexToolchain/ETVirtualMachine/instruction/alu.h>
#include <EmexToolchain/ETVirtualMachine/instruction/ctrl.h>
#include <EmexToolchain/ETVirtualMachine/instruction/fpu.h>
#include <EmexToolchain/ETVirtualMachine/device/board/display.h>
#include <EmexToolchain/Support/bitbolt.h>
#include <EmexToolchain/Support/likely.h>

#include <time.h>

#if defined(__APPLE__)
#include <CoreFoundation/CFRunLoop.h>
#include <pthread/qos.h>
#endif /* __APPLE__ */

const emex64_opfunc_entry_t kE64OpfuncTable[] = {
    /* core operations */
    [kE64OpcodeHLT] = { .func = E64HLT, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeNOP] = { .func = E64NOP, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 15 },

    /* data operations */
    [kE64OpcodeMOV] = { .func = E64MOV, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeMOVZ] = { .func = E64MOVZ, .minargs = 2, .maxargs = 2, .argmask = 0b11000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeSWP] = { .func = E64SWP, .minargs = 2, .maxargs = 2, .argmask = 0b11000000000000000000000000000000, .isa = 15 },
    [kE64OpcodePUSH] = { .func = E64PUSH, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodePOP] = { .func = E64POP, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111, .isa = 15 },
    [kE64OpcodeLDB] = { .func = E64LDB, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeLDW] = { .func = E64LDW, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeLDD] = { .func = E64LDD, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeLDQ] = { .func = E64LDQ, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeSTB] = { .func = E64STB, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeSTW] = { .func = E64STW, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeSTD] = { .func = E64STD, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeSTQ] = { .func = E64STQ, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000, .isa = 15 },

    /* arithmetic operations */
    [kE64OpcodeADD] = { .func = E64ADD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeSUB] = { .func = E64SUB, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeMUL] = { .func = E64MUL, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeDIV] = { .func = E64DIV, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeIDIV] = { .func = E64IDIV, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeMOD] = { .func = E64MOD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeNOT] = { .func = E64NOT, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111, .isa = 15 },
    [kE64OpcodeNEG] = { .func = E64NEG, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111, .isa = 15 },
    [kE64OpcodeAND] = { .func = E64AND, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeOR] = { .func = E64OR, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeXOR] = { .func = E64XOR, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeSHR] = { .func = E64SHR, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeSHL] = { .func = E64SHL, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeSAR] = { .func = E64SAR, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeROR] = { .func = E64ROR, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeROL] = { .func = E64ROL, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodePDEP] = { .func = E64PDEP, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodePEXT] = { .func = E64PEXT, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBSWAPW] = { .func = E64BSWAPW, .minargs = 1, .maxargs = 1, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBSWAPD] = { .func = E64BSWAPD, .minargs = 1, .maxargs = 1, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBSWAPQ] = { .func = E64BSWAPQ, .minargs = 1, .maxargs = 1, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeINC] = { .func = E64INC, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111, .isa = 15 },
    [kE64OpcodeDEC] = { .func = E64DEC, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111, .isa = 15 },

    /* control flow operations */
    [kE64OpcodeB] = { .func = E64B, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeCMP] = { .func = E64CMP, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBE] = { .func = E64BE, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBNE] = { .func = E64BNE, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBLT] = { .func = E64BLT, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBGT] = { .func = E64BGT, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBLE] = { .func = E64BLE, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBGE] = { .func = E64BGE, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBZ] = { .func = E64BZ, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBNZ] = { .func = E64BNZ, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBLW] = { .func = E64BLW, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeWRET] = { .func = E64WRET, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeIRET] = { .func = E64IRET, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBL] = { .func = E64BL, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeRET] = { .func = E64RET, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 15 },

    /* data operations v2 */
    [kE64OpcodeCLR] = { .func = E64CLR, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111, .isa = 15 },
    [kE64OpcodeCMOV] = { .func = E64CMOV, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeCMOVB] = { .func = E64CMOVB, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeCLAR] = { .func = E64CLAR, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 15 },

    /* control flow operations v2 */
    [kE64OpcodeBBZ] = { .func = E64BBZ, .minargs = 3, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },
    [kE64OpcodeBBNZ] = { .func = E64BBNZ, .minargs = 3, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 15 },

    /* ISA v16 (data operations v3) */
    [kE64OpcodeRDRND] = { .func = E64RDRND, .minargs = 1, .maxargs = 1, .argmask = 0b10000000000000000000000000000000, .isa = 16 },
    [kE64OpcodeMOVM] = { .func = E64MOVM, .minargs = 3, .maxargs = 3, .argmask = 0b00000000000000000000000000000000, .isa = 16 },
    [kE64OpcodeCPYM] = { .func = E64CPYM, .minargs = 3, .maxargs = 3, .argmask = 0b00000000000000000000000000000000, .isa = 16 },
    [kE64OpcodeSETM] = { .func = E64SETM, .minargs = 3, .maxargs = 3, .argmask = 0b00000000000000000000000000000000, .isa = 16 },

    /* ISA v16 (control flow operations v3) */
    [kE64OpcodeSVC] = { .func = E64SVC, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 16 },

    /* ISA v17 (trap model and atomics) */
    [kE64OpcodeERET] = { .func = E64ERET, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 17 },
    [kE64OpcodeCSWP] = { .func = E64CSWP, .minargs = 2, .maxargs = 2, .argmask = 0b01000000000000000000000000000000, .isa = 17 },
    [kE64OpcodeXCHG] = { .func = E64XCHG, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 17 },
    [kE64OpcodeCAS] = { .func = E64CAS, .minargs = 3, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 17 },

    /* ISA v18 (floating point unit) */
    [kE64OpcodeFADD] = { .func = E64FADD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFSUB] = { .func = E64FSUB, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFMUL] = { .func = E64FMUL, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFDIV] = { .func = E64FDIV, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFMIN] = { .func = E64FMIN, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFMAX] = { .func = E64FMAX, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFSQRT] = { .func = E64FSQRT, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFMADD] = { .func = E64FMADD, .minargs = 4, .maxargs = 4, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFNEG] = { .func = E64FNEG, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFABS] = { .func = E64FABS, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCMP] = { .func = E64FCMP, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFADDS] = { .func = E64FADDS, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFSUBS] = { .func = E64FSUBS, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFMULS] = { .func = E64FMULS, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFDIVS] = { .func = E64FDIVS, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFMINS] = { .func = E64FMINS, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFMAXS] = { .func = E64FMAXS, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFSQRTS] = { .func = E64FSQRTS, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFMADDS] = { .func = E64FMADDS, .minargs = 4, .maxargs = 4, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFNEGS] = { .func = E64FNEGS, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFABSS] = { .func = E64FABSS, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCMPS] = { .func = E64FCMPS, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCVTDS] = { .func = E64FCVTDS, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCVTSD] = { .func = E64FCVTSD, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCVTDI] = { .func = E64FCVTDI, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCVTDU] = { .func = E64FCVTDU, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCVTID] = { .func = E64FCVTID, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCVTUD] = { .func = E64FCVTUD, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCVTSI] = { .func = E64FCVTSI, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCVTSU] = { .func = E64FCVTSU, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCVTIS] = { .func = E64FCVTIS, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },
    [kE64OpcodeFCVTUS] = { .func = E64FCVTUS, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 18 },

    /* ISA v19 (efficiency additions) */
    [kE64OpcodeSEXT] = { .func = E64SEXT, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodeCBLT] = { .func = E64CBLT, .minargs = 3, .maxargs = 3, .argmask = 0b00000000000000000000000000000000, .isa = 19 },
    [kE64OpcodeCLZ] = { .func = E64CLZ, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodeCTZ] = { .func = E64CTZ, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodePOPCNT] = { .func = E64POPCNT, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodeMULH] = { .func = E64MULH, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodeUMULH] = { .func = E64UMULH, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodeADC] = { .func = E64ADC, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodeSBB] = { .func = E64SBB, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 19 },

    /* ISA v19 (PAC extension) */
    [kE64OpcodePACIA] = { .func = E64PACIA, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodeAUTIA] = { .func = E64AUTIA, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodePACDA] = { .func = E64PACDA, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodeAUTDA] = { .func = E64AUTDA, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodeXPAC] = { .func = E64XPAC, .minargs = 1, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 19 },
    [kE64OpcodePACRET] = { .func = E64PACRET, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 19 },
    [kE64OpcodeRETA] = { .func = E64RETA, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 19 },

    /* ISA v20 (performance update) */
    [kE64OpcodeRDTIME] = { .func = E64RDTIME, .minargs = 1, .maxargs = 1, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeWFE] = { .func = E64WFE, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeSEV] = { .func = E64SEV, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 20 },
    [kE64OpcodePAUSE] = { .func = E64PAUSE, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeCBEQ] = { .func = E64CBEQ, .minargs = 3, .maxargs = 3, .argmask = 0b00000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeCBNE] = { .func = E64CBNE, .minargs = 3, .maxargs = 3, .argmask = 0b00000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeCBGE] = { .func = E64CBGE, .minargs = 3, .maxargs = 3, .argmask = 0b00000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeCBLTU] = { .func = E64CBLTU, .minargs = 3, .maxargs = 3, .argmask = 0b00000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeCBGEU] = { .func = E64CBGEU, .minargs = 3, .maxargs = 3, .argmask = 0b00000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeLDSB] = { .func = E64LDSB, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeLDSW] = { .func = E64LDSW, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeLDSD] = { .func = E64LDSD, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeLDADD] = { .func = E64LDADD, .minargs = 3, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeLDAND] = { .func = E64LDAND, .minargs = 3, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeLDOR] = { .func = E64LDOR, .minargs = 3, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeLDXOR] = { .func = E64LDXOR, .minargs = 3, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeSWPADD] = { .func = E64SWPADD, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeLDADDD] = { .func = E64LDADDD, .minargs = 3, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeLDANDD] = { .func = E64LDANDD, .minargs = 3, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeLDORD] = { .func = E64LDORD, .minargs = 3, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeLDXORD] = { .func = E64LDXORD, .minargs = 3, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeSWPADDD] = { .func = E64SWPADDD, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000, .isa = 20 },
    [kE64OpcodeTLBI] = { .func = E64TLBI, .minargs = 0, .maxargs = 2, .argmask = 0b00000000000000000000000000000000, .isa = 20 },

    /* ISA v21 (32bit arithmetic update) */
    [kE64OpcodeADDD] = { .func = E64ADDD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeIADDD] = { .func = E64IADDD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeSUBD] = { .func = E64SUBD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeISUBD] = { .func = E64ISUBD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeMULD] = { .func = E64MULD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeIMULD] = { .func = E64IMULD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeSHLD] = { .func = E64SHLD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeISHLD] = { .func = E64ISHLD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeSHRD] = { .func = E64SHRD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeSARD] = { .func = E64SARD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeDIVD] = { .func = E64DIVD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeIDIVD] = { .func = E64IDIVD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },
    [kE64OpcodeMODD] = { .func = E64MODD, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000, .isa = 21 },

    /* ISA v22 (calls that carry their arguments) */
    [kE64OpcodeBLA] = { .func = E64BLA, .minargs = 1, .maxargs = 1 + EMEX64_CALL_ARGS, .argmask = 0b00000000000000000000000000000000, .isa = 22 },

    /* ISA v23 (debugging update) */
    [kE64OpcodeBRK] = { .func = E64BRK, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000, .isa = 23 },
};

static const UInt8 kImmBits[] = {
    [kE64ParameterCodingImm4] = 4,
    [kE64ParameterCodingImm8] = 8,
    [kE64ParameterCodingImm16] = 16,
    [kE64ParameterCodingImm32] = 32,
    [kE64ParameterCodingImm64] = 64,
    [kE64ParameterCodingAddr64] = 64,
};

static void __E64CoreDeinit(EFObjectRef obj)
{
    __E64Core core = (__E64Core)obj;
    free(core->icache);
    free(core->icold);
    free(core->ktext.runs);
    core->icache = NULL;
    core->icold = NULL;
    core->ktext.runs = NULL;
    core->ktext.size = 0;
    pthread_mutex_destroy(&core->wait_lock);
    pthread_cond_destroy(&core->wait_cond);
}

static EFClassDefinitionV4 E64CoreClass = {
    .header = {
        .version = 4,
        .typeID = kEFTypeIDNone,
        .name = EFSTR_FILESCOPE("E64Core"),
    },
    .init = NULL,
    .deinit = __E64CoreDeinit,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = NULL,
};

static void E64CoreRegisterClass(void)
{
    EFClassRegister(&E64CoreClass);
}

EFTypeID E64CoreGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, E64CoreRegisterClass);
    return E64CoreClass.header.typeID;
}

E64CoreRef E64CoreCreateWithMachine(EFAllocatorRef allocatorRef,
                                    E64MachineRef machineRef)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL || machine->cores[0] != NULL)
    {
        return NULL;
    }
    return __E64CoreCreateWithID(allocatorRef, machineRef, 0);
}

E64CoreRef __E64CoreCreateWithID(EFAllocatorRef allocatorRef,
                                 E64MachineRef machineRef,
                                 UInt8 id)
{
    __E64Machine machine = (__E64Machine)machineRef;
    if(machine == NULL)
    {
        return NULL;
    }

    __E64Core core = (__E64Core)EFObjectCreate(allocatorRef, E64CoreGetTypeID(), (EFIndex)sizeof(struct __E64Core));
    if(core == NULL)
    {
        return NULL;
    }

    core->machine = machine;    /* borrowed! (otherwise this causes a retain loop) */

    /*
     * setting it up with secure page table monitor,
     * because otherwise the core would sit in usermode
     * at start and the firmware wouldn't be able
     * to write to control registers, ultimatively
     * rendering the entire state functionless.
     */
    core->cr_state.crel.level = kE64ElevationLevelSecurePageTableMonitor;

    /*
     * setting it to the latest ISA version the SoC
     * supports running.
     */
    core->cr_state.crisa.activeIsa = E64VM_ISA_MAX_VERSION;

    core->snoop = E64MemoryGetSnoop(machine->memory);
    core->ram = E64MemoryGetRAM(machine->memory);
    core->ram_size = E64MemoryGetSize(machine->memory);
    for(UInt64 k = 0; k < EMEX64_PAC_CACHE_ENTRIES; k++)
    {
        core->pac_cache[k].ptr = ~0ULL;
    }
    core->icache = aligned_alloc(_Alignof(emex64_icache_entry_t), sizeof(emex64_icache_entry_t) * EMEX64_ICACHE_ENTRIES);
    core->icold = malloc(sizeof(emex64_icache_cold_t) * EMEX64_ICACHE_ENTRIES);
    if(core->icache == NULL || core->icold == NULL)
    {
        free(core->icache);
        free(core->icold);
        core->icache = NULL;
        core->icold = NULL;
    }
    if(core->icache != NULL)
    {
        for(UInt64 k = 0; k < EMEX64_ICACHE_ENTRIES; k++)
        {
            core->icache[k].paddr = ~0ULL;
        }
    }

    /* nothing remembered about pages yet */
    memset(core->pages, 0, sizeof(core->pages));
    core->tlb_seq = 0;
    emex64_core_tlb_changed(core);

    /* core 0 starts at reset */
    core->id = id;
    emex64_lapic_init(&core->lapic);
    core->run_state = id == 0 ? E64_CORE_RUNNING : E64_CORE_WAITING;
    core->request = 0;
    pthread_mutex_init(&core->wait_lock, NULL);
    pthread_cond_init(&core->wait_cond, NULL);
    core->sleeping = false;
    core->event = false;
    core->wfe = false;

    return core;
}

static void __E64CoreWake(__E64Core core)
{
    pthread_mutex_lock(&core->wait_lock);
    pthread_cond_broadcast(&core->wait_cond);
    pthread_mutex_unlock(&core->wait_lock);
}

void __E64CoreKick(__E64Core core)
{
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    if(__atomic_load_n(&core->sleeping, __ATOMIC_RELAXED))
    {
        __E64CoreWake(core);
    }
}

void __E64CoreRequestReset(__E64Core core)
{
    __E64Machine machine = (__E64Machine)core->machine;
    __atomic_store_n(&machine->resetting, true, __ATOMIC_SEQ_CST);
    for(UInt8 i = 0; i < machine->coreCount; i++)
    {
        __E64Core core = (__E64Core)machine->cores[i];
        __atomic_store_n(&core->run_break, true, __ATOMIC_RELEASE);
        __E64CoreWake(core);
    }
}

void __E64CoreSendIPI(__E64Core sender,
                      __E64Core target,
                      UInt64 type)
{
    switch(type)
    {
        case APIC_IPI_FIXED:
            __atomic_fetch_or(&target->lapic.ipi_status, 1u, __ATOMIC_ACQ_REL);
            __atomic_fetch_or(&target->lapic.pending, APIC_PENDING_IPI, __ATOMIC_ACQ_REL);
            __E64CoreKick(target);
            break;
        case APIC_IPI_INIT:
            if(target != sender)
            {
                __atomic_store_n(&target->request, APIC_IPI_INIT + 1, __ATOMIC_RELEASE);
                __E64CoreWake(target);
            }
            break;
        case APIC_IPI_STARTUP:
            if(target != sender && __atomic_load_n(&target->run_state, __ATOMIC_ACQUIRE) == E64_CORE_WAITING)
            {
                pthread_mutex_lock(&target->wait_lock);
                target->startup.start = sender->lapic.start;
                target->startup.arg = sender->lapic.arg;
                target->startup.level = sender->cr_state.crel.level;
                target->startup.isa = sender->cr_state.crisa.activeIsa;
                __atomic_store_n(&target->request, APIC_IPI_STARTUP + 1, __ATOMIC_RELEASE);
                pthread_cond_broadcast(&target->wait_cond);
                pthread_mutex_unlock(&target->wait_lock);
            }
            break;
        default:
            break;
    }
}

static void __E64CoreReset(__E64Core core)
{
    memset(core->rl, 0, sizeof(core->rl));
    memset(core->erl, 0, sizeof(core->erl));
    memset(core->frl, 0, sizeof(core->frl));
    memset(&core->cr_state, 0, sizeof(core->cr_state));
    memset(&core->trap, 0, sizeof(core->trap));
    core->cr_state.crel.level = kE64ElevationLevelSecurePageTableMonitor;
    core->cr_state.crisa.activeIsa = E64VM_ISA_MAX_VERSION;
    core->halted = false;
    core->wfe = false;
    core->event = false;
    core->in_interrupt = false;
    core->irq_poll = true;
    core->irq_ie = false;
    core->tlb.asid = 0;
    for(UInt64 k = 0; k < EMEX64_TLB_ENTRIES; k++)
    {
        core->tlb.entries[k].valid = false;
    }
    for(UInt64 k = 0; k < EMEX64_TLB_ROOTS; k++)
    {
        core->tlb.roots[k].valid = false;
    }
    emex64_core_tlb_changed(core);
    memset(core->pages, 0, sizeof(core->pages));
    core->ktext.size = 0;
    core->ktext.wait = 0;
    core->ktext.flushed = false;
    core->lapic.timer.ctrl = 0;
    core->lapic.timer.min_elapsed = 0;
    __atomic_store_n(&core->lapic.pending, 0, __ATOMIC_RELEASE);
}

void __E64CoreResetForPowerOn(__E64Core core)
{
    __E64CoreReset(core);
    emex64_lapic_init(&core->lapic);
    for(UInt64 k = 0; k < EMEX64_PAC_CACHE_ENTRIES; k++)
    {
        core->pac_cache[k].ptr = ~0ULL;
    }
    if(core->icache != NULL)
    {
        for(UInt64 k = 0; k < EMEX64_ICACHE_ENTRIES; k++)
        {
            core->icache[k].paddr = ~0ULL;
        }
    }
    core->sleeping = false;
    __atomic_store_n(&core->request, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&core->run_state, core->id == 0 ? E64_CORE_RUNNING : E64_CORE_WAITING, __ATOMIC_RELEASE);
}

static Boolean __E64CoreHandleRequests(__E64Core core)
{
    __E64Machine machine = (__E64Machine)core->machine;
    for(;;)
    {
        UInt32 req = __atomic_exchange_n(&core->request, 0, __ATOMIC_ACQ_REL);
        if(req == APIC_IPI_INIT + 1)
        {
            __E64CoreReset(core);
            __atomic_store_n(&core->run_state, E64_CORE_WAITING, __ATOMIC_RELEASE);
        }
        else if(req == APIC_IPI_STARTUP + 1 && __atomic_load_n(&core->run_state, __ATOMIC_ACQUIRE) == E64_CORE_WAITING)
        {
            pthread_mutex_lock(&core->wait_lock);
            __E64CoreReset(core);
            core->cr_state.crel.level = core->startup.level;
            core->cr_state.crisa.activeIsa = core->startup.isa;
            core->rl[kE64RegisterPC] = core->startup.start;
            core->rl[kE64RegisterR0] = core->startup.arg;
            pthread_mutex_unlock(&core->wait_lock);
            __atomic_store_n(&core->run_state, E64_CORE_RUNNING, __ATOMIC_RELEASE);
        }

        if(__atomic_load_n(&core->run_state, __ATOMIC_ACQUIRE) == E64_CORE_RUNNING)
        {
            return true;
        }

        pthread_mutex_lock(&core->wait_lock);
        while(__atomic_load_n(&core->request, __ATOMIC_ACQUIRE) == 0 && !machine->poweredOff && !machine->resetting)
        {
            pthread_cond_wait(&core->wait_cond, &core->wait_lock);
        }
        pthread_mutex_unlock(&core->wait_lock);
        if(machine->poweredOff)
        {
            return false;
        }
        if(machine->resetting)
        {
            return true;
        }
    }
}

static inline EFIndex __E64CoreTakeInterrupt(__E64Core core)
{
    UInt32 p = __atomic_load_n(&core->lapic.pending, __ATOMIC_ACQUIRE);
    if(unlikely(p != 0))
    {
        UInt32 bit = (p & APIC_PENDING_IPI) ? APIC_PENDING_IPI : APIC_PENDING_TIMER;
        if(__atomic_fetch_and(&core->lapic.pending, ~bit, __ATOMIC_ACQ_REL) & bit)
        {
            return bit == APIC_PENDING_IPI ? EMEX64_IRQ_IPI : EMEX64_IRQ_LAPIC_TIMER;
        }
    }
    return E64ICTakeInterrupt(core->machine->interruptController, core->id);
}

static inline Boolean __E64CoreHasPendingInterrupt(__E64Core core)
{
    return __atomic_load_n(&core->lapic.pending, __ATOMIC_ACQUIRE) != 0 || E64ICHasPendingInterrupt(core->machine->interruptController, core->id);
}

static inline UInt64 __E64CoreTimerDue(const emex64_vtimer_t *t,
                                       UInt64 now)
{
    UInt64 elapsed = now - t->last_host_cycles;
    return elapsed < t->min_elapsed ? t->min_elapsed - elapsed : 0;
}

static void __E64CoreIdle(__E64Core core)
{
    __E64Machine machine = (__E64Machine)core->machine;
    UInt64 now = emex64_get_host_cycles();
    UInt64 freq = emex64_host_freq();
    UInt64 wait = freq / 100;
    UInt64 due = __E64CoreTimerDue(&core->lapic.timer, now);
    if(due < wait)
    {
        wait = due;
    }
    if(core->id == 0 && machine->timer != NULL)
    {
        due = __E64CoreTimerDue(&machine->timer->vt, now);
        if(due < wait)
        {
            wait = due;
        }
    }
    if(wait == 0)
    {
        return;
    }
    UInt64 ns = (UInt64)((__uint128_t)wait * 1000000000ULL / freq);

    pthread_mutex_lock(&core->wait_lock);
    __atomic_store_n(&core->sleeping, true, __ATOMIC_RELAXED);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    if(!__E64CoreHasPendingInterrupt(core) &&
       !(core->wfe && __atomic_load_n(&core->event, __ATOMIC_ACQUIRE)) &&
       __atomic_load_n(&core->request, __ATOMIC_ACQUIRE) == 0 &&
       !machine->poweredOff && !machine->resetting)
    {
#if defined(__APPLE__)
        struct timespec rel = { (time_t)(ns / 1000000000ULL), (long)(ns % 1000000000ULL) };
        pthread_cond_timedwait_relative_np(&core->wait_cond, &core->wait_lock, &rel);
#else
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += (time_t)(ns / 1000000000ULL);
        ts.tv_nsec += (long)(ns % 1000000000ULL);
        if(ts.tv_nsec >= 1000000000L)
        {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&core->wait_cond, &core->wait_lock, &ts);
#endif /* __APPLE__ */
    }
    __atomic_store_n(&core->sleeping, false, __ATOMIC_RELAXED);
    pthread_mutex_unlock(&core->wait_lock);
}

static inline void __E64CoreTickTimers(__E64Core core)
{
    UInt64 now = emex64_get_host_cycles();
    if(emex64_vtimer_tick(&core->lapic.timer, now))
    {
        __atomic_fetch_or(&core->lapic.pending, APIC_PENDING_TIMER, __ATOMIC_ACQ_REL);
    }
    if(core->id == 0)
    {
        emex64_timer_tick(core->machine->timer, now);
    }
}

void __E64CoreWritePageTableBase(__E64Core core,
                                 UInt64 value)
{
    UInt64 root = ((value & EMEX64_PTB_PFN_MASK) >> 8) << 13;
    UInt16 asid = (UInt16)(value >> EMEX64_ASID_SHIFT);
    emex64_tlb_t *tlb = &core->tlb;
    UInt64 r = asid & (EMEX64_TLB_ROOTS - 1);
    if(!(tlb->roots[r].valid && tlb->roots[r].asid == asid && tlb->roots[r].root == root))
    {
        for(UInt64 k = 0; k < EMEX64_TLB_ENTRIES; k++)
        {
            if(tlb->entries[k].asid == asid)
            {
                tlb->entries[k].valid = false;
            }
        }
        tlb->roots[r].valid = true;
        tlb->roots[r].asid = asid;
        tlb->roots[r].root = root;
    }
    tlb->asid = asid;
    core->cr_state.crptb.enabled = (value & EMEX64_MEMORY_MMU_MASK_FLAGS) & kE64MMUPTPresent;
    core->cr_state.crptb.pgd_addr = root;

    /* another address space */
    emex64_core_tlb_changed(core);

    if(core->ktext.size != 0)
    {
        UInt64 pte, top = 0;
        __E64MemoryCoreWatchPage(core->machine->memory, core, core->ktext.vbase, &pte, &top);
        if(top != core->ktext.root)
        {
            core->ktext.size = 0;
        }
    }
}

UInt64 __E64CoreReadPageTableBase(__E64Core core)
{
    UInt64 value = ((core->cr_state.crptb.pgd_addr >> 13) << 8) & EMEX64_PTB_PFN_MASK;
    value |= (UInt64)core->tlb.asid << EMEX64_ASID_SHIFT;
    value |= core->cr_state.crptb.enabled ? kE64MMUPTPresent : 0;
    return value;
}

static inline Boolean __E64CoreFetchAddress(__E64Core core,
                                            UInt64 pc,
                                            UInt64 *paddr)
{
    if(core->cr_state.crptb.enabled && !core->in_interrupt)
    {
        emex64_tlb_entry_t *e = emex64_tlb_lookup(&core->tlb, core->snoop, pc);
        if(likely(e != NULL))
        {
            UInt8 check = kE64MMUPTExec;
            if(core->cr_state.crel.level < kE64ElevationLevelKernel)
            {
                check |= kE64MMUPTUser;
            }
            if(likely((e->flags & check) == check))
            {
                *paddr = e->ppage + (pc & 0x1FFF);
                return true;
            }
        }
        return __E64MemoryCoreTranslate(core->machine->memory, core, pc, kE64MemoryActionTypeExecute, paddr);
    }

    /* this also keeps MMIO out */
    if(unlikely(pc >= core->ram_size))
    {
        return false;
    }
    *paddr = pc;

    return true;
}

/* a locked KTRR region is the only code kernel may run */
static inline Boolean __E64CoreKTRRAllowsFetch(__E64Core core,
                                               UInt64 paddr,
                                               UInt64 ilen)
{
    const emex64_snoop_t *snoop = core->snoop;
    if(likely(!snoop->ktrr_locked) || core->cr_state.crel.level < kE64ElevationLevelKernel)
    {
        return true;
    }
    return paddr >= snoop->ktrr_base && paddr + ilen <= snoop->ktrr_base + snoop->ktrr_size;
}

static inline UInt64 __E64CoreOperandValue(__E64Core core,
                                           UInt8 kind,
                                           UInt8 reg,
                                           UInt64 imm)
{
    switch(kind)
    {
        case kE64OperandReg:
            return core->rl[reg];
        case kE64OperandRegExtended:
            return core->erl[reg];
        case kE64OperandRegFloat:
            return core->frl[reg].u64;
        case kE64OperandRegInc:
            return core->rl[reg]++;
        case kE64OperandRegDec:
            return core->rl[reg]--;
        default:
            return imm;
    }
}

static inline __attribute__((always_inline)) void __E64CoreReplay(__E64Core core,
                                                                  const emex64_icache_entry_t *ce)
{
    const emex64_icache_cold_t *cc = &core->icold[ce - core->icache];
    core->op.ce = ce;
    core->op.opcode = cc->opcode;
    core->op.opce = cc->opce;
    core->op.ilen = ce->ilen;
    core->op.param_cnt = cc->param_cnt;
    core->op.param = (UInt64 **)cc->param;
    core->op.param_coding = (E64ParameterCoding *)cc->param_coding;

    for(UInt8 k = 0; k < cc->dyn_cnt; k++)
    {
        UInt8 i = cc->dyn[k];
        const emex64_operand_recipe_t *r = &cc->operands[i];
        if(likely(r->kind == kE64OperandOffsetRegImm))
        {
            core->op.immcache[i] = core->rl[r->base_reg] + r->off_imm;
        }
        else if(r->kind == kE64OperandOffset)
        {
            Boolean base_late = r->base_kind == kE64OperandReg || r->base_kind == kE64OperandRegExtended || r->base_kind == kE64OperandRegFloat;
            UInt64 base = base_late ? 0 : __E64CoreOperandValue(core, r->base_kind, r->base_reg, r->imm);
            UInt64 off = __E64CoreOperandValue(core, r->off_kind, r->off_reg, r->off_imm);
            if(base_late)
            {
                base = __E64CoreOperandValue(core, r->base_kind, r->base_reg, r->imm);
            }
            core->op.immcache[i] = r->add ? base + off : base - off;
        }
        else
        {
            core->op.immcache[i] = __E64CoreOperandValue(core, r->kind, r->reg, r->imm);
        }
    }
}

static void __E64CoreFetchFaultAt(__E64Core core,
                                  UInt64 missing)
{
    core->cr_state.crexc.exception = kE64ExceptionNone;
    __E64MemoryCoreCopyIn(core->machine->memory, core, core->op.inscache + EMEX64_MAX_ILEN - 1, missing, 1, kE64MemoryActionTypeExecute);
}

static const Boolean kE64RegistersOnly[kE64OpcodeMAX + 1] = {
    [kE64OpcodeNOP] = true,
    [kE64OpcodeMOV] = true,
    [kE64OpcodeMOVZ] = true,
    [kE64OpcodeSWP] = true,
    [kE64OpcodeADD] = true,
    [kE64OpcodeSUB] = true,
    [kE64OpcodeMUL] = true,
    [kE64OpcodeNOT] = true,
    [kE64OpcodeNEG] = true,
    [kE64OpcodeAND] = true,
    [kE64OpcodeOR] = true,
    [kE64OpcodeXOR] = true,
    [kE64OpcodeSHR] = true,
    [kE64OpcodeSHL] = true,
    [kE64OpcodeSAR] = true,
    [kE64OpcodeROR] = true,
    [kE64OpcodeROL] = true,
    [kE64OpcodePDEP] = true,
    [kE64OpcodePEXT] = true,
    [kE64OpcodeBSWAPW] = true,
    [kE64OpcodeBSWAPD] = true,
    [kE64OpcodeBSWAPQ] = true,
    [kE64OpcodeINC] = true,
    [kE64OpcodeDEC] = true,
    [kE64OpcodeCMP] = true,
    [kE64OpcodeCLR] = true,
    [kE64OpcodeRDRND] = true,
    [kE64OpcodeSEXT] = true,
    [kE64OpcodeCLZ] = true,
    [kE64OpcodeCTZ] = true,
    [kE64OpcodePOPCNT] = true,
    [kE64OpcodeMULH] = true,
    [kE64OpcodeUMULH] = true,
    [kE64OpcodeADC] = true,
    [kE64OpcodeSBB] = true,
    [kE64OpcodeXPAC] = true,
    [kE64OpcodeFADD] = true,
    [kE64OpcodeFSUB] = true,
    [kE64OpcodeFMUL] = true,
    [kE64OpcodeFDIV] = true,
    [kE64OpcodeFMIN] = true,
    [kE64OpcodeFMAX] = true,
    [kE64OpcodeFSQRT] = true,
    [kE64OpcodeFMADD] = true,
    [kE64OpcodeFNEG] = true,
    [kE64OpcodeFABS] = true,
    [kE64OpcodeFCMP] = true,
    [kE64OpcodeFADDS] = true,
    [kE64OpcodeFSUBS] = true,
    [kE64OpcodeFMULS] = true,
    [kE64OpcodeFDIVS] = true,
    [kE64OpcodeFMINS] = true,
    [kE64OpcodeFMAXS] = true,
    [kE64OpcodeFSQRTS] = true,
    [kE64OpcodeFMADDS] = true,
    [kE64OpcodeFNEGS] = true,
    [kE64OpcodeFABSS] = true,
    [kE64OpcodeFCMPS] = true,
    [kE64OpcodeFCVTDS] = true,
    [kE64OpcodeFCVTSD] = true,
    [kE64OpcodeFCVTDI] = true,
    [kE64OpcodeFCVTDU] = true,
    [kE64OpcodeFCVTID] = true,
    [kE64OpcodeFCVTUD] = true,
    [kE64OpcodeFCVTSI] = true,
    [kE64OpcodeFCVTSU] = true,
    [kE64OpcodeFCVTIS] = true,
    [kE64OpcodeFCVTUS] = true,
    [kE64OpcodeRDTIME] = true,
    [kE64OpcodeWFE] = true,
    [kE64OpcodeSEV] = true,
    [kE64OpcodePAUSE] = true,
    [kE64OpcodeCBLT] = true,
    [kE64OpcodeCBEQ] = true,
    [kE64OpcodeCBNE] = true,
    [kE64OpcodeCBGE] = true,
    [kE64OpcodeCBLTU] = true,
    [kE64OpcodeCBGEU] = true,
    [kE64OpcodeADDD] = true,
    [kE64OpcodeIADDD] = true,
    [kE64OpcodeSUBD] = true,
    [kE64OpcodeISUBD] = true,
    [kE64OpcodeMULD] = true,
    [kE64OpcodeIMULD] = true,
    [kE64OpcodeSHLD] = true,
    [kE64OpcodeISHLD] = true,
    [kE64OpcodeSHRD] = true,
    [kE64OpcodeSARD] = true,
    [kE64OpcodeB] = true,
    [kE64OpcodeBE] = true,
    [kE64OpcodeBNE] = true,
    [kE64OpcodeBLT] = true,
    [kE64OpcodeBGT] = true,
    [kE64OpcodeBLE] = true,
    [kE64OpcodeBGE] = true,
    [kE64OpcodeBZ] = true,
    [kE64OpcodeBNZ] = true,
    [kE64OpcodeBBZ] = true,
    [kE64OpcodeBBNZ] = true,
};

static const Boolean kE64FaultsClean[kE64OpcodeMAX + 1] = {
    [kE64OpcodeLDB] = true,
    [kE64OpcodeLDW] = true,
    [kE64OpcodeLDD] = true,
    [kE64OpcodeLDQ] = true,
    [kE64OpcodeLDSB] = true,
    [kE64OpcodeLDSW] = true,
    [kE64OpcodeLDSD] = true,
    [kE64OpcodeSTB] = true,
    [kE64OpcodeSTW] = true,
    [kE64OpcodeSTD] = true,
    [kE64OpcodeSTQ] = true,
};

static const UInt8 kE64ConstOperands[kE64OpcodeMAX + 1] = {
    [kE64OpcodeMOV] = 1,
    [kE64OpcodeLDB] = 1,
    [kE64OpcodeLDW] = 1,
    [kE64OpcodeLDD] = 1,
    [kE64OpcodeLDQ] = 1,
    [kE64OpcodeLDSB] = 1,
    [kE64OpcodeLDSW] = 1,
    [kE64OpcodeLDSD] = 1,
    [kE64OpcodeADD] = 1,
    [kE64OpcodeSUB] = 1,
    [kE64OpcodeMUL] = 1,
    [kE64OpcodeDIV] = 1,
    [kE64OpcodeIDIV] = 1,
    [kE64OpcodeMOD] = 1,
    [kE64OpcodeAND] = 1,
    [kE64OpcodeOR] = 1,
    [kE64OpcodeXOR] = 1,
    [kE64OpcodeSHR] = 1,
    [kE64OpcodeSHL] = 1,
    [kE64OpcodeSAR] = 1,
    [kE64OpcodeROR] = 1,
    [kE64OpcodeROL] = 1,
    [kE64OpcodeSEXT] = 1,
    [kE64OpcodeCLZ] = 1,
    [kE64OpcodeCTZ] = 1,
    [kE64OpcodePOPCNT] = 1,
    [kE64OpcodeMULH] = 1,
    [kE64OpcodeUMULH] = 1,
    [kE64OpcodeADC] = 1,
    [kE64OpcodeSBB] = 1,
    [kE64OpcodeADDD] = 1,
    [kE64OpcodeIADDD] = 1,
    [kE64OpcodeSUBD] = 1,
    [kE64OpcodeISUBD] = 1,
    [kE64OpcodeMULD] = 1,
    [kE64OpcodeIMULD] = 1,
    [kE64OpcodeSHLD] = 1,
    [kE64OpcodeISHLD] = 1,
    [kE64OpcodeSHRD] = 1,
    [kE64OpcodeSARD] = 1,
    [kE64OpcodeDIVD] = 1,
    [kE64OpcodeIDIVD] = 1,
    [kE64OpcodeMODD] = 1,
    [kE64OpcodeSTB] = 2,
    [kE64OpcodeSTW] = 2,
    [kE64OpcodeSTD] = 2,
    [kE64OpcodeSTQ] = 2,
    [kE64OpcodePUSH] = 2,
    [kE64OpcodeCMP] = 2,
    [kE64OpcodeB] = 2,
    [kE64OpcodeBE] = 2,
    [kE64OpcodeBNE] = 2,
    [kE64OpcodeBLT] = 2,
    [kE64OpcodeBGT] = 2,
    [kE64OpcodeBLE] = 2,
    [kE64OpcodeBGE] = 2,
    [kE64OpcodeBZ] = 2,
    [kE64OpcodeBNZ] = 2,
    [kE64OpcodeBBZ] = 2,
    [kE64OpcodeBBNZ] = 2,
    [kE64OpcodeBL] = 2,
    [kE64OpcodeBLA] = 2,
    [kE64OpcodeCBLT] = 2,
    [kE64OpcodeCBEQ] = 2,
    [kE64OpcodeCBNE] = 2,
    [kE64OpcodeCBGE] = 2,
    [kE64OpcodeCBLTU] = 2,
    [kE64OpcodeCBGEU] = 2,
};

static inline Boolean __E64CoreOperandChangesRegister(UInt8 kind)
{
    return kind == kE64OperandRegInc || kind == kE64OperandRegDec;
}

#define E64_REG(r)  (1u << kE64Register##r)
static const UInt16 kE64ImplicitWrites[kE64OpcodeMAX + 1] = {
    [kE64OpcodePUSH] = E64_REG(PC) | E64_REG(SP),
    [kE64OpcodePOP] = E64_REG(PC) | E64_REG(SP),
    [kE64OpcodeLDB] = E64_REG(PC),
    [kE64OpcodeLDW] = E64_REG(PC),
    [kE64OpcodeLDD] = E64_REG(PC),
    [kE64OpcodeLDQ] = E64_REG(PC),
    [kE64OpcodeLDSB] = E64_REG(PC),
    [kE64OpcodeLDSW] = E64_REG(PC),
    [kE64OpcodeLDSD] = E64_REG(PC),
    [kE64OpcodeSTB] = E64_REG(PC),
    [kE64OpcodeSTW] = E64_REG(PC),
    [kE64OpcodeSTD] = E64_REG(PC),
    [kE64OpcodeSTQ] = E64_REG(PC),
    [kE64OpcodeMOVM] = E64_REG(PC),
    [kE64OpcodeCPYM] = E64_REG(PC),
    [kE64OpcodeSETM] = E64_REG(PC),
    [kE64OpcodeBL] = E64_REG(PC) | E64_REG(SP) | E64_REG(FP),
    [kE64OpcodeBLA] = E64_REG(PC) | E64_REG(SP) | E64_REG(FP),
    [kE64OpcodeRET] = E64_REG(PC) | E64_REG(SP) | E64_REG(FP),
    [kE64OpcodeRETA] = E64_REG(PC) | E64_REG(SP) | E64_REG(FP),
    [kE64OpcodePACRET] = E64_REG(PC),
    [kE64OpcodeDIV] = E64_REG(PC),
    [kE64OpcodeIDIV] = E64_REG(PC),
    [kE64OpcodeMOD] = E64_REG(PC),
    [kE64OpcodeDIVD] = E64_REG(PC),
    [kE64OpcodeIDIVD] = E64_REG(PC),
    [kE64OpcodeMODD] = E64_REG(PC),
    [kE64OpcodeXCHG] = E64_REG(PC),
    [kE64OpcodeCAS] = E64_REG(PC) | E64_REG(CF),
    [kE64OpcodeLDADD] = E64_REG(PC),
    [kE64OpcodeLDAND] = E64_REG(PC),
    [kE64OpcodeLDOR] = E64_REG(PC),
    [kE64OpcodeLDXOR] = E64_REG(PC),
    [kE64OpcodeSWPADD] = E64_REG(PC),
    [kE64OpcodeLDADDD] = E64_REG(PC),
    [kE64OpcodeLDANDD] = E64_REG(PC),
    [kE64OpcodeLDORD] = E64_REG(PC),
    [kE64OpcodeLDXORD] = E64_REG(PC),
    [kE64OpcodeSWPADDD] = E64_REG(PC),
};
#undef E64_REG

static inline UInt16 __E64CoreRegBit(UInt8 kind,
                                     UInt8 reg)
{
    return (kind == kE64OperandReg || kind == kE64OperandRegInc || kind == kE64OperandRegDec) ? (UInt16)(1u << reg) : 0;
}

static inline Boolean __E64CoreIsRelative(const emex64_icache_cold_t *ce,
                                          const emex64_operand_recipe_t *rec,
                                          UInt8 i)
{
    return rec[i].kind == kE64OperandImm && ce->param_coding[i] >= kE64ParameterCodingImm4 && ce->param_coding[i] <= kE64ParameterCodingImm32;
}

static void __E64CoreSpecialize(__E64Core core,
                                emex64_icache_entry_t *ce,
                                const emex64_operand_recipe_t *rec,
                                UInt8 count)
{
    emex64_icache_cold_t *cc = &core->icold[ce - core->icache];
    ce->fast = kE64FastNone;
    ce->rel = 0;
    ce->cf_mask = 0;
    ce->target = NULL;

    UInt64 page_offset = ce->paddr & EMEX64_PAGE_MASK;
    ce->seq = page_offset + ce->ilen < EMEX64_PAGE_SIZE ? &core->icache[emex64_icache_index(ce->paddr + ce->ilen)] : NULL;

    switch(cc->opcode)
    {
        case kE64OpcodeBL:
        case kE64OpcodeBLA:
        {
            if(count < 1 || count > 1 + EMEX64_CALL_ARGS)
            {
                return;
            }

            for(UInt8 i = 0; i < count; i++)
            {
                if(__E64CoreOperandChangesRegister(rec[i].kind) ||
                   (rec[i].kind == kE64OperandOffset && (__E64CoreOperandChangesRegister(rec[i].base_kind) || __E64CoreOperandChangesRegister(rec[i].off_kind))))
                {
                    return;
                }
            }

            if(!__E64CoreIsRelative(cc, rec, 0))
            {
                if(cc->opcode == kE64OpcodeBL && cc->dyn_cnt == 0)
                {
                    ce->q[0] = cc->param[0];
                    ce->fast = kE64FastCallIndirect;
                }
                return;
            }

            ce->rel = (SInt32)(SInt64)emex64_branch_pc(0, rec[0].imm, cc->param_coding[0]);
            if((SInt64)page_offset + ce->rel >= 0 && (SInt64)page_offset + ce->rel < (SInt64)EMEX64_PAGE_SIZE)
            {
                ce->target = &core->icache[emex64_icache_index(ce->paddr + (UInt64)(SInt64)ce->rel)];
            }
            ce->fast = cc->opcode == kE64OpcodeBL ? kE64FastCall : kE64FastCallArgs;
            return;
        }
        case kE64OpcodeRET:
            ce->fast = kE64FastReturn;
            return;
        case kE64OpcodeRETA:
            ce->fast = kE64FastReturnAuth;
            return;
        case kE64OpcodePACRET:
            ce->fast = kE64FastSignReturn;
            return;
        default:
            break;
    }

    if(ce->rollback)
    {
        return;
    }

    UInt8 fast = kE64FastNone;
    switch(cc->opcode)
    {
        /* fast math */
        case kE64OpcodeADD:
            fast = kE64FastAdd;
            goto arithmetic;
        case kE64OpcodeSUB:
            fast = kE64FastSub;
            goto arithmetic;
        case kE64OpcodeMUL:
            fast = kE64FastMul;
            goto arithmetic;
        case kE64OpcodeAND:
            fast = kE64FastAnd;
            goto arithmetic;
        case kE64OpcodeOR:
            fast = kE64FastOr;
            goto arithmetic;
        case kE64OpcodeXOR:
            fast = kE64FastXor;
            goto arithmetic;
        case kE64OpcodeSHL:
            fast = kE64FastShl;
            goto arithmetic;
        case kE64OpcodeSHR:
            fast = kE64FastShr;
            goto arithmetic;
        arithmetic:
            if(cc->dyn_cnt != 0 || (count != 2 && count != 3))
            {
                return;
            }
            ce->q[0] = cc->param[0];
            ce->q[1] = cc->param[count - 2];
            ce->q[2] = cc->param[count - 1];
            goto writes_first;
        case kE64OpcodeINC:
        case kE64OpcodeDEC:
        case kE64OpcodeNEG:
        case kE64OpcodeNOT:
        case kE64OpcodeCLR:
        {
            if(cc->dyn_cnt != 0 || count != 1)
            {
                return;
            }
            UInt64 *constant = &cc->operands[1].imm;
            ce->q[0] = cc->param[0];
            ce->q[1] = cc->param[0];
            ce->q[2] = constant;
            switch(cc->opcode)
            {
                case kE64OpcodeINC:
                    *constant = 1;
                    fast = kE64FastAdd;
                    break;
                case kE64OpcodeDEC:
                    *constant = 1;
                    fast = kE64FastSub;
                    break;
                case kE64OpcodeNOT:
                    *constant = ~(UInt64)0;
                    fast = kE64FastXor;
                    break;
                case kE64OpcodeNEG:
                    *constant = 0;
                    ce->q[1] = constant;
                    ce->q[2] = cc->param[0];
                    fast = kE64FastSub;
                    break;
                default:
                    *constant = 0;
                    ce->q[1] = constant;
                    fast = kE64FastMov;
                    break;
            }
            goto writes_first;
        }
        case kE64OpcodeMOV:
        case kE64OpcodeCMP:
            if(cc->dyn_cnt != 0 || count != 2)
            {
                return;
            }
            fast = cc->opcode == kE64OpcodeMOV ? kE64FastMov : kE64FastCmp;
            ce->q[0] = cc->param[0];
            ce->q[1] = cc->param[1];
            if(fast == kE64FastMov)
            {
                goto writes_first;
            }
            break;
        
        /* fast branch */
        case kE64OpcodeB:
            fast = kE64FastBranch;
            goto branch;
        case kE64OpcodeBE:
            fast = kE64FastBranchFlags;
            ce->cf_mask = kE64CompareFlagZ;
            goto branch;
        case kE64OpcodeBNE:
            fast = kE64FastBranchFlagsNot;
            ce->cf_mask = kE64CompareFlagZ;
            goto branch;
        case kE64OpcodeBLT:
            fast = kE64FastBranchFlags;
            ce->cf_mask = kE64CompareFlagL;
            goto branch;
        case kE64OpcodeBGT:
            fast = kE64FastBranchFlags;
            ce->cf_mask = kE64CompareFlagG;
            goto branch;
        case kE64OpcodeBLE:
            fast = kE64FastBranchFlags;
            ce->cf_mask = kE64CompareFlagL | kE64CompareFlagZ;
            goto branch;
        case kE64OpcodeBGE:
            fast = kE64FastBranchFlags;
            ce->cf_mask = kE64CompareFlagG | kE64CompareFlagZ;
            goto branch;
        case kE64OpcodeBZ:
            fast = kE64FastBranchZero;
            goto branch;
        case kE64OpcodeBNZ:
            fast = kE64FastBranchNotZero;
            goto branch;
        case kE64OpcodeCBEQ:
            fast = kE64FastBranchEqual;
            goto branch;
        case kE64OpcodeCBNE:
            fast = kE64FastBranchNotEqual;
            goto branch;
        case kE64OpcodeCBLT:
            fast = kE64FastBranchLess;
            goto branch;
        case kE64OpcodeCBGE:
            fast = kE64FastBranchGreaterEqual;
            goto branch;
        case kE64OpcodeCBLTU:
            fast = kE64FastBranchLessUnsigned;
            goto branch;
        case kE64OpcodeCBGEU:
            fast = kE64FastBranchGreaterEqualUnsigned;
            goto branch;
        branch:
        {
            UInt8 target = fast <= kE64FastBranchFlagsNot ? 0 : fast <= kE64FastBranchNotZero ? 1 : 2;
            if(cc->dyn_cnt != 0 || count != target + 1 || !__E64CoreIsRelative(cc, rec, target))
            {
                return;
            }
            ce->rel = (SInt32)(SInt64)emex64_branch_pc(0, rec[target].imm, cc->param_coding[target]);
            ce->q[0] = cc->param[0];
            ce->q[1] = cc->param[1];
            if((SInt64)page_offset + ce->rel >= 0 && (SInt64)page_offset + ce->rel < (SInt64)EMEX64_PAGE_SIZE)
            {
                ce->target = &core->icache[emex64_icache_index(ce->paddr + (UInt64)(SInt64)ce->rel)];
            }
            break;
        }

        /* fast memory */
        case kE64OpcodeLDB:
            fast = kE64FastLoad8;
            goto memory;
        case kE64OpcodeLDW:
            fast = kE64FastLoad16;
            goto memory;
        case kE64OpcodeLDD:
            fast = kE64FastLoad32;
            goto memory;
        case kE64OpcodeLDQ:
            fast = kE64FastLoad64;
            goto memory;
        case kE64OpcodeLDSB:
            fast = kE64FastLoadSigned8;
            goto memory;
        case kE64OpcodeLDSW:
            fast = kE64FastLoadSigned16;
            goto memory;
        case kE64OpcodeLDSD:
            fast = kE64FastLoadSigned32;
            goto memory;
        case kE64OpcodeSTB:
            fast = kE64FastStore8;
            goto memory;
        case kE64OpcodeSTW:
            fast = kE64FastStore16;
            goto memory;
        case kE64OpcodeSTD:
            fast = kE64FastStore32;
            goto memory;
        case kE64OpcodeSTQ:
            fast = kE64FastStore64;
            goto memory;
        memory:
        {
            UInt8 at = fast >= kE64FastStore8 ? 0 : 1;
            UInt8 value = 1 - at;
            if(count != 2)
            {
                return;
            }

            /* the only operand that may need computing is the address, and only as register + constant */
            for(UInt8 k = 0; k < cc->dyn_cnt; k++)
            {
                if(cc->dyn[k] != at || cc->operands[at].kind != kE64OperandOffsetRegImm)
                {
                    return;
                }
            }
            ce->q[0] = cc->param[value];
            if(cc->operands[at].kind == kE64OperandOffsetRegImm)
            {
                if((SInt64)cc->operands[at].off_imm != (SInt32)cc->operands[at].off_imm)
                {
                    return;
                }
                ce->q[1] = &core->rl[cc->operands[at].base_reg];
                ce->rel = (SInt32)cc->operands[at].off_imm;
            }
            else
            {
                ce->q[1] = cc->param[at];
            }
            if(fast < kE64FastStore8)
            {
                goto writes_first;
            }
            break;
        }

        writes_first:
            if(ce->q[0] == &core->rl[kE64RegisterPC])
            {
                return;
            }
            break;

        default:
            return;
    }

    ce->fast = fast;
}

static void __E64CoreFillCacheEntry(__E64Core core,
                                    emex64_icache_entry_t *ce,
                                    const emex64_operand_recipe_t *rec,
                                    UInt8 count)
{
    emex64_icache_cold_t *cc = &core->icold[ce - core->icache];
    UInt16 implicit = kE64ImplicitWrites[cc->opcode];
    cc->save_rl = implicit != 0 ? implicit : 0xFFFF;
    cc->save_erl = implicit == 0;
    cc->dyn_cnt = 0;
    UInt8 const_from = kE64ConstOperands[cc->opcode] == 2 ? 0 : kE64ConstOperands[cc->opcode] == 1 ? 1 : 0xFF;

    for(UInt8 i = 0; i < count; i++)
    {
        const emex64_operand_recipe_t *r = &rec[i];
        switch(r->kind)
        {
            case kE64OperandImm:
                if(i >= const_from)
                {
                    cc->param[i] = &cc->operands[i].imm;
                    break;
                }
                cc->param[i] = &core->op.immcache[i];
                cc->dyn[cc->dyn_cnt++] = i;
                break;
            case kE64OperandOffset:
                if(r->base_kind == kE64OperandReg && r->off_kind == kE64OperandImm)
                {
                    emex64_operand_recipe_t *o = &cc->operands[i];
                    o->kind = kE64OperandOffsetRegImm;
                    o->off_imm = r->add ? r->off_imm : (UInt64)0 - r->off_imm;
                }
                cc->param[i] = &core->op.immcache[i];
                cc->dyn[cc->dyn_cnt++] = i;
                break;
            case kE64OperandReg:
                cc->param[i] = &core->rl[r->reg];
                break;
            case kE64OperandRegExtended:
                cc->param[i] = &core->erl[r->reg];
                cc->save_erl = true;
                break;
            case kE64OperandRegFloat:
                cc->param[i] = &core->frl[r->reg].u64;
                break;
            default:
                cc->param[i] = &core->op.immcache[i];
                cc->dyn[cc->dyn_cnt++] = i;
                break;
        }

        cc->save_rl |= __E64CoreRegBit(r->kind, r->reg);
        if(r->kind == kE64OperandOffset)
        {
            cc->save_rl |= __E64CoreRegBit(r->base_kind, r->base_reg) | __E64CoreRegBit(r->off_kind, r->off_reg);
        }
    }
    for(UInt8 i = count; i < EMEX64_ICACHE_ARGS; i++)
    {
        cc->param[i] = &core->op.immcache[i];
    }

    __E64CoreSpecialize(core, ce, rec, count);
}

static inline UInt16 __E64CoreTextRegister(__E64Core core,
                                           const UInt64 *p)
{
    const UInt8 *b = (const UInt8 *)p;
    Boolean reg = (b >= (const UInt8 *)core->rl && b < (const UInt8 *)(core->rl + (kE64RegisterMAX + 1))) || (b >= (const UInt8 *)core->erl && b < (const UInt8 *)(core->erl + (kE64RegisterExtendedMAX + 1))) || (b >= (const UInt8 *)core->frl && b < (const UInt8 *)(core->frl + (kE64FloatingRegisterMAX + 1)));
    UInt64 off = (UInt64)(b - (const UInt8 *)core);
    return reg && off < 65536 ? (UInt16)off : 0;
}

static inline Boolean __E64CoreTextConstant(const emex64_icache_cold_t *cc,
                                            const UInt64 *p)
{
    return (const UInt8 *)p >= (const UInt8 *)cc->operands && (const UInt8 *)p < (const UInt8 *)(cc->operands + EMEX64_ICACHE_ARGS);
}

static int __E64CoreTextSources(__E64Core core,
                                const emex64_icache_cold_t *cc,
                                emex64_ktext_entry_t *k,
                                const UInt64 *a,
                                const UInt64 *b)
{
    UInt16 ra = __E64CoreTextRegister(core, a);
    UInt16 rb = __E64CoreTextRegister(core, b);
    if(ra != 0 && rb != 0)
    {
        k->r[1] = ra;
        k->r[2] = rb;
        return 0;
    }
    if(ra != 0 && __E64CoreTextConstant(cc, b))
    {
        k->r[1] = ra;
        k->imm = *b;
        return 1;
    }
    if(rb != 0 && __E64CoreTextConstant(cc, a))
    {
        k->r[2] = rb;
        k->imm = *a;
        return 2;
    }
    return -1;
}

static Boolean __E64CoreTextConvert(__E64Core core,
                                    const emex64_icache_entry_t *ce,
                                    const emex64_icache_cold_t *cc,
                                    emex64_ktext_entry_t *k,
                                    emex64_ktext_arg_t *args)
{
    static const UInt8 two[] = {
        [kE64FastAdd] = kE64TextAddRR, [kE64FastSub] = kE64TextSubRR, [kE64FastMul] = kE64TextMulRR,
        [kE64FastAnd] = kE64TextAndRR, [kE64FastOr] = kE64TextOrRR, [kE64FastXor] = kE64TextXorRR,
        [kE64FastShl] = kE64TextShlRR, [kE64FastShr] = kE64TextShrRR, [kE64FastCmp] = kE64TextCmpRR,
        [kE64FastBranchEqual] = kE64TextBranchEqualRR, [kE64FastBranchNotEqual] = kE64TextBranchNotEqualRR,
        [kE64FastBranchLess] = kE64TextBranchLessRR, [kE64FastBranchGreaterEqual] = kE64TextBranchGreaterEqualRR,
        [kE64FastBranchLessUnsigned] = kE64TextBranchLessUnsignedRR,
        [kE64FastBranchGreaterEqualUnsigned] = kE64TextBranchGreaterEqualUnsignedRR,
    };
    static const UInt8 one[] = {
        [kE64FastBranch] = kE64TextBranch, [kE64FastBranchFlags] = kE64TextBranchFlags,
        [kE64FastBranchFlagsNot] = kE64TextBranchFlagsNot,
        [kE64FastBranchZero] = kE64TextBranchZero, [kE64FastBranchNotZero] = kE64TextBranchNotZero,
        [kE64FastLoad8] = kE64TextLoad8, [kE64FastLoad16] = kE64TextLoad16, [kE64FastLoad32] = kE64TextLoad32,
        [kE64FastLoad64] = kE64TextLoad64, [kE64FastLoadSigned8] = kE64TextLoadSigned8,
        [kE64FastLoadSigned16] = kE64TextLoadSigned16, [kE64FastLoadSigned32] = kE64TextLoadSigned32,
        [kE64FastStore8] = kE64TextStore8R, [kE64FastStore16] = kE64TextStore16R,
        [kE64FastStore32] = kE64TextStore32R, [kE64FastStore64] = kE64TextStore64R,
        [kE64FastCall] = kE64TextCall, [kE64FastCallArgs] = kE64TextCallArgs,
        [kE64FastCallIndirect] = kE64TextCallIndirect, [kE64FastReturn] = kE64TextReturn,
        [kE64FastReturnAuth] = kE64TextReturnAuth, [kE64FastSignReturn] = kE64TextSignReturn,
    };
    int v;

    memset(k, 0, sizeof(*k));
    k->ilen = ce->ilen;
    k->rel = ce->rel;
    k->cf_mask = ce->cf_mask;

    switch(ce->fast)
    {
        case kE64FastMov:
            k->r[0] = __E64CoreTextRegister(core, ce->q[0]);
            k->r[1] = __E64CoreTextRegister(core, ce->q[1]);
            if(k->r[0] == 0)
            {
                return false;
            }
            if(k->r[1] != 0)
            {
                k->fast = kE64TextMovRR;
                return true;
            }
            if(!__E64CoreTextConstant(cc, ce->q[1]))
            {
                return false;
            }
            k->imm = *ce->q[1];
            k->fast = kE64TextMovRI;
            return true;
        case kE64FastAdd:
        case kE64FastSub:
        case kE64FastMul:
        case kE64FastAnd:
        case kE64FastOr:
        case kE64FastXor:
        case kE64FastShl:
        case kE64FastShr:
            k->r[0] = __E64CoreTextRegister(core, ce->q[0]);
            v = __E64CoreTextSources(core, cc, k, ce->q[1], ce->q[2]);
            if(k->r[0] == 0 || v < 0)
            {
                return false;
            }
            k->fast = (UInt8)(two[ce->fast] + v);
            return true;
        case kE64FastCmp:
        case kE64FastBranchEqual:
        case kE64FastBranchNotEqual:
        case kE64FastBranchLess:
        case kE64FastBranchGreaterEqual:
        case kE64FastBranchLessUnsigned:
        case kE64FastBranchGreaterEqualUnsigned:
            v = __E64CoreTextSources(core, cc, k, ce->q[0], ce->q[1]);
            if(v < 0)
            {
                return false;
            }
            k->fast = (UInt8)(two[ce->fast] + v);
            return true;
        case kE64FastBranchZero:
        case kE64FastBranchNotZero:
        case kE64FastCallIndirect:
            k->r[1] = __E64CoreTextRegister(core, ce->q[0]);
            if(k->r[1] == 0)
            {
                return false;
            }
            k->fast = one[ce->fast];
            return true;
        case kE64FastLoad8:
        case kE64FastLoad16:
        case kE64FastLoad32:
        case kE64FastLoad64:
        case kE64FastLoadSigned8:
        case kE64FastLoadSigned16:
        case kE64FastLoadSigned32:
            k->r[0] = __E64CoreTextRegister(core, ce->q[0]);
            k->r[1] = __E64CoreTextRegister(core, ce->q[1]);
            if(k->r[0] == 0 || k->r[1] == 0)
            {
                return false;
            }
            k->fast = one[ce->fast];
            return true;
        case kE64FastStore8:
        case kE64FastStore16:
        case kE64FastStore32:
        case kE64FastStore64:
            k->r[0] = __E64CoreTextRegister(core, ce->q[0]);
            k->r[1] = __E64CoreTextRegister(core, ce->q[1]);
            if(k->r[1] == 0)
            {
                return false;
            }
            k->fast = one[ce->fast];
            if(k->r[0] == 0)
            {
                if(!__E64CoreTextConstant(cc, ce->q[0]))
                {
                    return false;
                }
                k->imm = *ce->q[0];
                k->fast++;
            }
            return true;
        case kE64FastCallArgs:
            if(cc->param_cnt < 1 || cc->param_cnt > 1 + EMEX64_CALL_ARGS)
            {
                return false;
            }
            k->argc = cc->param_cnt - 1;
            for(UInt8 i = 0; i < k->argc; i++)
            {
                const emex64_operand_recipe_t *r = &cc->operands[i + 1];
                emex64_ktext_arg_t *a = &args[i];
                a->imm = 0;
                switch(r->kind)
                {
                    case kE64OperandReg:
                        a->kind = kE64TextArgReg;
                        a->reg = __E64CoreTextRegister(core, &core->rl[r->reg]);
                        break;
                    case kE64OperandRegExtended:
                        a->kind = kE64TextArgReg;
                        a->reg = __E64CoreTextRegister(core, &core->erl[r->reg]);
                        break;
                    case kE64OperandRegFloat:
                        a->kind = kE64TextArgReg;
                        a->reg = __E64CoreTextRegister(core, &core->frl[r->reg].u64);
                        break;
                    case kE64OperandImm:
                        a->kind = kE64TextArgImm;
                        a->reg = 0;
                        a->imm = r->imm;
                        break;
                    case kE64OperandOffsetRegImm:
                        a->kind = kE64TextArgRegImm;
                        a->reg = __E64CoreTextRegister(core, &core->rl[r->base_reg]);
                        a->imm = r->off_imm;
                        break;
                    default:
                        return false;
                }
                if(a->kind != kE64TextArgImm && a->reg == 0)
                {
                    return false;
                }
            }
            k->fast = kE64TextCallArgs;
            return true;
        case kE64FastBranch:
        case kE64FastBranchFlags:
        case kE64FastBranchFlagsNot:
        case kE64FastCall:
        case kE64FastReturn:
        case kE64FastReturnAuth:
        case kE64FastSignReturn:
            k->fast = one[ce->fast];
            return true;

        default:
            return false;
    }
}

static void __E64CoreTextPublish(__E64Core core,
                                 emex64_icache_entry_t *ce)
{
    emex64_ktext_store_t *store = &core->snoop->text;
    UInt64 off = ce->paddr - store->base;
    if(off >= (__atomic_load_n(&store->pages, __ATOMIC_ACQUIRE) << 13) || ce->fast == kE64FastNone)
    {
        return;
    }

    emex64_ktext_entry_t k;
    emex64_ktext_arg_t args[EMEX64_CALL_ARGS];
    if(!__E64CoreTextConvert(core, ce, &core->icold[ce - core->icache], &k, args))
    {
        return;
    }

    /* a call's arguments lie in the entries behind it */
    UInt32 slots = 1 + (UInt32)((k.argc * sizeof(emex64_ktext_arg_t) + sizeof(emex64_ktext_entry_t) - 1) / sizeof(emex64_ktext_entry_t));

    pthread_mutex_lock(&store->lock);
    UInt32 *page = store->index[off >> 13];
    if(page == NULL)
    {
        page = calloc(EMEX64_PAGE_SIZE, sizeof(UInt32));
        if(page != NULL)
        {
            __atomic_store_n(&store->index[off >> 13], page, __ATOMIC_RELEASE);
        }
    }
    if(page != NULL && page[off & EMEX64_PAGE_MASK] == 0)
    {
        UInt32 n = store->count + 1;
        if(n % EMEX64_KTEXT_SLAB + slots > EMEX64_KTEXT_SLAB)
        {
            n = (n / EMEX64_KTEXT_SLAB + 1) * EMEX64_KTEXT_SLAB;
        }
        UInt32 s = n / EMEX64_KTEXT_SLAB;
        if(s < EMEX64_KTEXT_SLABS)
        {
            if(store->slab[s] == NULL)
            {
                store->slab[s] = calloc(EMEX64_KTEXT_SLAB, sizeof(emex64_ktext_entry_t));
            }
            if(store->slab[s] != NULL)
            {
                emex64_ktext_entry_t *e = &store->slab[s][n % EMEX64_KTEXT_SLAB];
                if(k.argc != 0)
                {
                    memcpy(e + 1, args, k.argc * sizeof(emex64_ktext_arg_t));
                    k.args = (const emex64_ktext_arg_t *)(e + 1);
                }
                *e = k;
                store->count = n + slots - 1;
                __atomic_store_n(&page[off & EMEX64_PAGE_MASK], n, __ATOMIC_RELEASE);
            }
        }
    }
    Boolean there = page != NULL && page[off & EMEX64_PAGE_MASK] != 0;
    pthread_mutex_unlock(&store->lock);

    if(there)
    {
        ce->seq = NULL;
        ce->target = NULL;
    }
}

static Boolean __E64CoreNeedsRollback(E64Opcode opcode,
                                      const emex64_operand_recipe_t *rec,
                                      UInt8 count)
{
    if(opcode > kE64OpcodeMAX || !(kE64RegistersOnly[opcode] || kE64FaultsClean[opcode]))
    {
        return true;
    }
    for(UInt8 j = 0; j < count; j++)
    {
        if(__E64CoreOperandChangesRegister(rec[j].kind) ||
           (rec[j].kind == kE64OperandOffset && (__E64CoreOperandChangesRegister(rec[j].base_kind) || __E64CoreOperandChangesRegister(rec[j].off_kind))))
        {
            return true;
        }
    }
    return false;
}

static __attribute__((noinline)) void __E64CoreDecodeAndExecute(__E64Core core,
                                                                UInt64 pc,
                                                                Boolean have_pa,
                                                                UInt64 fetch_pa)
{
    emex64_operand_recipe_t rec[EMEX64_MAX_ARGS + 4];

    UInt64 avail = EMEX64_MAX_ILEN;
    UInt64 first = EMEX64_PAGE_SIZE - (pc & EMEX64_PAGE_MASK);

    if(unlikely(E64MemoryIsKTRRLocked(core->machine->memory)) && !(core->cr_state.crptb.enabled && !core->in_interrupt) &&
       core->cr_state.crel.level >= kE64ElevationLevelKernel)
    {
        UInt64 kbase = E64MemoryGetKTRRBase(core->machine->memory);
        UInt64 kend = kbase + E64MemoryGetKTRRSize(core->machine->memory);
        if(pc >= kbase && pc < kend && kend - pc < first)
        {
            first = kend - pc;
        }
    }

    if(likely(first >= EMEX64_MAX_ILEN))
    {
        if(unlikely(!__E64MemoryCoreCopyIn(core->machine->memory, core, core->op.inscache, pc, EMEX64_MAX_ILEN, kE64MemoryActionTypeExecute)))
        {
            /* callee wrote exception already */
            return;
        }
    }
    else
    {
        if(unlikely(!__E64MemoryCoreCopyIn(core->machine->memory, core, core->op.inscache, pc, first, kE64MemoryActionTypeExecute)))
        {
            return;
        }
        E64Exception exc = core->cr_state.crexc.exception;
        Boolean fault_valid = core->trap.fault_valid;
        if(!__E64MemoryCoreCopyIn(core->machine->memory, core, core->op.inscache + first, pc + first, EMEX64_MAX_ILEN - first, kE64MemoryActionTypeExecute))
        {
            core->cr_state.crexc.exception = exc;
            core->trap.fault_valid = fault_valid;
            memset(core->op.inscache + first, 0, EMEX64_MAX_ILEN - first);
            avail = first;
        }
    }

    bitbolt_t bb = { core->op.inscache, 0, 0, 0 };

#define EMEX64_BAD_INSTRUCTION()                                            \
    do                                                                      \
    {                                                                       \
        if(unlikely(((bb.pos + 7u) >> 3) > avail))                          \
        {                                                                   \
            __E64CoreFetchFaultAt(core, pc + avail);                        \
        }                                                                   \
        else                                                                \
        {                                                                   \
            core->cr_state.crexc.exception = kE64ExceptionBadInstruction;   \
        }                                                                   \
        return;                                                             \
    } while(0)

    E64Opcode opcode = (UInt8)bb_read(&bb, 8);
    if(unlikely(opcode > kE64OpcodeMAX))
    {
        core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
        return;
    }

    core->op.opcode = opcode;
    core->op.opce = &kE64OpfuncTable[opcode];
    core->op.ce = NULL;
    core->op.param = core->op.param_store;
    core->op.param_coding = core->op.coding_store;

    UInt8 maxarg = core->op.opce->maxargs;
    UInt8 i = 0;
    Boolean offsetMode = false;
    Boolean offsetAdd = false;
    UInt8 offsetI = 0;
    for(; i < maxarg; i++)
    {
        E64ParameterCoding coding = (UInt8)bb_read(&bb, 4);
        core->op.param_coding[i] = coding;
        switch(coding)
        {
            case kE64ParameterCodingEnd:
                /* a the weekend reference lol */
                goto escape_from_la;
            case kE64ParameterCodingReg:
                rec[i].kind = kE64OperandReg;
                rec[i].reg = (UInt8)bb_read(&bb, 4);
                core->op.param[i] = &(core->rl[rec[i].reg]);
                break;
            case kE64ParameterCodingRegExtended:
                rec[i].kind = kE64OperandRegExtended;
                rec[i].reg = (UInt8)bb_read(&bb, 4);
                core->op.param[i] = &(core->erl[rec[i].reg]);
                break;
            case kE64ParameterCodingRegFloat:
                rec[i].kind = kE64OperandRegFloat;
                rec[i].reg = (UInt8)bb_read(&bb, 4);
                core->op.param[i] = &(core->frl[rec[i].reg].u64);
                break;
            case kE64ParameterCodingAddr64:
                bb_align(&bb);
                [[fallthrough]];
            case kE64ParameterCodingImm4:
            case kE64ParameterCodingImm8:
            case kE64ParameterCodingImm16:
            case kE64ParameterCodingImm32:
            case kE64ParameterCodingImm64:
                core->op.immcache[i] = bb_read(&bb, kImmBits[coding]);
                core->op.param[i] = &(core->op.immcache[i]);
                rec[i].kind = kE64OperandImm;
                rec[i].imm = core->op.immcache[i];
                break;
            case kE64ParameterCodingRegImmInc:
                rec[i].kind = kE64OperandRegInc;
                rec[i].reg = (UInt8)bb_read(&bb, 4);
                core->op.immcache[i] = core->rl[rec[i].reg]++;
                core->op.param[i] = &(core->op.immcache[i]);
                break;
            case kE64ParameterCodingRegImmDec:
                rec[i].kind = kE64OperandRegDec;
                rec[i].reg = (UInt8)bb_read(&bb, 4);
                core->op.immcache[i] = core->rl[rec[i].reg]--;
                core->op.param[i] = &(core->op.immcache[i]);
                break;
            case kE64ParameterCodingOffsetAdd:
            case kE64ParameterCodingOffsetSub:
                if(unlikely(offsetMode))
                {
                    /* shouldn't be in offsetting mode nor the first argument */
                    EMEX64_BAD_INSTRUCTION();
                }
                offsetMode = true;
                offsetAdd = (coding == kE64ParameterCodingOffsetAdd);
                maxarg += 3;
                offsetI = 2;
                goto skip_after_offsetmode_branch;
            default:
                /* illegal coding */
                EMEX64_BAD_INSTRUCTION();
        }

        if(offsetMode)
        {
            if(--offsetI <= 0)
            {
                /* forcing operand to be offsetted to be a intermediate and giving it the offset */
                core->op.immcache[i - 2] = *core->op.param[i - 1];
                if(offsetAdd)
                {
                    core->op.immcache[i - 2] += *core->op.param[i];
                }
                else
                {
                    core->op.immcache[i - 2] -= *core->op.param[i];
                }
                core->op.param[i - 2] = &(core->op.immcache[i - 2]);    /* overriding the operand with the offsetted one */

                emex64_operand_recipe_t *o = &rec[i - 2];
                o->kind = kE64OperandOffset;
                o->add = offsetAdd;
                o->base_kind = rec[i - 1].kind;
                o->base_reg = rec[i - 1].reg;
                o->imm = rec[i - 1].imm;
                o->off_kind = rec[i].kind;
                o->off_reg = rec[i].reg;
                o->off_imm = rec[i].imm;

                /* offsets aren't counted as operands */
                i -= 2;
                maxarg -= 3;
                offsetMode = false;
            }
        }

    skip_after_offsetmode_branch:
    }

escape_from_la:

    /* offsetting mode shall not be enabled now */
    if(unlikely(offsetMode))
    {
        EMEX64_BAD_INSTRUCTION();
    }

    /*
     * now we know all about this instruction, the
     * lenght and the amount of parameters, this is
     * very very very good.
     *
     * we had sex in the studio~~ nobody's watching~~   (the weekend escape from la reference xD)
     */
    core->op.param_cnt = i;
    core->op.ilen = (UInt32)((bb.pos + 7u) >> 3);

    if(unlikely(core->op.ilen > avail))
    {
        __E64CoreFetchFaultAt(core, pc + avail);
        return;
    }

    if(unlikely(i < core->op.opce->minargs))
    {
        core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
    }
    else
    {
        if(have_pa && i <= EMEX64_ICACHE_ARGS && (pc & EMEX64_PAGE_MASK) + core->op.ilen <= EMEX64_PAGE_SIZE)
        {
            UInt64 first_chunk = fetch_pa >> EMEX64_CHUNK_SHIFT;
            UInt64 last_chunk = (fetch_pa + core->op.ilen - 1) >> EMEX64_CHUNK_SHIFT;
            emex64_snoop_mark(core->snoop, fetch_pa, EMEX64_CHUNK_CODE);
            emex64_snoop_mark(core->snoop, fetch_pa + core->op.ilen - 1, EMEX64_CHUNK_CODE);
            emex64_icache_entry_t *ce = &core->icache[emex64_icache_index(fetch_pa)];
            emex64_icache_cold_t *cc = &core->icold[ce - core->icache];
            ce->paddr = fetch_pa;
            ce->code_seq = __atomic_load_n(&core->snoop->code_seq, __ATOMIC_ACQUIRE);
            cc->gen_first = emex64_snoop_gen(core->snoop, first_chunk << EMEX64_CHUNK_SHIFT);
            cc->gen_last = emex64_snoop_gen(core->snoop, last_chunk << EMEX64_CHUNK_SHIFT);
            cc->opce = core->op.opce;
            ce->ilen = core->op.ilen;
            cc->opcode = core->op.opcode;
            cc->param_cnt = i;
            ce->rollback = __E64CoreNeedsRollback(core->op.opcode, rec, i);
            memcpy(cc->param_coding, core->op.coding_store, sizeof(cc->param_coding));
            memcpy(cc->operands, rec, sizeof(emex64_operand_recipe_t) * i);
            __E64CoreFillCacheEntry(core, ce, rec, i);
            if(EMEX64_KTEXT_SHARED)
            {
                __E64CoreTextPublish(core, ce);
            }
        }

        core->op.opce->func(core);
    }

    /* the part of executing the instruction */
    core->rl[kE64RegisterPC] += core->op.ilen;

    return;
}
#undef EMEX64_BAD_INSTRUCTION

static inline void __E64CoreExecuteInstructionAtPC(__E64Core core)
{
    if(unlikely(core->halted))
    {
        return;
    }

    UInt64 pc = core->rl[kE64RegisterPC];
    UInt64 fetch_pa = 0;
    Boolean have_pa = false;
    if(likely(core->icache != NULL))
    {
        have_pa = __E64CoreFetchAddress(core, pc, &fetch_pa);
        if(likely(have_pa))
        {
            const emex64_icache_entry_t *ce = &core->icache[emex64_icache_index(fetch_pa)];
            const emex64_icache_cold_t *cc = &core->icold[ce - core->icache];
            if(likely(ce->paddr == fetch_pa) &&
               likely(emex64_snoop_gen(core->snoop, fetch_pa) == cc->gen_first) &&
               likely(emex64_snoop_gen(core->snoop, fetch_pa + ce->ilen - 1) == cc->gen_last) &&
               likely(__E64CoreKTRRAllowsFetch(core, fetch_pa, ce->ilen)))
            {
                __E64CoreReplay(core, ce);
                core->op.opce->func(core);
                core->rl[kE64RegisterPC] += core->op.ilen;
                return;
            }
        }
    }

    __E64CoreDecodeAndExecute(core, pc, have_pa, fetch_pa);
}

static void __E64CoreTrapEnter(__E64Core core,
                               UInt64 cause,
                               UInt64 epc,
                               UInt64 tval)
{
    UInt64 st = core->cr_state.crstat;
    UInt64 el = core->cr_state.crel.level;

    st &= ~(E64_CRSTAT_PIE | E64_CRSTAT_PEL_MASK);
    if(st & E64_CRSTAT_IE)
    {
        st |= E64_CRSTAT_PIE;
    }
    st |= (el << E64_CRSTAT_PEL_SHIFT) & E64_CRSTAT_PEL_MASK;
    st &= ~E64_CRSTAT_IE;
    core->cr_state.crstat = st;

    if(el < kE64ElevationLevelKernel)
    {
        core->cr_state.crel.level = kE64ElevationLevelKernel;
    }

    core->cr_state.crepc = epc;
    core->cr_state.crcause = cause;
    core->cr_state.crtval = tval;
    core->rl[kE64RegisterPC] = core->cr_state.crtvec;
    core->halted = false;
    core->wfe = false;
    core->fetch.vpage = ~0ULL;
    core->irq_ie = false;
}

static inline void __E64CoreSaveForRollback(__E64Core core,
                                            const emex64_icache_entry_t *ce)
{
    const emex64_icache_cold_t *cc = ce != NULL ? &core->icold[ce - core->icache] : NULL;
    if(ce == NULL || cc->save_rl == 0xFFFF)
    {
        memcpy(core->trap.rl, core->rl, sizeof(core->trap.rl));
    }
    else
    {
        for(UInt16 m = cc->save_rl; m != 0; m &= m - 1)
        {
            UInt8 r = (UInt8)__builtin_ctz(m);
            core->trap.rl[r] = core->rl[r];
        }
    }
    if(ce == NULL || cc->save_erl)
    {
        memcpy(core->trap.erl, core->erl, sizeof(core->trap.erl));
    }
}

static __attribute__((noinline)) void __E64CoreTrapFromException(__E64Core core,
                                                                 const emex64_icache_entry_t *ce,
                                                                 UInt64 pc)
{
    const emex64_icache_cold_t *cc = ce != NULL ? &core->icold[ce - core->icache] : NULL;
    UInt64 cause = core->cr_state.crexc.exception;
    UInt64 tval = 0;
    if(core->trap.fault_valid)
    {
        cause |= (UInt64)core->trap.fault_access << E64_CRCAUSE_ACCESS_SHIFT;
        tval = core->trap.fault_addr;
    }

    if(ce == NULL || ce->rollback)
    {
        for(UInt16 m = ce != NULL ? cc->save_rl : 0xFFFF; m != 0; m &= m - 1)
        {
            UInt8 r = (UInt8)__builtin_ctz(m);
            core->rl[r] = core->trap.rl[r];
        }
        if(ce == NULL || cc->save_erl)
        {
            memcpy(core->erl, core->trap.erl, sizeof(core->erl));
        }
    }
    core->cr_state.crexc.exception = kE64ExceptionNone;
    core->trap.fault_valid = false;
    core->trap.svc_pending = false;
    core->halted = false;

    if(unlikely(pc == core->cr_state.crtvec && core->trap.fault_access == E64_ACCESS_EXECUTE && (cause >> E64_CRCAUSE_ACCESS_SHIFT) == E64_ACCESS_EXECUTE))
    {
        fprintf(stderr, "emex64vm: double fault: the trap vector 0x%llx cannot be fetched\n", (unsigned long long)pc);
        if(((__E64Machine)core->machine)->embedded)
        {
            E64CoreTerminate((E64CoreRef)core);
            return;
        }
        exit(3);
    }

    __E64CoreTrapEnter(core, cause, pc, tval);
}

static inline UInt64 __E64CoreFetchContext(__E64Core core)
{
    return (UInt64)core->cr_state.crel.level | ((UInt64)(core->cr_state.crptb.enabled != 0) << 8) | ((UInt64)(core->in_interrupt != 0) << 9) | ((UInt64)(core->snoop->ktrr_locked != 0) << 10) | (core->tlb_seq << 16);
}

#define EMEX64_KTEXT_MAX_PAGES 65536

static __attribute__((noinline)) void __E64CoreTextSetup(__E64Core core,
                                                         UInt64 pc,
                                                         UInt64 fetch_pa)
{
    emex64_snoop_t *snoop = core->snoop;
    UInt64 base = snoop->ktrr_base;
    UInt64 pages = (snoop->ktrr_size + EMEX64_PAGE_MASK) >> 13;

    core->ktext.size = 0;
    if(pages == 0 || pages > EMEX64_KTEXT_MAX_PAGES || (base & EMEX64_PAGE_MASK) != 0)
    {
        return;
    }
    if(pages != core->ktext.pages)
    {
        free(core->ktext.runs);
        core->ktext.runs = calloc((pages + 7) / 8, 1);
        core->ktext.pages = core->ktext.runs != NULL ? pages : 0;
        if(core->ktext.runs == NULL)
        {
            return;
        }
    }
    else
    {
        memset(core->ktext.runs, 0, (pages + 7) / 8);
    }

    UInt64 seq = __atomic_load_n(&snoop->ktext_seq, __ATOMIC_ACQUIRE);
    UInt64 vbase = (pc & ~(UInt64)EMEX64_PAGE_MASK) - ((fetch_pa & ~(UInt64)EMEX64_PAGE_MASK) - base);
    UInt64 pte = 0, root = 0;
    Boolean any = false;
    __E64MemoryCoreWatchPage(core->machine->memory, core, vbase, &pte, &root);
    for(UInt64 i = 0; i < pages; i++)
    {
        if(__E64MemoryCoreWatchPage(core->machine->memory, core, vbase + (i << 13), &pte, NULL) &&
           (pte & kE64MMUPTExec) && ((pte & EMEX64_MEMORY_MMU_MASK_PFN) >> 8) << 13 == base + (i << 13))
        {
            core->ktext.runs[i >> 3] |= (UInt8)(1u << (i & 7));
            any = true;
        }
    }
    if(!any)
    {
        return;
    }

    if(EMEX64_KTEXT_SHARED && !core->ktext.flushed && core->icache != NULL)
    {
        for(UInt64 k = 0; k < EMEX64_ICACHE_ENTRIES; k++)
        {
            if(core->icache[k].paddr - base < (pages << 13))
            {
                core->icache[k].paddr = ~0ULL;
            }
        }
        core->ktext.flushed = true;
    }

    core->ktext.vbase = vbase;
    core->ktext.pbase = base;
    core->ktext.seq = seq;
    core->ktext.context = emex64_data_context(core) & 0xFFFF;
    core->ktext.root = root;
    core->ktext.size = pages << 13;
}

static inline Boolean __E64CoreTextLive(__E64Core core,
                                        UInt64 context)
{
    return core->ktext.size != 0 && (context & 0xFFFF) == core->ktext.context && __atomic_load_n(&core->snoop->ktext_seq, __ATOMIC_ACQUIRE) == core->ktext.seq;
}

static inline __attribute__((always_inline)) Boolean __E64CoreTextFetch(__E64Core core,
                                                                        UInt64 context,
                                                                        UInt64 pc,
                                                                        UInt64 *fetch_pa)
{
    UInt64 off = pc - core->ktext.vbase;
    if(off >= core->ktext.size || unlikely(!__E64CoreTextLive(core, context)))
    {
        return false;
    }
    UInt64 page = off >> 13;
    if(unlikely(!((core->ktext.runs[page >> 3] >> (page & 7)) & 1)))
    {
        return false;
    }
    *fetch_pa = core->ktext.pbase + off;
    return true;
}

static inline UInt64 __E64CoreDataContext(__E64Core core)
{
    return likely(core->cr_state.crexc.exception == kE64ExceptionNone) ? emex64_data_context(core) : 0;
}

static inline __attribute__((always_inline)) Boolean __E64CoreCacheEntryIsGood(__E64Core core,
                                                                               emex64_icache_entry_t *ce,
                                                                               UInt64 fetch_pa)
{
    if(unlikely(ce->paddr != fetch_pa))
    {
        return false;
    }
    UInt64 code_seq = __atomic_load_n(&core->snoop->code_seq, __ATOMIC_ACQUIRE);
    if(likely(ce->code_seq == code_seq))
    {
        return true;
    }
    const emex64_icache_cold_t *cc = &core->icold[ce - core->icache];
    if(emex64_snoop_gen(core->snoop, fetch_pa) != cc->gen_first ||
       emex64_snoop_gen(core->snoop, fetch_pa + ce->ilen - 1) != cc->gen_last)
    {
        return false;
    }
    ce->code_seq = code_seq;
    return true;
}

static __attribute__((noinline)) emex64_icache_entry_t *__E64CoreFetchSlow(__E64Core core,
                                                                           UInt64 pc,
                                                                           Boolean *have_pa,
                                                                           UInt64 *fetch_pa)
{
    core->fetch.vpage = ~0ULL;
    *have_pa = false;
    *fetch_pa = 0;
    if(unlikely(core->icache == NULL))
    {
        return NULL;
    }

    const emex64_snoop_t *snoop = core->snoop;
    UInt64 map_seq = __atomic_load_n(&snoop->map_seq, __ATOMIC_ACQUIRE);

    *have_pa = __E64CoreFetchAddress(core, pc, fetch_pa);
    if(unlikely(!*have_pa))
    {
        return NULL;
    }

    if(EMEX64_KTEXT && snoop->ktrr_locked && core->cr_state.crel.level >= kE64ElevationLevelKernel &&
       core->cr_state.crptb.enabled && !core->in_interrupt &&
       *fetch_pa - snoop->ktrr_base < snoop->ktrr_size && !__E64CoreTextLive(core, emex64_data_context(core)))
    {
        if(core->ktext.wait != 0)
        {
            core->ktext.wait--;
        }
        else
        {
            core->ktext.wait = 255;
            __E64CoreTextSetup(core, pc, *fetch_pa);
        }
    }

    UInt64 ppage = *fetch_pa & ~(UInt64)EMEX64_PAGE_MASK;
    if((likely(!snoop->ktrr_locked) || core->cr_state.crel.level < kE64ElevationLevelKernel ||
        (ppage >= snoop->ktrr_base && ppage + EMEX64_PAGE_SIZE <= snoop->ktrr_base + snoop->ktrr_size)) &&
       ppage + EMEX64_PAGE_SIZE <= core->ram_size)
    {
        Boolean whole_page = true;
        if(core->cr_state.crptb.enabled && !core->in_interrupt)
        {
            const emex64_tlb_entry_t *e = emex64_tlb_lookup(&core->tlb, core->snoop, pc);
            UInt8 check = kE64MMUPTExec;
            if(core->cr_state.crel.level < kE64ElevationLevelKernel)
            {
                check |= kE64MMUPTUser;
            }
            whole_page = e != NULL && (e->flags & check) == check && e->ppage == ppage;
        }
        if(whole_page)
        {
            core->fetch.ppage = ppage;
            core->fetch.map_seq = map_seq;
            core->fetch.context = __E64CoreFetchContext(core);
            core->fetch.vpage = pc & ~(UInt64)EMEX64_PAGE_MASK;

            UInt64 slot = (pc >> 13) & (EMEX64_FETCH_PAGES - 1);
            core->fetch.pages[slot].vpage = core->fetch.vpage;
            core->fetch.pages[slot].ppage = ppage;
            core->fetch.pages[slot].map_seq = map_seq;
            core->fetch.pages[slot].context = core->fetch.context;
        }
    }

    emex64_icache_entry_t *ce = &core->icache[emex64_icache_index(*fetch_pa)];
    if(unlikely(!__E64CoreCacheEntryIsGood(core, ce, *fetch_pa)) ||
       unlikely(!__E64CoreKTRRAllowsFetch(core, *fetch_pa, ce->ilen)))
    {
        return NULL;
    }
    return ce;
}

static UInt32 __E64CoreRunTrapModel(__E64Core core,
                                    UInt32 budget)
{
    if((core->cr_state.crstat & E64_CRSTAT_IE) != 0 && (core->irq_poll || !core->irq_ie))
    {
        core->irq_poll = false;
        EFIndex irq = __E64CoreTakeInterrupt(core);
        if(irq >= 0)
        {
            __E64CoreTrapEnter(core, E64_CRCAUSE_INTERRUPT | (UInt64)irq, core->rl[kE64RegisterPC], 0);
        }
    }
    core->irq_ie = (core->cr_state.crstat & E64_CRSTAT_IE) != 0;

    if(core->halted)
    {
        if(__E64CoreHasPendingInterrupt(core) ||
           (core->wfe && __atomic_exchange_n(&core->event, false, __ATOMIC_ACQ_REL)))
        {
            core->halted = false;
            core->wfe = false;
            core->irq_poll = true;
        }
        else
        {
            __E64CoreIdle(core);
        }
        return 1;
    }

    if(core->fetch.context != __E64CoreFetchContext(core))
    {
        core->fetch.vpage = ~0ULL;
    }
    core->run_break = false;

    const emex64_snoop_t *snoop = core->snoop;
    emex64_icache_entry_t *hint = NULL;
    emex64_icache_entry_t *ce;
    UInt64 pc = 0;
    UInt64 fetch_pa = 0;
    UInt64 code_seq = 0;
    UInt64 data_context = __E64CoreDataContext(core);
    Boolean have_pa;
    Boolean quick = false;
    UInt32 ran = 0;

    const emex64_ktext_store_t *text_store = &snoop->text;
    emex64_ktext_entry_t *text = NULL;
    emex64_ktext_entry_t *text_next = NULL;
    Boolean text_skip = false;
    for(;;)
    {
        ran++;

        if(text_next != NULL)
        {
            text = text_next;
            goto text_dispatch;
        }

        if(likely(quick) && likely(hint->paddr == fetch_pa) && likely(hint->code_seq == code_seq))
        {
            have_pa = true;
            ce = hint;
        }
        else if(likely(hint != NULL) && likely(__atomic_load_n(&snoop->map_seq, __ATOMIC_RELAXED) == core->fetch.map_seq))
        {
            have_pa = true;
            ce = hint;
            if(unlikely(!__E64CoreCacheEntryIsGood(core, ce, fetch_pa)))
            {
                ce = NULL;
            }
            else
            {
                code_seq = ce->code_seq;
            }
        }
        else
        {
            Boolean skip = text_skip;
            text_skip = false;

            pc = core->rl[kE64RegisterPC];
            if(EMEX64_KTEXT && __E64CoreTextFetch(core, data_context, pc, &fetch_pa))
            {
                have_pa = true;
                if(EMEX64_KTEXT_SHARED && !skip && (text = emex64_ktext_find(text_store, fetch_pa)) != NULL)
                {
                    goto text_dispatch;
                }
                ce = &core->icache[emex64_icache_index(fetch_pa)];
                if(unlikely(!__E64CoreCacheEntryIsGood(core, ce, fetch_pa)))
                {
                    ce = NULL;
                }
            }
            else if(likely((pc & ~(UInt64)EMEX64_PAGE_MASK) == core->fetch.vpage) &&
                    likely(__atomic_load_n(&snoop->map_seq, __ATOMIC_RELAXED) == core->fetch.map_seq))
            {
                have_pa = true;
                fetch_pa = core->fetch.ppage + (pc & EMEX64_PAGE_MASK);
                ce = &core->icache[emex64_icache_index(fetch_pa)];
                if(unlikely(!__E64CoreCacheEntryIsGood(core, ce, fetch_pa)))
                {
                    ce = NULL;
                }
            }
            else
            {
                UInt64 slot = (pc >> 13) & (EMEX64_FETCH_PAGES - 1);
                if(likely(core->fetch.pages[slot].vpage == (pc & ~(UInt64)EMEX64_PAGE_MASK)) &&
                   likely(core->fetch.pages[slot].map_seq == __atomic_load_n(&snoop->map_seq, __ATOMIC_RELAXED)) &&
                   likely(core->fetch.pages[slot].context == __E64CoreFetchContext(core)))
                {
                    core->fetch.vpage = core->fetch.pages[slot].vpage;
                    core->fetch.ppage = core->fetch.pages[slot].ppage;
                    core->fetch.map_seq = core->fetch.pages[slot].map_seq;
                    core->fetch.context = core->fetch.pages[slot].context;
                    have_pa = true;
                    fetch_pa = core->fetch.ppage + (pc & EMEX64_PAGE_MASK);
                    ce = &core->icache[emex64_icache_index(fetch_pa)];
                    if(unlikely(!__E64CoreCacheEntryIsGood(core, ce, fetch_pa)))
                    {
                        ce = NULL;
                    }
                }
                else
                {
                    ce = __E64CoreFetchSlow(core, pc, &have_pa, &fetch_pa);
                }
            }
            if(ce != NULL)
            {
                code_seq = ce->code_seq;
            }
        }

        if(likely(ce != NULL))
        {
            UInt64 *const *q = ce->q;
            switch(ce->fast)
            {
                case kE64FastMov:
                    *q[0] = *q[1];
                    goto sequential;
                case kE64FastAdd:
                    *q[0] = *q[1] + *q[2];
                    goto sequential;
                case kE64FastSub:
                    *q[0] = *q[1] - *q[2];
                    goto sequential;
                case kE64FastMul:
                    *q[0] = *q[1] * *q[2];
                    goto sequential;
                case kE64FastAnd:
                    *q[0] = *q[1] & *q[2];
                    goto sequential;
                case kE64FastOr:
                    *q[0] = *q[1] | *q[2];
                    goto sequential;
                case kE64FastXor:
                    *q[0] = *q[1] ^ *q[2];
                    goto sequential;
                case kE64FastShl:
                    *q[0] = *q[1] << *q[2];
                    goto sequential;
                case kE64FastShr:
                    *q[0] = *q[1] >> *q[2];
                    goto sequential;
                case kE64FastCmp:
                {
                    SInt64 a = (SInt64)*q[0];
                    SInt64 b = (SInt64)*q[1];
                    core->rl[kE64RegisterCF] = (a == b) * kE64CompareFlagZ | (a <  b) * kE64CompareFlagL | (a >  b) * kE64CompareFlagG;
                    goto sequential;
                }

#define E64_FAST_BRANCH(kind, taken)                                    \
                case kind:                                              \
                    if(taken)                                           \
                    {                                                   \
                        goto branched;                                  \
                    }                                                   \
                    goto sequential;
                E64_FAST_BRANCH(kE64FastBranch, true)
                E64_FAST_BRANCH(kE64FastBranchFlags, (core->rl[kE64RegisterCF] & ce->cf_mask) != 0)
                E64_FAST_BRANCH(kE64FastBranchFlagsNot, (core->rl[kE64RegisterCF] & ce->cf_mask) == 0)
                E64_FAST_BRANCH(kE64FastBranchZero, *q[0] == 0)
                E64_FAST_BRANCH(kE64FastBranchNotZero, *q[0] != 0)
                E64_FAST_BRANCH(kE64FastBranchEqual, *q[0] == *q[1])
                E64_FAST_BRANCH(kE64FastBranchNotEqual, *q[0] != *q[1])
                E64_FAST_BRANCH(kE64FastBranchLess, (SInt64)*q[0] < (SInt64)*q[1])
                E64_FAST_BRANCH(kE64FastBranchGreaterEqual, (SInt64)*q[0] >= (SInt64)*q[1])
                E64_FAST_BRANCH(kE64FastBranchLessUnsigned, *q[0] < *q[1])
                E64_FAST_BRANCH(kE64FastBranchGreaterEqualUnsigned, *q[0] >= *q[1])
#undef E64_FAST_BRANCH

#define E64_FAST_LOAD(kind, size, extend)                               \
                case kind:                                              \
                {                                                       \
                    UInt64 value;                                       \
                    if(likely(emex64_hot_load(core, data_context, *q[1] + (UInt64)(SInt64)ce->rel, size, &value)))  \
                    {                                                   \
                        *q[0] = extend;                                 \
                        goto sequential;                                \
                    }                                                   \
                    break;                                              \
                }
                E64_FAST_LOAD(kE64FastLoad8, 1, value)
                E64_FAST_LOAD(kE64FastLoad16, 2, value)
                E64_FAST_LOAD(kE64FastLoad32, 4, value)
                E64_FAST_LOAD(kE64FastLoad64, 8, value)
                E64_FAST_LOAD(kE64FastLoadSigned8, 1, (UInt64)(SInt64)(SInt8)value)
                E64_FAST_LOAD(kE64FastLoadSigned16, 2, (UInt64)(SInt64)(SInt16)value)
                E64_FAST_LOAD(kE64FastLoadSigned32, 4, (UInt64)(SInt64)(SInt32)value)
#undef E64_FAST_LOAD

#define E64_FAST_STORE(kind, size)                                      \
                case kind:                                              \
                {                                                       \
                    Boolean touched;                                    \
                    if(likely(emex64_hot_store(core, data_context, *q[1] + (UInt64)(SInt64)ce->rel, size, *q[0], &touched)))  \
                    {                                                   \
                        if(unlikely(touched))                           \
                        {                                               \
                            goto stored;                                \
                        }                                               \
                        goto sequential;                                \
                    }                                                   \
                    break;                                              \
                }
                E64_FAST_STORE(kE64FastStore8, 1)
                E64_FAST_STORE(kE64FastStore16, 2)
                E64_FAST_STORE(kE64FastStore32, 4)
                E64_FAST_STORE(kE64FastStore64, 8)
#undef E64_FAST_STORE

                case kE64FastCall:
                {
                    Boolean touched;
                    if(likely(emex64_hot_call(core, data_context, pc + ce->ilen, &touched)))
                    {
                        goto branched;
                    }
                    break;
                }
                case kE64FastCallArgs:
                {
                    const emex64_icache_cold_t *cc = &core->icold[ce - core->icache];
                    UInt64 arg[EMEX64_CALL_ARGS];
                    UInt8 args = cc->param_cnt - 1;
                    Boolean touched;
                    __E64CoreReplay(core, ce);
                    for(UInt8 i = 0; i < args; i++)
                    {
                        arg[i] = *cc->param[i + 1];
                    }
                    if(likely(emex64_hot_call(core, data_context, pc + ce->ilen, &touched)))
                    {
                        for(UInt8 i = 0; i < args; i++)
                        {
                            core->rl[kE64RegisterR0 + i] = arg[i];
                        }
                        goto branched;
                    }
                    break;
                }
                case kE64FastCallIndirect:
                {
                    UInt64 to = *q[0];
                    Boolean touched;
                    if(likely(emex64_hot_call(core, data_context, pc + ce->ilen, &touched)))
                    {
                        core->rl[kE64RegisterPC] = to;
                        goto jumped;
                    }
                    break;
                }
                case kE64FastReturn:
                case kE64FastReturnAuth:
                {
                    UInt64 frame = core->rl[kE64RegisterFP];
                    UInt64 fp, to;
                    if(likely(emex64_hot_frame(core, data_context, frame, &fp, &to)) &&
                       (ce->fast == kE64FastReturn || likely(emex64_fast_reta(core, frame, to, &to))))
                    {
                        core->rl[kE64RegisterSP] = frame + 16;
                        core->rl[kE64RegisterFP] = fp;
                        core->rl[kE64RegisterPC] = to;
                        goto jumped;
                    }
                    break;
                }
                case kE64FastSignReturn:
                {
                    Boolean touched;
                    if(likely(data_context != 0) && likely(emex64_fast_pacret(core, &touched)))
                    {
                        if(unlikely(touched))
                        {
                            goto stored;
                        }
                        goto sequential;
                    }
                    break;
                }
                case kE64FastNone:
                    break;
                default:
                    __builtin_unreachable();
            }

            core->trap.fault_valid = false;
            if(unlikely(ce->rollback))
            {
                __E64CoreSaveForRollback(core, ce);
            }
            __E64CoreReplay(core, ce);
            core->op.opce->func(core);
            core->rl[kE64RegisterPC] += core->op.ilen;
        }
        else
        {
            core->trap.fault_valid = false;
            __E64CoreSaveForRollback(core, NULL);
            __E64CoreDecodeAndExecute(core, pc, have_pa, fetch_pa);
        }

        if(unlikely(core->cr_state.crexc.exception != kE64ExceptionNone))
        {
            __E64CoreTrapFromException(core, ce, pc);
        }
        else if(unlikely(core->trap.svc_pending))
        {
            core->trap.svc_pending = false;
            __E64CoreTrapEnter(core, kE64ExceptionSVC, core->rl[kE64RegisterPC], 0);
        }

        hint = NULL;
        quick = false;
        if(ran >= budget || core->halted || core->run_break ||
           unlikely(__atomic_load_n(&core->request, __ATOMIC_RELAXED) != 0))
        {
            break;
        }

        data_context = __E64CoreDataContext(core);
        continue;

    jumped:
        hint = NULL;
        quick = false;
        if(ran >= budget || unlikely(__atomic_load_n(&core->request, __ATOMIC_RELAXED) != 0))
        {
            break;
        }
        continue;

    sequential:
        pc += ce->ilen;
        fetch_pa += ce->ilen;
        hint = ce->seq;
        quick = hint != NULL;
        goto moved;
    stored:
        pc += ce->ilen;
        fetch_pa += ce->ilen;
        hint = ce->seq;
        quick = false;
        goto moved;
    branched:
        pc += (UInt64)(SInt64)ce->rel;
        fetch_pa += (UInt64)(SInt64)ce->rel;
        hint = ce->target;
        quick = false;
    moved:
        core->rl[kE64RegisterPC] = pc;
        if(ran >= budget || (unlikely(!quick) && unlikely(__atomic_load_n(&core->request, __ATOMIC_RELAXED) != 0)))
        {
            break;
        }
        continue;

#define TR(i)   (*(UInt64 *)((UInt8 *)core + text->r[i]))
#define TI      (text->imm)
    text_dispatch:
        hint = NULL;
        quick = false;
        switch(text->fast)
        {
            case kE64TextMovRR:
                TR(0) = TR(1);
                goto text_sequential;
            case kE64TextMovRI:
                TR(0) = TI;
                goto text_sequential;

#define E64_TEXT_ALU(name, op)                                          \
            case kE64Text##name##RR:                                    \
                TR(0) = TR(1) op TR(2);                                 \
                goto text_sequential;                                   \
            case kE64Text##name##RI:                                    \
                TR(0) = TR(1) op TI;                                    \
                goto text_sequential;                                   \
            case kE64Text##name##IR:                                    \
                TR(0) = TI op TR(2);                                    \
                goto text_sequential;
            E64_TEXT_ALU(Add, +)
            E64_TEXT_ALU(Sub, -)
            E64_TEXT_ALU(Mul, *)
            E64_TEXT_ALU(And, &)
            E64_TEXT_ALU(Or, |)
            E64_TEXT_ALU(Xor, ^)
            E64_TEXT_ALU(Shl, <<)
            E64_TEXT_ALU(Shr, >>)
#undef E64_TEXT_ALU

#define E64_TEXT_CMP(variant, first, second)                            \
            case kE64TextCmp##variant:                                  \
            {                                                           \
                SInt64 a = (SInt64)(first);                             \
                SInt64 b = (SInt64)(second);                            \
                core->rl[kE64RegisterCF] = (a == b) * kE64CompareFlagZ | (a <  b) * kE64CompareFlagL | (a >  b) * kE64CompareFlagG;  \
                goto text_sequential;                                   \
            }
            E64_TEXT_CMP(RR, TR(1), TR(2))
            E64_TEXT_CMP(RI, TR(1), TI)
            E64_TEXT_CMP(IR, TI, TR(2))
#undef E64_TEXT_CMP

#define E64_TEXT_BRANCH(kind, taken)                                    \
            case kind:                                                  \
                if(taken)                                               \
                {                                                       \
                    goto text_branched;                                 \
                }                                                       \
                goto text_sequential;
            E64_TEXT_BRANCH(kE64TextBranch, true)
            E64_TEXT_BRANCH(kE64TextBranchFlags, (core->rl[kE64RegisterCF] & text->cf_mask) != 0)
            E64_TEXT_BRANCH(kE64TextBranchFlagsNot, (core->rl[kE64RegisterCF] & text->cf_mask) == 0)
            E64_TEXT_BRANCH(kE64TextBranchZero, TR(1) == 0)
            E64_TEXT_BRANCH(kE64TextBranchNotZero, TR(1) != 0)
#define E64_TEXT_COMPARE(name, cond)                                    \
            case kE64TextBranch##name##RR:                              \
            {                                                           \
                UInt64 a = TR(1), b = TR(2);                            \
                if(cond)                                                \
                {                                                       \
                    goto text_branched;                                 \
                }                                                       \
                goto text_sequential;                                   \
            }                                                           \
            case kE64TextBranch##name##RI:                              \
            {                                                           \
                UInt64 a = TR(1), b = TI;                               \
                if(cond)                                                \
                {                                                       \
                    goto text_branched;                                 \
                }                                                       \
                goto text_sequential;                                   \
            }                                                           \
            case kE64TextBranch##name##IR:                              \
            {                                                           \
                UInt64 a = TI, b = TR(2);                               \
                if(cond)                                                \
                {                                                       \
                    goto text_branched;                                 \
                }                                                       \
                goto text_sequential;                                   \
            }
            E64_TEXT_COMPARE(Equal, a == b)
            E64_TEXT_COMPARE(NotEqual, a != b)
            E64_TEXT_COMPARE(Less, (SInt64)a < (SInt64)b)
            E64_TEXT_COMPARE(GreaterEqual, (SInt64)a >= (SInt64)b)
            E64_TEXT_COMPARE(LessUnsigned, a < b)
            E64_TEXT_COMPARE(GreaterEqualUnsigned, a >= b)
#undef E64_TEXT_COMPARE
#undef E64_TEXT_BRANCH

#define E64_TEXT_LOAD(kind, size, extend)                               \
            case kind:                                                  \
            {                                                           \
                UInt64 value;                                           \
                if(likely(emex64_hot_load(core, data_context, TR(1) + (UInt64)(SInt64)text->rel, size, &value)))  \
                {                                                       \
                    TR(0) = extend;                                     \
                    goto text_sequential;                               \
                }                                                       \
                goto text_fallback;                                     \
            }
            E64_TEXT_LOAD(kE64TextLoad8, 1, value)
            E64_TEXT_LOAD(kE64TextLoad16, 2, value)
            E64_TEXT_LOAD(kE64TextLoad32, 4, value)
            E64_TEXT_LOAD(kE64TextLoad64, 8, value)
            E64_TEXT_LOAD(kE64TextLoadSigned8, 1, (UInt64)(SInt64)(SInt8)value)
            E64_TEXT_LOAD(kE64TextLoadSigned16, 2, (UInt64)(SInt64)(SInt16)value)
            E64_TEXT_LOAD(kE64TextLoadSigned32, 4, (UInt64)(SInt64)(SInt32)value)
#undef E64_TEXT_LOAD

#define E64_TEXT_STORE(kind, size, what)                                \
            case kind:                                                  \
            {                                                           \
                Boolean touched;                                        \
                if(likely(emex64_hot_store(core, data_context, TR(1) + (UInt64)(SInt64)text->rel, size, what, &touched)))  \
                {                                                       \
                    if(unlikely(touched))                               \
                    {                                                   \
                        goto text_stored;                               \
                    }                                                   \
                    goto text_sequential;                               \
                }                                                       \
                goto text_fallback;                                     \
            }
            E64_TEXT_STORE(kE64TextStore8R, 1, TR(0))
            E64_TEXT_STORE(kE64TextStore8I, 1, TI)
            E64_TEXT_STORE(kE64TextStore16R, 2, TR(0))
            E64_TEXT_STORE(kE64TextStore16I, 2, TI)
            E64_TEXT_STORE(kE64TextStore32R, 4, TR(0))
            E64_TEXT_STORE(kE64TextStore32I, 4, TI)
            E64_TEXT_STORE(kE64TextStore64R, 8, TR(0))
            E64_TEXT_STORE(kE64TextStore64I, 8, TI)
#undef E64_TEXT_STORE

            case kE64TextCall:
            {
                Boolean touched;
                if(likely(emex64_hot_call(core, data_context, pc + text->ilen, &touched)))
                {
                    goto text_branched;
                }
                goto text_fallback;
            }
            case kE64TextCallArgs:
            {
                UInt64 arg[EMEX64_CALL_ARGS];
                Boolean touched;
                for(UInt8 i = 0; i < text->argc; i++)
                {
                    const emex64_ktext_arg_t *a = &text->args[i];
                    arg[i] = a->kind == kE64TextArgImm ? a->imm : *(UInt64 *)((UInt8 *)core + a->reg) + a->imm;
                }
                if(likely(emex64_hot_call(core, data_context, pc + text->ilen, &touched)))
                {
                    for(UInt8 i = 0; i < text->argc; i++)
                    {
                        core->rl[kE64RegisterR0 + i] = arg[i];
                    }
                    goto text_branched;
                }
                goto text_fallback;
            }
            case kE64TextCallIndirect:
            {
                UInt64 to = TR(1);
                Boolean touched;
                if(likely(emex64_hot_call(core, data_context, pc + text->ilen, &touched)))
                {
                    core->rl[kE64RegisterPC] = to;
                    goto text_jumped;
                }
                goto text_fallback;
            }
            case kE64TextReturn:
            case kE64TextReturnAuth:
            {
                UInt64 frame = core->rl[kE64RegisterFP];
                UInt64 fp, to;
                if(likely(emex64_hot_frame(core, data_context, frame, &fp, &to)) &&
                   (text->fast == kE64TextReturn || likely(emex64_fast_reta(core, frame, to, &to))))
                {
                    core->rl[kE64RegisterSP] = frame + 16;
                    core->rl[kE64RegisterFP] = fp;
                    core->rl[kE64RegisterPC] = to;
                    goto text_jumped;
                }
                goto text_fallback;
            }
            case kE64TextSignReturn:
            {
                Boolean touched;
                if(likely(data_context != 0) && likely(emex64_fast_pacret(core, &touched)))
                {
                    if(unlikely(touched))
                    {
                        goto text_stored;
                    }
                    goto text_sequential;
                }
                goto text_fallback;
            }
            default:
                goto text_fallback;
        }
#undef TR
#undef TI

    text_sequential:
        pc += text->ilen;
        fetch_pa += text->ilen;
        core->rl[kE64RegisterPC] = pc;
        text_next = __atomic_load_n(&text->seq, __ATOMIC_RELAXED);
        if(unlikely(text_next == NULL) && (fetch_pa & EMEX64_PAGE_MASK) != 0)
        {
            text_next = emex64_ktext_find(text_store, fetch_pa);
            if(text_next != NULL)
            {
                __atomic_store_n(&text->seq, text_next, __ATOMIC_RELAXED);
            }
        }
        if(ran >= budget)
        {
            break;
        }
        continue;
    text_stored:
        pc += text->ilen;
        fetch_pa += text->ilen;
        core->rl[kE64RegisterPC] = pc;
        text_next = NULL;
        if(ran >= budget || unlikely(__atomic_load_n(&core->request, __ATOMIC_RELAXED) != 0))
        {
            break;
        }
        continue;
    text_branched:
        pc += (UInt64)(SInt64)text->rel;
        fetch_pa += (UInt64)(SInt64)text->rel;
        core->rl[kE64RegisterPC] = pc;
        text_next = NULL;
        {
            UInt64 at;
            if(likely(__E64CoreTextFetch(core, data_context, pc, &at)) && likely(at == fetch_pa))
            {
                text_next = __atomic_load_n(&text->target, __ATOMIC_RELAXED);
                if(unlikely(text_next == NULL))
                {
                    text_next = emex64_ktext_find(text_store, fetch_pa);
                    if(text_next != NULL)
                    {
                        __atomic_store_n(&text->target, text_next, __ATOMIC_RELAXED);
                    }
                }
            }
        }
        if(ran >= budget || unlikely(__atomic_load_n(&core->request, __ATOMIC_RELAXED) != 0))
        {
            break;
        }
        continue;
    text_jumped:
        text_next = NULL;
        if(ran >= budget || unlikely(__atomic_load_n(&core->request, __ATOMIC_RELAXED) != 0))
        {
            break;
        }
        continue;
    text_fallback:
        text_next = NULL;
        text_skip = true;
        ran--;
        continue;
    }

    return ran;
}

static void __E64CoreParkForReset(__E64Core core)
{
    __E64Machine machine = (__E64Machine)core->machine;
    pthread_mutex_lock(&machine->resetLock);
    UInt64 generation = machine->resetGeneration;
    machine->resetParked++;
    pthread_cond_broadcast(&machine->resetCond);
    if(core->id == 0)
    {
        while(machine->resetParked < machine->coreCount && !machine->poweredOff)
        {
            pthread_cond_wait(&machine->resetCond, &machine->resetLock);
        }
        if(!machine->poweredOff)
        {
            __E64MachineResetHardware(machine);
        }
        machine->resetParked = 0;
        machine->resetting = false;
        machine->resetGeneration++;
        pthread_cond_broadcast(&machine->resetCond);
    }
    else
    {
        while(machine->resetGeneration == generation && !machine->poweredOff)
        {
            pthread_cond_wait(&machine->resetCond, &machine->resetLock);
        }
    }
    pthread_mutex_unlock(&machine->resetLock);
}

static void *__E64CoreExecutionThread(void *arg)
{
    assert(arg != NULL);

    /* execution loop */
    __E64Core core = arg;
    __E64Machine machine = (__E64Machine)core->machine;
    UInt32 tick_countdown = EMEX64_TIMER_TICK_INTERVAL;

#if defined(__APPLE__)
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif /* __APPLE__ */
    for(;;)
    {
        if(unlikely(machine->poweredOff || machine->resetting))
        {
            if(machine->poweredOff)
            {
                break;
            }
            __E64CoreParkForReset(core);
            tick_countdown = EMEX64_TIMER_TICK_INTERVAL;
            continue;
        }
        if(unlikely(__atomic_load_n(&core->request, __ATOMIC_RELAXED) != 0) ||
           unlikely(__atomic_load_n(&core->run_state, __ATOMIC_RELAXED) != E64_CORE_RUNNING))
        {
            if(!__E64CoreHandleRequests(core))
            {
                break;
            }
            tick_countdown = EMEX64_TIMER_TICK_INTERVAL;
            continue;
        }

        if(core->cr_state.crstat & E64_CRSTAT_TRAPS)
        {
            if(unlikely(core->cr_state.crstat & E64_CRSTAT_STEP) && core->cr_state.crel.level == kE64ElevationLevelUser && !core->halted)
            {
                tick_countdown -= __E64CoreRunTrapModel(core, 1);
                if(core->cr_state.crel.level == kE64ElevationLevelUser && !core->halted)
                {
                    __E64CoreTrapEnter(core, kE64ExceptionStep, core->rl[kE64RegisterPC], 0);
                }
            }
            else
            {
                tick_countdown -= __E64CoreRunTrapModel(core, tick_countdown);
            }

            if(unlikely(tick_countdown == 0) || unlikely(core->halted))
            {
                tick_countdown = EMEX64_TIMER_TICK_INTERVAL;
                __E64CoreTickTimers(core);
                core->irq_poll = true;
            }
            continue;
        }

        core->trap.exception_at_start = core->cr_state.crexc.exception;
        __E64CoreExecuteInstructionAtPC(core);

        if(core->id == 0)
        {
            __E64ServeInterruptIfNeeded(core->machine->interruptController, core);
        }

        /*
         * currently exceptions happening in a interrupt
         * are ignored, this shall not be the case long
         * term.
         */
        if(!core->in_interrupt)
        {
            /* handling exception if applicable */
            if(unlikely(core->cr_state.crexc.exception != kE64ExceptionNone))
            {
                core->halted = true;
                E64ICRaiseInterrupt(core->machine->interruptController, EMEX64_IRQ_EXCEPTION);
            }
            else if(unlikely(core->halted))
            {
                __E64CoreIdle(core);
            }
        }

        if(unlikely(--tick_countdown == 0) || unlikely(core->halted))
        {
            tick_countdown = EMEX64_TIMER_TICK_INTERVAL;
            __E64CoreTickTimers(core);
        }
    }

    return NULL;
}


E64Exception E64CoreExecute(E64CoreRef coreRef)
{
    __E64Core core = (__E64Core)coreRef;
    assert(core != NULL && core->pthread == 0);
    __E64Machine machine = (__E64Machine)core->machine;

    for(UInt8 i = 0; i < machine->coreCount; i++)
    {
        __E64Core c = (__E64Core)machine->cores[i];
        pthread_create(&(c->pthread), NULL, __E64CoreExecutionThread, (void*)c);
    }

    #if EMEX64VM_DISPLAY_WINDOW && defined(__APPLE__)
    if(!machine->embedded)
    {
        CFRunLoopRun();
    }
    #endif /* EMEX64VM_DISPLAY_WINDOW && __APPLE__ */

    pthread_join(core->pthread, NULL);

    machine->poweredOff = true;
    for(UInt8 i = 0; i < machine->coreCount; i++)
    {
        __E64Core c = (__E64Core)machine->cores[i];
        if(c != core)
        {
            __E64CoreWake(c);
            pthread_join(c->pthread, NULL);
        }
    }

    return core->cr_state.crexc.exception;
}

void E64CoreTerminate(E64CoreRef coreRef)
{
    __E64Core core = (__E64Core)coreRef;
    assert(core != NULL);

    #if EMEX64VM_DISPLAY_WINDOW && defined(__APPLE__)
    if(!((__E64Machine)core->machine)->embedded)
    {
        CFRunLoopStop(CFRunLoopGetMain());
        if(((emex64_display_t*)(core->machine->display))->enabled)
        {
            exit(0);
        }
    }
    #endif /* EMEX64VM_DISPLAY_WINDOW && __APPLE__ */

    __E64Machine machine = (__E64Machine)core->machine;
    machine->poweredOff = true;
    for(UInt8 i = 0; i < machine->coreCount; i++)
    {
        __E64CoreWake((__E64Core)machine->cores[i]);
    }
    pthread_mutex_lock(&machine->resetLock);
    pthread_cond_broadcast(&machine->resetCond);
    pthread_mutex_unlock(&machine->resetLock);
}

UInt64 E64CoreGetValueFromRegister(E64CoreRef coreRef,
                                   E64Register reg)
{
    __E64Core core = (__E64Core)coreRef;
    return (core != NULL && reg <= kE64RegisterMAX) ? core->rl[reg] : 0;
}

void E64CoreSetRegisterWithValue(E64CoreRef coreRef,
                                 E64Register reg,
                                 UInt64 value)
{
    __E64Core core = (__E64Core)coreRef;
    if(core == NULL || reg > kE64RegisterMAX)
    {
        return;
    }
    core->rl[reg] = value;
}

E64Exception E64CoreGetException(E64CoreRef coreRef)
{
    __E64Core core = (__E64Core)coreRef;
    return core != NULL ? core->cr_state.crexc.exception : kE64ExceptionNone;
}
