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
    kEmex64OpcodeINC =      0b00100100,
    kEmex64OpcodeDEC =      0b00100101,

    /* control flow operations */
    kEmex64OpcodeB =        0b00100110,
    kEmex64OpcodeCMP =      0b00100111,
    kEmex64OpcodeBE =       0b00101000,
    kEmex64OpcodeBNE =      0b00101001,
    kEmex64OpcodeBLT =      0b00101010,
    kEmex64OpcodeBGT =      0b00101011,
    kEmex64OpcodeBLE =      0b00101100,
    kEmex64OpcodeBGE =      0b00101101,
    kEmex64OpcodeBZ =       0b00101110,
    kEmex64OpcodeBNZ =      0b00101111,
    kEmex64OpcodeBLW =      0b00110000,
    kEmex64OpcodeWRET =     0b00110001,
    kEmex64OpcodeIRET =     0b00110010,
    kEmex64OpcodeBL =       0b00110011,
    kEmex64OpcodeRET =      0b00110100,

    /* data operations v2 */
    kEmex64OpcodeCLR =      0b00110101,
    kEmex64OpcodeCMOV =     0b00110110,
    kEmex64OpcodeCMOVB =    0b00110111,

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

    kEmex64OpcodeMAX = kEmex64OpcodeCMOVB,
};

enum kEmex64ParameterCoding: uint8_t {
    /*
     * defines the end of a instruction in case the
     * instruction can have such a end coding, like
     * dynamic instructions.
     */
    kEmex64ParameterCodingEnd =     0b000,

    kEmex64ParameterCodingReg =     0b001,
    kEmex64ParameterCodingImm5 =    0b010,
    kEmex64ParameterCodingImm8  =   0b011,
    kEmex64ParameterCodingImm16 =   0b100,
    kEmex64ParameterCodingImm32 =   0b101,
    kEmex64ParameterCodingImm64 =   0b110,

    /*
     * this means the decoder has to first skip until
     * the next boundary before it can safely read the
     * data, this coding has been added for compatibility
     * for dynamic symbol relocation.
     */
    kEmex64ParameterCodingAddr64 =  0b111
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
     * a stack frame on the EMEX64 architecture is a full
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
     * floating point control register: can be used in
     * userspace.
     */
    kEmex64RegisterFPC =    0b00100,

    /*
     * general purpose registers: these registers arent used
     * for anything other than the software, these registers
     * can be used for any purpose, thats why they are called
     * general purpose registers, because they got no fixed
     * purpose like pc, sp, fp, cf.
     */
    kEmex64RegisterR0 =     0b00101,
    kEmex64RegisterR1 =     0b00110,
    kEmex64RegisterR2 =     0b00111,
    kEmex64RegisterR3 =     0b01000,
    kEmex64RegisterR4 =     0b01001,
    kEmex64RegisterR5 =     0b01010,
    kEmex64RegisterR6 =     0b01011,
    kEmex64RegisterR7 =     0b01100,
    kEmex64RegisterR8 =     0b01101,
    kEmex64RegisterR9 =     0b01110,
    kEmex64RegisterR10 =    0b01111,
    kEmex64RegisterR11 =    0b10000,
    kEmex64RegisterR12 =    0b10001,
    kEmex64RegisterR13 =    0b10010,
    kEmex64RegisterR14 =    0b10011,
    kEmex64RegisterR15 =    0b10100,
    kEmex64RegisterR16 =    0b10101,
    kEmex64RegisterR17 =    0b10110,
    kEmex64RegisterR18 =    0b10111,
    kEmex64RegisterR19 =    0b11000,
    kEmex64RegisterR20 =    0b11001,
    kEmex64RegisterR21 =    0b11010,
    kEmex64RegisterR22 =    0b11011,
    kEmex64RegisterR23 =    0b11100,
    kEmex64RegisterR24 =    0b11101,
    kEmex64RegisterR25 =    0b11110,

    /*
     * return register: also a general purpose register but
     * it is not affected by bl and ret, this register
     * has the purpose of a called symbol to be able to
     * return without any crazy memory math a value for
     * example.
     */
    kEmex64RegisterRR =     0b11111,

    kEmex64RegisterMAX = kEmex64RegisterRR
};

enum kEmex64ControlRegister: uint8_t {
    kEmex64ControlRegisterCR0 = 0b00000,    /* CREL:    elevation control register */
    kEmex64ControlRegisterCR1 = 0b00001,    /* CRKSP:   kernel stack pointer (the stack pointer the interrupt controller will use when receiving interrupt) */
    kEmex64ControlRegisterCR2 = 0b00010,    /* CREXC:   exception register (first 3bits for the exception) */
    kEmex64ControlRegisterCR3 = 0b00011,    /* CRVEC:   cpu vector table */
    kEmex64ControlRegisterCR4 = 0b00100,    /* CRPTB:   page table pointer (first 8bits are the flags and the rest is the physical address where the page table is) */
    kEmex64ControlRegisterCR5 = 0b00101,    /* CRFPC:   kernel only floating point control register */
    kEmex64ControlRegisterCR6 = 0b00110,
    kEmex64ControlRegisterCR7 = 0b00111,
    kEmex64ControlRegisterCR8 = 0b01000,
    kEmex64ControlRegisterCR9 = 0b01001,
    kEmex64ControlRegisterCR10 = 0b01010,
    kEmex64ControlRegisterCR11 = 0b01011,
    kEmex64ControlRegisterCR12 = 0b01100,
    kEmex64ControlRegisterCR13 = 0b01101,
    kEmex64ControlRegisterCR14 = 0b01110,
    kEmex64ControlRegisterCR15 = 0b01111,
    kEmex64ControlRegisterCR16 = 0b10000,
    kEmex64ControlRegisterCR17 = 0b10001,
    kEmex64ControlRegisterCR18 = 0b10010,
    kEmex64ControlRegisterCR19 = 0b10011,
    kEmex64ControlRegisterCR20 = 0b10100,
    kEmex64ControlRegisterCR21 = 0b10101,
    kEmex64ControlRegisterCR22 = 0b10110,
    kEmex64ControlRegisterCR23 = 0b10111,
    kEmex64ControlRegisterCR24 = 0b11000,
    kEmex64ControlRegisterCR25 = 0b11001,
    kEmex64ControlRegisterCR26 = 0b11010,
    kEmex64ControlRegisterCR27 = 0b11011,
    kEmex64ControlRegisterCR28 = 0b11100,
    kEmex64ControlRegisterCR29 = 0b11101,
    kEmex64ControlRegisterCR30 = 0b11110,
    kEmex64ControlRegisterCR31 = 0b11111,
};

enum kEmex64FloatingRegister: uint8_t {
    kEmex64FloatingRegisterFR0 = 0b00000,
    kEmex64FloatingRegisterFR1 = 0b00001,
    kEmex64FloatingRegisterFR2 = 0b00010,
    kEmex64FloatingRegisterFR3 = 0b00011,
    kEmex64FloatingRegisterFR4 = 0b00100,
    kEmex64FloatingRegisterFR5 = 0b00101,
    kEmex64FloatingRegisterFR6 = 0b00110,
    kEmex64FloatingRegisterFR7 = 0b00111,
    kEmex64FloatingRegisterFR8 = 0b01000,
    kEmex64FloatingRegisterFR9 = 0b01001,
    kEmex64FloatingRegisterFR10 = 0b01010,
    kEmex64FloatingRegisterFR11 = 0b01011,
    kEmex64FloatingRegisterFR12 = 0b01100,
    kEmex64FloatingRegisterFR13 = 0b01101,
    kEmex64FloatingRegisterFR14 = 0b01110,
    kEmex64FloatingRegisterFR15 = 0b01111,
    kEmex64FloatingRegisterFR16 = 0b10000,
    kEmex64FloatingRegisterFR17 = 0b10001,
    kEmex64FloatingRegisterFR18 = 0b10010,
    kEmex64FloatingRegisterFR19 = 0b10011,
    kEmex64FloatingRegisterFR20 = 0b10100,
    kEmex64FloatingRegisterFR21 = 0b10101,
    kEmex64FloatingRegisterFR22 = 0b10110,
    kEmex64FloatingRegisterFR23 = 0b10111,
    kEmex64FloatingRegisterFR24 = 0b11000,
    kEmex64FloatingRegisterFR25 = 0b11001,
    kEmex64FloatingRegisterFR26 = 0b11010,
    kEmex64FloatingRegisterFR27 = 0b11011,
    kEmex64FloatingRegisterFR28 = 0b11100,
    kEmex64FloatingRegisterFR29 = 0b11101,
    kEmex64FloatingRegisterFR30 = 0b11110,
    kEmex64FloatingRegisterFR31 = 0b11111,
    kEmex64FloatingRegisterMAX = kEmex64FloatingRegisterFR31
};

typedef union {
    uint64_t u64;
    uint32_t u32;
    int64_t i64;
    int32_t i32;
    double f64;
    float f32;
} FPReg;

enum kEmex64ElevationLevel: uint8_t {
    kEmex64ElevationLevelUser =             0b00,
    kEmex64ElevationLevelKernel =           0b01,
    kEmex64ElevationLevelSecureMonitor =    0b10    /* used for software kernel secure mechanism like apples PPL */
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

    /*
     * when the MMU sees a page is dirty and a user program
     * wants to write to it it will cause a page fault or
     * when a page was accessed that is not accessible or
     * not mapped.
     */
    kEmex64ExceptionPageFault =         0b101,

    /*
     * KTRR exception!?
     */
    kEmex64ExceptionKTRRViolation =     0b110,
};

#define EMEX64_MAX_ARGS 26
#define EMEX64_MAX_ILEN (1 + EMEX64_MAX_ARGS * 9)

typedef struct emex64_core emex64_core_t;

/* definition of the handler of each operation */
typedef void (*emex64_opfunc_t)(emex64_core_t *core);

typedef struct emex64_machine emex64_machine_t;

typedef struct emex64_opfunc_entry {
    emex64_opfunc_t func;
    uint8_t maxargs;
} emex64_opfunc_entry_t;

typedef struct emex64_core {

    /* the pthread this core is running on on the host */
    pthread_t pthread;

    /* a array of all (control) registers */
    uint64_t rl[kEmex64RegisterMAX + 1];
    FPReg frl[kEmex64FloatingRegisterMAX + 1];

    /* data of currently decoding or decoded operation */
    struct {
        /*
         * 145 bytes instruction cache
         */
        uint8_t inscache[EMEX64_MAX_ILEN];

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
        uint64_t imm[EMEX64_MAX_ARGS];

        /*
         * pointer array for parameters, at emulation we
         * dont have many emulation options so we stuff
         * each parameter into this array.. register
         * intermediate, etc.
         */
        uint8_t param_cnt;
        uint64_t *param[EMEX64_MAX_ARGS];
        enum kEmex64ParameterCoding param_coding[EMEX64_MAX_ARGS];
    } op;

    /*
     * control state is a optimization to minimize
     * usage of packed register reading.
     */
    struct {
        struct {
            enum kEmex64ElevationLevel level;
        } crel;

        struct {
            uint64_t address;
        } crksp;

        struct {
            enum kEmex64Exception exception;
        } crexc;

        /*
         * to be created
         * struct {
         *
         * } crvec;
        */

        struct {
            bool enabled;
            uint64_t pgd_addr;
        } crptb;

        /*
         * to be created
         * struct {
         *
         * } crfpc;
        */
    } cr_state;

    /*
     * cpu halting status (will later be in the same
     * control register as the exception register CR0).
     */
    bool halted;

    /*
     * cpu cant get a second interrupt while handling
     * one, but will be unset when the cpu calls iret.
     */
    bool in_interrupt;

    /* pointer back to machine */
    emex64_machine_t *machine;
} emex64_core_t;

emex64_core_t *emex64_core_alloc(void);
void emex64_core_dealloc(emex64_core_t *core);
void emex64_core_execute(emex64_core_t *core);
void emex64_core_terminate(emex64_core_t *core);

#endif /* EMEX64VM_CORE_H */
