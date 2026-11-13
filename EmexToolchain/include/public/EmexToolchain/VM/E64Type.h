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

#ifndef E64TYPE_H
#define E64TYPE_H

#include <EmexFoundation/EmexFoundation.h>

typedef enum: UInt8 {
    /* core operations */
    kE64OpcodeHLT =     0b00000000,
    kE64OpcodeNOP =     0b00000001,

    /* data operations */
    kE64OpcodeMOV =     0b00000010,
    kE64OpcodeMOVZ =    0b00000011,
    kE64OpcodeSWP =     0b00000100,
    kE64OpcodePUSH =    0b00000101,
    kE64OpcodePOP =     0b00000110,
    kE64OpcodeLDB =     0b00000111,
    kE64OpcodeLDW =     0b00001000,
    kE64OpcodeLDD =     0b00001001,
    kE64OpcodeLDQ =     0b00001010,
    kE64OpcodeSTB =     0b00001011,
    kE64OpcodeSTW =     0b00001100,
    kE64OpcodeSTD =     0b00001101,
    kE64OpcodeSTQ =     0b00001110,

    /* alu operations */
    kE64OpcodeADD =     0b00001111,
    kE64OpcodeSUB =     0b00010000,
    kE64OpcodeMUL =     0b00010001,
    kE64OpcodeDIV =     0b00010010,
    kE64OpcodeIDIV =    0b00010011,
    kE64OpcodeMOD =     0b00010100,
    kE64OpcodeNOT =     0b00010101,
    kE64OpcodeNEG =     0b00010110,
    kE64OpcodeAND =     0b00010111,
    kE64OpcodeOR  =     0b00011000,
    kE64OpcodeXOR =     0b00011001,
    kE64OpcodeSHR =     0b00011010,
    kE64OpcodeSHL =     0b00011011,
    kE64OpcodeSAR =     0b00011100,
    kE64OpcodeROR =     0b00011101,
    kE64OpcodeROL =     0b00011110,
    kE64OpcodePDEP =    0b00011111,
    kE64OpcodePEXT =    0b00100000,
    kE64OpcodeBSWAPW =  0b00100001,
    kE64OpcodeBSWAPD =  0b00100010,
    kE64OpcodeBSWAPQ =  0b00100011,
    kE64OpcodeINC =     0b00100100,
    kE64OpcodeDEC =     0b00100101,

    /* control flow operations */
    kE64OpcodeB =       0b00100110,
    kE64OpcodeCMP =     0b00100111,
    kE64OpcodeBE =      0b00101000,
    kE64OpcodeBNE =     0b00101001,
    kE64OpcodeBLT =     0b00101010,
    kE64OpcodeBGT =     0b00101011,
    kE64OpcodeBLE =     0b00101100,
    kE64OpcodeBGE =     0b00101101,
    kE64OpcodeBZ =      0b00101110,
    kE64OpcodeBNZ =     0b00101111,
    kE64OpcodeBLW =     0b00110000,
    kE64OpcodeWRET =    0b00110001,
    kE64OpcodeIRET =    0b00110010,
    kE64OpcodeBL =      0b00110011,
    kE64OpcodeRET =     0b00110100,

    /* data operations v2 */
    kE64OpcodeCLR =     0b00110101,
    kE64OpcodeCMOV =    0b00110110,
    kE64OpcodeCMOVB =   0b00110111,
    kE64OpcodeCLAR =    0b00111000,

    /* control flow operation v2 */
    kE64OpcodeBBZ =     0b00111001,
    kE64OpcodeBBNZ =    0b00111010,

    /* data operations v3 */
    kE64OpcodeRDRND =   0b00111011,
    kE64OpcodeMOVM =    0b00111100,
    kE64OpcodeCPYM =    0b00111101,
    kE64OpcodeSETM =    0b00111110,

    /* control flow operations v3 */
    kE64OpcodeSVC =     0b00111111,

    /* ISA v17 (trap model and atomics) */
    kE64OpcodeERET =    0b01000000,
    kE64OpcodeCSWP =    0b01000001,
    kE64OpcodeXCHG =    0b01000010,
    kE64OpcodeCAS =     0b01000011,

    /* ISA v18 (floating point unit) */
    kE64OpcodeFADD =    0b01000100,
    kE64OpcodeFSUB =    0b01000101,
    kE64OpcodeFMUL =    0b01000110,
    kE64OpcodeFDIV =    0b01000111,
    kE64OpcodeFSQRT =   0b01001000,
    kE64OpcodeFMIN =    0b01001001,
    kE64OpcodeFMAX =    0b01001010,
    kE64OpcodeFMADD =   0b01001011,
    kE64OpcodeFNEG =    0b01001100,
    kE64OpcodeFABS =    0b01001101,
    kE64OpcodeFCMP =    0b01001110,
    kE64OpcodeFADDS =   0b01001111,
    kE64OpcodeFSUBS =   0b01010000,
    kE64OpcodeFMULS =   0b01010001,
    kE64OpcodeFDIVS =   0b01010010,
    kE64OpcodeFSQRTS =  0b01010011,
    kE64OpcodeFMINS =   0b01010100,
    kE64OpcodeFMAXS =   0b01010101,
    kE64OpcodeFMADDS =  0b01010110,
    kE64OpcodeFNEGS =   0b01010111,
    kE64OpcodeFABSS =   0b01011000,
    kE64OpcodeFCMPS =   0b01011001,
    kE64OpcodeFCVTDS =  0b01011010,
    kE64OpcodeFCVTSD =  0b01011011,
    kE64OpcodeFCVTDI =  0b01011100,
    kE64OpcodeFCVTDU =  0b01011101,
    kE64OpcodeFCVTID =  0b01011110,
    kE64OpcodeFCVTUD =  0b01011111,
    kE64OpcodeFCVTSI =  0b01100000,
    kE64OpcodeFCVTSU =  0b01100001,
    kE64OpcodeFCVTIS =  0b01100010,
    kE64OpcodeFCVTUS =  0b01100011,

    /* ISA v19 (binary size reduction) */
    kE64OpcodeSEXT =    0b01100100,
    kE64OpcodeCBLT =    0b01100101,
    kE64OpcodeCLZ =     0b01100110,
    kE64OpcodeCTZ =     0b01100111,
    kE64OpcodePOPCNT =  0b01101000,
    kE64OpcodeMULH =    0b01101001,
    kE64OpcodeUMULH =   0b01101010,
    kE64OpcodeADC =     0b01101011,
    kE64OpcodeSBB =     0b01101100,

    /* ISA v19 (PAC extension) */
    kE64OpcodePACIA =   0b01101101,
    kE64OpcodeAUTIA =   0b01101110,
    kE64OpcodePACDA =   0b01101111,
    kE64OpcodeAUTDA =   0b01110000,
    kE64OpcodeXPAC =    0b01110001,
    kE64OpcodePACRET =  0b01110010,
    kE64OpcodeRETA =    0b01110011,

    /* ISA v20 (performance update) */
    kE64OpcodeRDTIME =  0b01110100,
    kE64OpcodeWFE =     0b01110101,
    kE64OpcodeSEV =     0b01110110,
    kE64OpcodePAUSE =   0b01110111,
    kE64OpcodeCBEQ =    0b01111000,
    kE64OpcodeCBNE =    0b01111001,
    kE64OpcodeCBGE =    0b01111010,
    kE64OpcodeCBLTU =   0b01111011,
    kE64OpcodeCBGEU =   0b01111100,
    kE64OpcodeLDSB =    0b01111101,
    kE64OpcodeLDSW =    0b01111110,
    kE64OpcodeLDSD =    0b01111111,
    kE64OpcodeLDADD =   0b10000000,
    kE64OpcodeLDAND =   0b10000001,
    kE64OpcodeLDOR =    0b10000010,
    kE64OpcodeLDXOR =   0b10000011,
    kE64OpcodeSWPADD =  0b10000100,
    kE64OpcodeLDADDD =  0b10000101,
    kE64OpcodeLDANDD =  0b10000110,
    kE64OpcodeLDORD =   0b10000111,
    kE64OpcodeLDXORD =  0b10001000,
    kE64OpcodeSWPADDD = 0b10001001,
    kE64OpcodeTLBI =    0b10001010,

    /* ISA v21 (32bit arithmetic update) */
    kE64OpcodeADDD =    0b10001011,
    kE64OpcodeIADDD =   0b10001100,
    kE64OpcodeSUBD =    0b10001101,
    kE64OpcodeISUBD =   0b10001110,
    kE64OpcodeMULD =    0b10001111,
    kE64OpcodeIMULD =   0b10010000,
    kE64OpcodeSHLD =    0b10010001,
    kE64OpcodeISHLD =   0b10010010,
    kE64OpcodeSHRD =    0b10010011,
    kE64OpcodeSARD =    0b10010100,
    kE64OpcodeDIVD =    0b10010101,
    kE64OpcodeIDIVD =   0b10010110,
    kE64OpcodeMODD =    0b10010111,

    kE64OpcodeMAX = kE64OpcodeMODD,

    kE64OpcodeInvalid =  0b11111111,
} E64Opcode;

typedef enum: UInt8 {
    /*
     * defines the end of a instruction in case the
     * instruction can have such a end coding, like
     * dynamic instructions.
     */
    kE64ParameterCodingEnd =            0b0000,

    kE64ParameterCodingReg =            0b0001,
    kE64ParameterCodingImm4 =           0b0010,
    kE64ParameterCodingImm8  =          0b0011,
    kE64ParameterCodingImm16 =          0b0100,
    kE64ParameterCodingImm32 =          0b0101,
    kE64ParameterCodingImm64 =          0b0110,

    /*
     * this means the decoder has to first skip until
     * the next boundary before it can safely read the
     * data, this coding has been added for compatibility
     * for dynamic symbol relocation.
     */
    kE64ParameterCodingAddr64 =         0b0111,

    kE64ParameterCodingRegExtended =    0b1000,

    /* parameter codings that do things */
    /*
     * registers with the difference that they get loaded
     * as intermediates and then incrementet.
     */
    kE64ParameterCodingRegImmInc =      0b1001,
    kE64ParameterCodingRegImmDec =      0b1010,

    /* offsetting */
    kE64ParameterCodingOffsetAdd =      0b1011,
    kE64ParameterCodingOffsetSub =      0b1100,

    /* floating point register */
    kE64ParameterCodingRegFloat =       0b1101,
} E64ParameterCoding;

typedef enum: UInt8 {
    /*
     * program counter: it points to the current address at
     * which the CPU currently is, it increments by the
     * lenght of the instruction when the CPU is done
     * executing the instruction at which PC points to at
     * that time.
     */
    kE64RegisterPC =     0b0000,

    /*
     * stack pointer: it points to the current address at
     * which the stack lives, stack grows downwards on
     * allocation and upwards on deallocation.
     *
     * stack allocation is meant to be done for small
     * things, as heap allocation is way more expensive,
     * than a simple register decrement.
     */
    kE64RegisterSP =     0b0001,

    /*
     * frame pointer: it points to the address at which the
     * stack frame of the last function call lives,
     * basically empowering you to branch and link and
     * return back without destroying values stored
     * in registers previously.
     *
     * a stack frame on the EMEX64 architecture is a full
     * backup of all registers stored onto stack memory
     * that is expensive(256 bytes per frame) but its also
     * simplistic, for now that will be the soulution,
     * cannot be guranteed that this ABI choice wont change.
     */
    kE64RegisterFP =     0b0010,

    /*
     * control flag: used by control flow instructions like
     * cmp, je, jne.. basically used for if else statements.
     */
    kE64RegisterCF =     0b0011,

    /*
     * floating point control register: can be used in
     * userspace.
     */
    kE64RegisterFPC =    0b0100,

    /*
     * general purpose registers: these registers arent used
     * for anything other than the software, these registers
     * can be used for any purpose, thats why they are called
     * general purpose registers, because they got no fixed
     * purpose like pc, sp, fp, cf.
     */
    kE64RegisterR0 =     0b0101,
    kE64RegisterR1 =     0b0110,
    kE64RegisterR2 =     0b0111,
    kE64RegisterR3 =     0b1000,
    kE64RegisterR4 =     0b1001,
    kE64RegisterR5 =     0b1010,
    kE64RegisterR6 =     0b1011,
    kE64RegisterR7 =     0b1100,
    kE64RegisterR8 =     0b1101,
    kE64RegisterR9 =     0b1110,

    /*
     * return register: also a general purpose register but
     * it is not affected by bl and ret, this register
     * has the purpose of a called symbol to be able to
     * return without any crazy memory math a value for
     * example.
     */
    kE64RegisterRR =     0b1111,

    kE64RegisterMAX = kE64RegisterRR,

    kE64RegisterInvalid =    0b11111111,
} E64Register;

typedef enum: UInt8 {
    kE64RegisterExtendedER0 =    0b0000,
    kE64RegisterExtendedER1 =    0b0001,
    kE64RegisterExtendedER2 =    0b0010,
    kE64RegisterExtendedER3 =    0b0011,
    kE64RegisterExtendedER4 =    0b0100,
    kE64RegisterExtendedER5 =    0b0101,
    kE64RegisterExtendedER6 =    0b0110,
    kE64RegisterExtendedER7 =    0b0111,
    kE64RegisterExtendedER8 =    0b1000,
    kE64RegisterExtendedER9 =    0b1001,
    kE64RegisterExtendedER10 =   0b1010,
    kE64RegisterExtendedER11 =   0b1011,
    kE64RegisterExtendedER12 =   0b1100,
    kE64RegisterExtendedER13 =   0b1101,
    kE64RegisterExtendedER14 =   0b1110,
    kE64RegisterExtendedER15 =   0b1111,

    kE64RegisterExtendedMAX = kE64RegisterExtendedER15,

    kE64RegisterExtendedInvalid =   0b11111111,
} E64RegisterExtended;

typedef enum: UInt8 {
    kE64ControlRegisterCREL =   0b0000,                     /* CREL:    elevation control register */
    kE64ControlRegisterCRKSP =  0b0001,                     /* CRKSP:   kernel stack pointer (the stack pointer the interrupt controller will use when receiving interrupt) */
    kE64ControlRegisterCREXC =  0b0010,                     /* CREXC:   exception register (first 3bits for the exception) */
    kE64ControlRegisterCRVEC =  0b0011,                     /* CRVEC:   cpu vector table */
    kE64ControlRegisterCRPTB =  0b0100,                     /* CRPTB:   page table pointer (first 8bits are the flags and the rest is the physical address where the page table is) */
    kE64ControlRegisterCRFPC =  0b0101,                     /* CRFPC:   kernel only floating point control register */
    kE64ControlRegisterCRISA =  0b0110,                     /* CRISA:   instruction set architecture configuration */

    /* trap model */
    kE64ControlRegisterCRSTAT = 0b0111,                     /* CRSTAT:  trap model enable, interrupt enable, state before the last trap */
    kE64ControlRegisterCRTVEC = 0b1000,                     /* CRTVEC:  address of the trap entry */
    kE64ControlRegisterCREPC =  0b1001,                     /* CREPC:   pc to return to */
    kE64ControlRegisterCRCAUSE = 0b1010,                    /* CRCAUSE: exception cause */
    kE64ControlRegisterCRTVAL = 0b1011,                     /* CRTVAL:  faulting address */
    kE64ControlRegisterCRSCRATCH = 0b1100,                  /* CRSCRATCH: free for the kernel */
    kE64ControlRegisterCRIAKEY = 0b1101,                    /* CRIAKEY: the key for instruction pointers (PAC) */
    kE64ControlRegisterCRDAKEY = 0b1110,                    /* CRDAKEY: the key for data pointers (PAC) */
    kE64ControlRegisterCRPAC =  0b1111,                     /* CRPAC:   pointer authentication on per level (PAC) */
    kE64ControlRegisterMAX =    kE64ControlRegisterCRPAC
} E64ControlRegister;

typedef enum: UInt8 {
    kE64FloatingRegisterFR0 = 0b00000,
    kE64FloatingRegisterFR1 = 0b00001,
    kE64FloatingRegisterFR2 = 0b00010,
    kE64FloatingRegisterFR3 = 0b00011,
    kE64FloatingRegisterFR4 = 0b00100,
    kE64FloatingRegisterFR5 = 0b00101,
    kE64FloatingRegisterFR6 = 0b00110,
    kE64FloatingRegisterFR7 = 0b00111,
    kE64FloatingRegisterFR8 = 0b01000,
    kE64FloatingRegisterFR9 = 0b01001,
    kE64FloatingRegisterFR10 = 0b01010,
    kE64FloatingRegisterFR11 = 0b01011,
    kE64FloatingRegisterFR12 = 0b01100,
    kE64FloatingRegisterFR13 = 0b01101,
    kE64FloatingRegisterFR14 = 0b01110,
    kE64FloatingRegisterFR15 = 0b01111,
    kE64FloatingRegisterMAX = kE64FloatingRegisterFR15
} E64FloatingRegister;

typedef enum: UInt8 {
    kE64ElevationLevelUser =             0b00,
    kE64ElevationLevelKernel =           0b01,
    kE64ElevationLevelSecureMonitor =    0b10    /* used for software kernel secure mechanism like apples PPL */
} E64ElevationLevel;

/*
 * these flags is what the CF register contains of, yk we talked
 * about the control flag, those flags here are set
 * by cmp, when you compare two values or registers
 * with eachother, in this case the compare flag
 * gets set to one of the following.
 *
 * Z = EQUAL
 * L = LESS
 * G = GREATER
 *
 */
typedef enum: UInt8 {
    kE64CompareFlagZ =   0x1,
    kE64CompareFlagL =   0x2,
    kE64CompareFlagG =   0x4,
    kE64CompareFlagU =   0x8,
    kE64CompareFlagC =   0x10
} E64CompareFlag;

typedef enum: UInt8 {
    /*
     * normal state, simply a marker to say nothing
     * to trigger a interrupt for.
     */
    kE64ExceptionNone =              0b000,

    /*
     * this exception means that a memory address was
     * accessed inappropriately, which means memory
     * if the cpu writes to memory that it doesnt have
     * access to this exception is triggered.
     */
    kE64ExceptionBadAccess =         0b001,

    /*
     * this exception means that the current cpu state
     * did not have the appropriate permissions to
     * access a certain register for example.
     */
    kE64ExceptionPermission =        0b010,

    /*
     * this exception means that the cpu regocnised a
     * instruction that was not valid was being tried
     * to decode.
     */
    kE64ExceptionBadInstruction =    0b011,

    /*
     * the alu tried to perform illegal math operations
     * like for example N / 0 or N % 0.
     */
    kE64ExceptionBadArithmetic =     0b100,

    /*
     * when the MMU sees a page is dirty and a user program
     * wants to write to it it will cause a page fault or
     * when a page was accessed that is not accessible or
     * not mapped.
     */
    kE64ExceptionPageFault =         0b101,

    /*
     * KTRR exception!?
     */
    kE64ExceptionKTRRViolation =     0b110,

    /*
     * a system call, crepc points behind the svc instruction.
     */
    kE64ExceptionSVC =               0b111,

    /*
     * autia, autda or reta found a pointer whose
     * signature is wrong.
     */
    kE64ExceptionPAC =               0b1000,
} E64Exception;

/* CRSTAT bits */
#define E64_CRSTAT_IE               (1ULL << 0)                     /* interrupts enabled */
#define E64_CRSTAT_PIE              (1ULL << 1)                     /* IE before the last trap */
#define E64_CRSTAT_PEL_SHIFT        8                               /* elevation level before the last trap */
#define E64_CRSTAT_PEL_MASK         (3ULL << E64_CRSTAT_PEL_SHIFT)
#define E64_CRSTAT_TRAPS            (1ULL << 16)                    /* trap model enabled */
#define E64_CRSTAT_WRITABLE         (E64_CRSTAT_IE | E64_CRSTAT_PIE | E64_CRSTAT_PEL_MASK | E64_CRSTAT_TRAPS)

/* FPC bits */
#define E64_FPC_RM_MASK             0x3ULL          /* rounding mode */
#define E64_FPC_RM_NEAREST          0               /* to nearest, ties to even */
#define E64_FPC_RM_ZERO             1               /* toward zero */
#define E64_FPC_RM_DOWN             2               /* toward negative infinity */
#define E64_FPC_RM_UP               3               /* toward positive infinity */
#define E64_FPC_FLAGS_SHIFT         4               /* accrued exception flags */
#define E64_FPC_NV                  (1ULL << 4)     /* invalid operation */
#define E64_FPC_DZ                  (1ULL << 5)     /* division by zero */
#define E64_FPC_OF                  (1ULL << 6)     /* overflow */
#define E64_FPC_UF                  (1ULL << 7)     /* underflow */
#define E64_FPC_NX                  (1ULL << 8)     /* inexact */

/* PAC bits */
#define E64_CRISA_EXT_PAC           (1ULL << 48)    /* CRISA PAC feature */
#define E64_CRPAC_USER              (1ULL << 0)     /* on for user code */
#define E64_CRPAC_KERNEL            (1ULL << 1)     /* on for kernel code */
#define E64_CRPAC_WRITABLE          (E64_CRPAC_USER | E64_CRPAC_KERNEL)
#define E64_PAC_SHIFT               54
#define E64_PAC_MASK                (0x3FFULL << E64_PAC_SHIFT)

/* CRCAUSE */
#define E64_CRCAUSE_INTERRUPT       (1ULL << 63)
#define E64_CRCAUSE_ACCESS_SHIFT    8
#define E64_ACCESS_READ             1
#define E64_ACCESS_WRITE            2
#define E64_ACCESS_EXECUTE          3

#endif /* E64TYPE_H */
