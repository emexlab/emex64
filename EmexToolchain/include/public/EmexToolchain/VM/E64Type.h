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

    /* PSE - pointer self-entropy (implementation pending) */
    kE64OpcodePSEAL =   0b00111100,
    kE64OpcodePSEAUTH = 0b00111101,
    kE64OpcodePSEDET =  0b00111110,
    kE64OpcodePSECTX =  0b00111111,
    kE64OpcodePSESEM =  0b01000000,
    kE64OpcodePSECAP =  0b01000001,
    kE64OpcodePSESYNC = 0b01000010,

    /*
     * for floating point later (ideas atleast):
     *
     * data operations:
     * fmov
     * fswp
     * fswpz
     *
     * floating point arithmetic:
     * fadd
     * fsub
     * fmul
     * fdiv
     * ... (what not)
     */

    kE64OpcodeMAX = kE64OpcodeRDRND,

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
    kE64ControlRegisterCR0 = 0b0000,    /* CREL:    elevation control register */
    kE64ControlRegisterCR1 = 0b0001,    /* CRKSP:   kernel stack pointer (the stack pointer the interrupt controller will use when receiving interrupt) */
    kE64ControlRegisterCR2 = 0b0010,    /* CREXC:   exception register (first 3bits for the exception) */
    kE64ControlRegisterCR3 = 0b0011,    /* CRVEC:   cpu vector table */
    kE64ControlRegisterCR4 = 0b0100,    /* CRPTB:   page table pointer (first 8bits are the flags and the rest is the physical address where the page table is) */
    kE64ControlRegisterCR5 = 0b0101,    /* CRFPC:   kernel only floating point control register */
    kE64ControlRegisterMAX = kE64ControlRegisterCR5
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
    kE64CompareFlagG =   0x4
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
} E64Exception;

#endif /* E64TYPE_H */
