/*
 * MIT License
 *
 * Copyright (c) 2024 cr4zyengineer
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

#ifndef LA64VM_CORE_H
#define LA64VM_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#pragma mark - opcode

/* core operations */
#define LA64_OPCODE_HLT             0b00000000
#define LA64_OPCODE_NOP             0b00000001

/* data operations */
#define LA64_OPCODE_MOV             0b00000010
#define LA64_OPCODE_SWP             0b00000011
#define LA64_OPCODE_SWPZ            0b00000100
#define LA64_OPCODE_PUSH            0b00000101
#define LA64_OPCODE_POP             0b00000110
#define LA64_OPCODE_LDB             0b00000111
#define LA64_OPCODE_LDW             0b00001000
#define LA64_OPCODE_LDD             0b00001001
#define LA64_OPCODE_LDQ             0b00001010
#define LA64_OPCODE_STB             0b00001011
#define LA64_OPCODE_STW             0b00001100
#define LA64_OPCODE_STD             0b00001101
#define LA64_OPCODE_STQ             0b00001110

/* alu operations */
#define LA64_OPCODE_ADD             0b00001111
#define LA64_OPCODE_SUB             0b00010000
#define LA64_OPCODE_MUL             0b00010001
#define LA64_OPCODE_DIV             0b00010010
#define LA64_OPCODE_IDIV            0b00010011
#define LA64_OPCODE_MOD             0b00010100
#define LA64_OPCODE_NOT             0b00010101
#define LA64_OPCODE_NEG             0b00010110
#define LA64_OPCODE_AND             0b00010111
#define LA64_OPCODE_OR              0b00011000
#define LA64_OPCODE_XOR             0b00011001
#define LA64_OPCODE_SHR             0b00011010
#define LA64_OPCODE_SHL             0b00011011
#define LA64_OPCODE_SAR             0b00011100
#define LA64_OPCODE_ROR             0b00011101
#define LA64_OPCODE_ROL             0b00011110
#define LA64_OPCODE_PDEP            0b00011111
#define LA64_OPCODE_PEXT            0b00100000
#define LA64_OPCODE_BSWAPW          0b00100001
#define LA64_OPCODE_BSWAPD          0b00100010
#define LA64_OPCODE_BSWAPQ          0b00100011

/* control flow operations */
#define LA64_OPCODE_B               0b00100100
#define LA64_OPCODE_CMP             0b00100101
#define LA64_OPCODE_BE              0b00100110
#define LA64_OPCODE_BNE             0b00100111
#define LA64_OPCODE_BLT             0b00101000
#define LA64_OPCODE_BGT             0b00101001
#define LA64_OPCODE_BLE             0b00101010
#define LA64_OPCODE_BGE             0b00101011
#define LA64_OPCODE_BZ              0b00101100
#define LA64_OPCODE_BNZ             0b00101101
#define LA64_OPCODE_BL              0b00101110
#define LA64_OPCODE_RET             0b00101111
#define LA64_OPCODE_IRET            0b00110000

#define LA64_OPCODE_MAX             LA64_OPCODE_IRET

#pragma mark - parameter modes

#define LA64_PARAMETER_CODING_INSTR_END 0b000   /* instruction end marker for instructions dynamic in lenght */
#define LA64_PARAMETER_CODING_REG       0b001   /* register */
#define LA64_PARAMETER_CODING_IMM8      0b010   /* 8bit intermediate */
#define LA64_PARAMETER_CODING_IMM16     0b011   /* 16bit intermediate */
#define LA64_PARAMETER_CODING_IMM32     0b100   /* 32bit intermediate */
#define LA64_PARAMETER_CODING_IMM64     0b101   /* 64bit intermediate */
/* leaving 0b110 and 0b111 open for later additions */

/* registers */

/*
 * program counter: it points to the current address at
 * which the CPU currently is, it increments by the
 * lenght of the instruction when the CPU is done
 * executing the instruction at which PC points to at
 * that time.
 */
#define LA64_REGISTER_PC    0b00000

/*
 * stack pointer: it points to the current address at
 * which the stack lives, stack grows downwards on
 * allocation and upwards on deallocation.
 *
 * stack allocation is meant to be done for small
 * things, as heap allocation is way more expensive,
 * than a simple register decrement.
 */
#define LA64_REGISTER_SP    0b00001

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
#define LA64_REGISTER_FP    0b00010

/*
 * control flag: used by control flow instructions like
 * cmp, je, jne.. basically used for if else statements.
 */
#define LA64_REGISTER_CF    0b00011

/*
 * general purpose registers: these registers arent used
 * for anything other than the software, these registers
 * can be used for any purpose, thats why they are called
 * general purpose registers, because they got no fixed
 * purpose like pc, sp, fp, cf.
 */
#define LA64_REGISTER_R0    0b00100
#define LA64_REGISTER_R1    0b00101
#define LA64_REGISTER_R2    0b00110
#define LA64_REGISTER_R3    0b00111
#define LA64_REGISTER_R4    0b01000
#define LA64_REGISTER_R5    0b01001
#define LA64_REGISTER_R6    0b01010
#define LA64_REGISTER_R7    0b01011
#define LA64_REGISTER_R8    0b01100
#define LA64_REGISTER_R9    0b01101
#define LA64_REGISTER_R10   0b01110
#define LA64_REGISTER_R11   0b01111
#define LA64_REGISTER_R12   0b10000
#define LA64_REGISTER_R13   0b10001
#define LA64_REGISTER_R14   0b10010
#define LA64_REGISTER_R15   0b10011
#define LA64_REGISTER_R16   0b10100

/*
 * return register: also a general purpose register but
 * it is not affected by bl and ret, this register
 * has the purpose of a called symbol to be able to
 * return without any crazy memory math a value for
 * example.
 */
#define LA64_REGISTER_RR    0b10101

/* control registers */
#define LA64_REGISTER_CR0   0b10110 /* CREL:    elevation control register */
#define LA64_REGISTER_CR1   0b10111 /* CRKSP:   kernel stack pointer (the stack pointer the interrupt controller will use when receiving interrupt) */
#define LA64_REGISTER_CR2   0b11000 /* CREXC:   exception register (first 3bits for the exception) */
#define LA64_REGISTER_CR3   0b11001 /* CRVEC:   cpu vector table */
#define LA64_REGISTER_CR4   0b11010 /* CRPTB:   page table pointer (first 8bits are the flags and the rest is the physical address where the page table is) */
#define LA64_REGISTER_CR5   0b11011
#define LA64_REGISTER_CR6   0b11100
#define LA64_REGISTER_CR7   0b11101
#define LA64_REGISTER_CR8   0b11110
#define LA64_REGISTER_CR9   0b11111

#define LA64_REGISTER_MAX   LA64_REGISTER_CR9

/* elevation levels */
#define LA64_ELEVATION_USER             0b00
#define LA64_ELEVATION_KERNEL           0b01
#define LA64_ELEVATION_SECURE_MONITOR   0b10    /* used for software kernel secure mechanism like the apples PPL */

/* compare flags */

/*
 * these flags is what CF contains of, yk we talked
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
#define LA64_CMP_Z  0x1
#define LA64_CMP_L  0x2
#define LA64_CMP_G  0x4

/* exception flags */

/*
 * normal state, simply a marker to say nothing
 * to trigger a interrupt for.
 */
#define LA64_EXCEPTION_NONE              0b000

/*
 * this exception means that a memory address was
 * accessed inappropriately, which means memory
 * if the cpu writes to memory that it doesnt have
 * access to this exception is triggered.
 */
#define LA64_EXCEPTION_BAD_ACCESS        0b001

/*
 * this exception means that the current cpu state
 * did not have the appropriate permissions to
 * access a certain register for example.
 */
#define LA64_EXCEPTION_PERMISSION        0b010

/*
 * this exception means that the cpu regocnised a
 * instruction that was not valid was being tried
 * to decode.
 */
#define LA64_EXCEPTION_BAD_INSTRUCTION   0b011

/*
 * the alu tried to perform illegal math operations
 * like for example N / 0 or N % 0.
 */
#define LA64_EXCEPTION_BAD_ARITHMETIC    0b100

typedef struct la64_machine la64_machine_t;
typedef struct la64_operation la64_operation_t;

typedef struct la64_core {

    /* the pthread this core is running on on the host */
    pthread_t pthread;

    /* a array of all (control) registers */
    uint64_t rl[LA64_REGISTER_MAX + 1];

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
        uint8_t op;

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

/* definition of the handler of each operation */
typedef void (*la64_opfunc_t)(la64_core_t *core);

la64_core_t *la64_core_alloc(void);
void la64_core_dealloc(la64_core_t *core);
void la64_core_execute(la64_core_t *core);
void la64_core_terminate(la64_core_t *core);

#endif /* LA64VM_CORE_H */
