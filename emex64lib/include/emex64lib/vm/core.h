/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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

#ifndef EMEX64VM_CORE_H
#define EMEX64VM_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

enum kEmex64Opcode: uint8_t {
    /* core operations */
    kEmex64OpcodeHLT =      0b00000000,
    kEmex64OpcodeNOP =      0b00000001,

    /* data operations */
    kEmex64OpcodeMOV =      0b00000010,
    kEmex64OpcodeSWP =      0b00000011,
    kEmex64OpcodeSWPZ =     0b00000100,
    kEmex64OpcodePUSH =     0b00000101,
    kEmex64OpcodePOP =      0b00000110,
    kEmex64OpcodeLDB =      0b00000111,
    kEmex64OpcodeLDW =      0b00001000,
    kEmex64OpcodeLDD =      0b00001001,
    kEmex64OpcodeLDQ =      0b00001010,
    kEmex64OpcodeSTB =      0b00001011,
    kEmex64OpcodeSTW =      0b00001100,
    kEmex64OpcodeSTD =      0b00001101,
    kEmex64OpcodeSTQ =      0b00001110,

    /* alu operations */
    kEmex64OpcodeADD =      0b00001111,
    kEmex64OpcodeSUB =      0b00010000,
    kEmex64OpcodeMUL =      0b00010001,
    kEmex64OpcodeDIV =      0b00010010,
    kEmex64OpcodeIDIV =     0b00010011,
    kEmex64OpcodeMOD =      0b00010100,
    kEmex64OpcodeNOT =      0b00010101,
    kEmex64OpcodeNEG =      0b00010110,
    kEmex64OpcodeAND =      0b00010111,
    kEmex64OpcodeOR  =      0b00011000,
    kEmex64OpcodeXOR =      0b00011001,
    kEmex64OpcodeSHR =      0b00011010,
    kEmex64OpcodeSHL =      0b00011011,
    kEmex64OpcodeSAR =      0b00011100,
    kEmex64OpcodeROR =      0b00011101,
    kEmex64OpcodeROL =      0b00011110,
    kEmex64OpcodePDEP =     0b00011111,
    kEmex64OpcodePEXT =     0b00100000,
    kEmex64OpcodeBSWAPW =   0b00100001,
    kEmex64OpcodeBSWAPD =   0b00100010,
    kEmex64OpcodeBSWAPQ =   0b00100011,

    /* control flow operations */
    kEmex64OpcodeB =        0b00100100,
    kEmex64OpcodeCMP =      0b00100101,
    kEmex64OpcodeBE =       0b00100110,
    kEmex64OpcodeBNE =      0b00100111,
    kEmex64OpcodeBLT =      0b00101000,
    kEmex64OpcodeBGT =      0b00101001,
    kEmex64OpcodeBLE =      0b00101010,
    kEmex64OpcodeBGE =      0b00101011,
    kEmex64OpcodeBZ =       0b00101100,
    kEmex64OpcodeBNZ =      0b00101101,
    kEmex64OpcodeBL =       0b00101110,
    kEmex64OpcodeRET =      0b00101111,
    kEmex64OpcodeIRET =     0b00110000,

    kEmex64OpcodeMAX = kEmex64OpcodeIRET,
};

enum kEmex64ParameterCoding: uint8_t {
    kEmex64ParameterCodingEnd   = 0b000,
    kEmex64ParameterCodingReg   = 0b001,
    kEmex64ParameterCodingImm8  = 0b010,
    kEmex64ParameterCodingImm16 = 0b011,
    kEmex64ParameterCodingImm32 = 0b100,
    kEmex64ParameterCodingImm64 = 0b101
    /* leaving 0b110 and 0b111 open for later additions */
};

enum kEmex64Register: uint8_t {
    /*
     * program counter: it points to the current address at
     * which the CPU currently is, it increments by the
     * lenght of the instruction when the CPU is done
     * executing the instruction at which PC points to at
     * that time.
     */
    kEmex64RegisterPC =     0b00000,

    /*
     * stack pointer: it points to the current address at
     * which the stack lives, stack grows downwards on
     * allocation and upwards on deallocation.
     *
     * stack allocation is meant to be done for small
     * things, as heap allocation is way more expensive,
     * than a simple register decrement.
     */
    kEmex64RegisterSP =     0b00001,

    /*
     * frame pointer: it points to the address at which the
     * stack frame of the last function call lives,
     * basically empowering you to branch and link and
     * return back without destroying values stored
     * in registers previously.
     *
     * a stack frame on the LA64 architecture is a full
     * backup of all registers stored onto stack memory
     * that is expensive(256 bytes per frame) but its also
     * simplistic, for now that will be the soulution,
     * cannot be guranteed that this ABI choice wont change.
     */
    kEmex64RegisterFP =     0b00010,

    /*
     * control flag: used by control flow instructions like
     * cmp, je, jne.. basically used for if else statements.
     */
    kEmex64RegisterCF =     0b00011,

    /*
     * general purpose registers: these registers arent used
     * for anything other than the software, these registers
     * can be used for any purpose, thats why they are called
     * general purpose registers, because they got no fixed
     * purpose like pc, sp, fp, cf.
     */
    kEmex64RegisterR0 =     0b00100,
    kEmex64RegisterR1 =     0b00101,
    kEmex64RegisterR2 =     0b00110,
    kEmex64RegisterR3 =     0b00111,
    kEmex64RegisterR4 =     0b01000,
    kEmex64RegisterR5 =     0b01001,
    kEmex64RegisterR6 =     0b01010,
    kEmex64RegisterR7 =     0b01011,
    kEmex64RegisterR8 =     0b01100,
    kEmex64RegisterR9 =     0b01101,
    kEmex64RegisterR10 =    0b01110,
    kEmex64RegisterR11 =    0b01111,
    kEmex64RegisterR12 =    0b10000,
    kEmex64RegisterR13 =    0b10001,
    kEmex64RegisterR14 =    0b10010,
    kEmex64RegisterR15 =    0b10011,
    kEmex64RegisterR16 =    0b10100,

    /*
     * return register: also a general purpose register but
     * it is not affected by bl and ret, this register
     * has the purpose of a called symbol to be able to
     * return without any crazy memory math a value for
     * example.
     */
    kEmex64RegisterRR =     0b10101,

    /* control registers */
    kEmex64RegisterCR0 =    0b10110,    /* CREL:    elevation control register */
    kEmex64RegisterCR1 =    0b10111,    /* CRKSP:   kernel stack pointer (the stack pointer the interrupt controller will use when receiving interrupt) */
    kEmex64RegisterCR2 =    0b11000,    /* CREXC:   exception register (first 3bits for the exception) */
    kEmex64RegisterCR3 =    0b11001,    /* CRVEC:   cpu vector table */
    kEmex64RegisterCR4 =    0b11010,    /* CRPTB:   page table pointer (first 8bits are the flags and the rest is the physical address where the page table is) */
    kEmex64RegisterCR5 =    0b11011,
    kEmex64RegisterCR6 =    0b11100,
    kEmex64RegisterCR7 =    0b11101,
    kEmex64RegisterCR8 =    0b11110,
    kEmex64RegisterCR9 =    0b11111,

    kEmex64RegisterMAX = kEmex64RegisterCR9
};

enum kEmex64ElevationLevel: uint8_t {
    kEmex64ElevationLevelUser =             0b00,
    kEmex64ElevationLevelKernel =           0b01,
    kEmex64ElevationLevelSecureMonitor =    0b10    /* used for software kernel secure mechanism like the apples PPL */
};

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
enum kEmex64CompareFlag: uint8_t {

    kEmex64CompareFlagZ =   0x1,
    kEmex64CompareFlagL =   0x2,
    kEmex64CompareFlagG =   0x4
};

enum kEmex64Exception {
    /*
     * normal state, simply a marker to say nothing
     * to trigger a interrupt for.
     */
    kEmex64ExceptionNone =              0b000,

    /*
     * this exception means that a memory address was
     * accessed inappropriately, which means memory
     * if the cpu writes to memory that it doesnt have
     * access to this exception is triggered.
     */
    kEmex64ExceptionBadAccess =         0b001,

    /*
     * this exception means that the current cpu state
     * did not have the appropriate permissions to
     * access a certain register for example.
     */
    kEmex64ExceptionPermission =        0b010,

    /*
     * this exception means that the cpu regocnised a
     * instruction that was not valid was being tried
     * to decode.
     */
    kEmex64ExceptionBadInstruction =    0b011,

    /*
     * the alu tried to perform illegal math operations
     * like for example N / 0 or N % 0.
     */
    kEmex64ExceptionBadArithmetic =     0b100,
};

typedef struct la64_core la64_core_t;

/* definition of the handler of each operation */
typedef void (*la64_opfunc_t)(la64_core_t *core);

typedef struct la64_machine la64_machine_t;

typedef struct emex64_opfunc_entry {
    la64_opfunc_t func;
    uint8_t maxargs;
} emex64_opfunc_entry_t;

typedef struct la64_core {

    /* the pthread this core is running on on the host */
    pthread_t pthread;

    /* a array of all (control) registers */
    uint64_t rl[kEmex64RegisterMAX + 1];

    /* data of currently decoding or decoded operation */
    struct la64_operation {

        /*
         * lenght of decoded instruction so that the cpu
         * can correctly increment the program counter.
         */
        uint8_t ilen;

        /*
         * the opcode it self, so the cpu knows what to
         * execute.
         */
        enum kEmex64Opcode opcode;
        emex64_opfunc_entry_t op;

        /*
         * a array of intermediate values that can go from
         * 8 to 64 bits in width, the cpu stuffs that cache
         * on time of decoding.
         */
        uint64_t imm[32];

        /* count of parameters */
        uint8_t param_cnt;

        /*
         * pointer array for parameters, at emulation we
         * dont have many emulation options so we stuff
         * each parameter into this array.. register
         * intermediate, etc.
         */
        uint64_t *param[32];
    } op;

    /*
     * cpu halting status (will later be in the same
     * control register as the exception register CR0).
     */
    bool halted;

    /*
     * in-case a interrupt fired before the cpu
     * intentionally executed hlt then it shall
     * immediately wake from the halt.
     */
    bool unhalted_interrupt;

    /*
     * cpu cant get a second interrupt while handling
     * one, but will be unset when the cpu calls iret.
     */
    bool in_interrupt;

    /* pointer back to machine */
    la64_machine_t *machine;

} la64_core_t;

la64_core_t *la64_core_alloc(void);
void la64_core_dealloc(la64_core_t *core);
void la64_core_execute(la64_core_t *core);
void la64_core_terminate(la64_core_t *core);

#endif /* EMEX64VM_CORE_H */
