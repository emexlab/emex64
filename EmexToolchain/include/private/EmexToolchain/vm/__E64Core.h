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

#ifndef __E64CORE_H
#define __E64CORE_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/vm/E64Type.h>

#define EMEX64_MAX_ARGS 16
#define EMEX64_MAX_ILEN (1 + EMEX64_MAX_ARGS * 9)

typedef struct __E64Machine *E64MachineRef;

typedef union {
    UInt64 u64;
    UInt32 u32;
    int64_t i64;
    int32_t i32;
    double f64;
    float f32;
} FPReg;

typedef struct __E64Core {
    EFObject header;

    /* the pthread this core is running on on the host */
    pthread_t pthread;

    /* a array of all (control) registers */
    UInt64 rl[kE64RegisterMAX + 1];
    FPReg frl[kE64FloatingRegisterMAX + 1];

    /* data of currently decoding or decoded operation */
    struct {
        /*
         * the instruction cache holds all bytes a instruction
         * can contain at a maximum at the current PC address,
         * it exists for clean synchronisation later when
         * multithreading gets added. for a future reader:
         * it is necessary to count + 8, otherwise bb_read
         * might overread the inscache.
         * 
         * And the immediate cache stores all immediates into
         * a cache so they can like registers be used inside
         * the parameter pointer array.
         */
        UInt8 inscache[EMEX64_MAX_ILEN + 8];
        UInt64 immcache[EMEX64_MAX_ARGS];

        /*
         * lenght of decoded instruction so that the cpu
         * can correctly increment the program counter.
         */
        UInt8 ilen;

        /*
         * the opcode it self, so the cpu knows what to
         * execute. The opcode entry which tells the decoder
         * how to decode the instruction.
         */
        E64Opcode opcode;
        const struct emex64_opfunc_entry *opce;

        /*
         * pointer array for parameters, at emulation we
         * dont have many emulation options so we stuff
         * each parameter into this array.. register
         * immediate, etc.
         */
        UInt8 param_cnt;
        UInt64 *param[EMEX64_MAX_ARGS];
        E64ParameterCoding param_coding[EMEX64_MAX_ARGS];
    } op;

    /*
     * control state is a optimization to minimize
     * usage of packed register reading.
     */
    struct {
        struct {
            E64ElevationLevel level;
        } crel;

        struct {
            UInt64 address;
        } crksp;

        struct {
            E64Exception exception;
        } crexc;

        /*
         * to be created
         * struct {
         *
         * } crvec;
        */

        struct {
            Boolean enabled;
            UInt64 pgd_addr;
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
    Boolean halted;

    /*
     * cpu cant get a second interrupt while handling
     * one, but will be unset when the cpu calls iret.
     */
    Boolean in_interrupt;

    /* pointer back to machine */
    E64MachineRef machine;
} *__E64Core;

typedef void (*emex64_opfunc_t)(__E64Core core);

typedef struct emex64_opfunc_entry {
    emex64_opfunc_t func;
    UInt8 minargs;
    UInt8 maxargs;
    UInt32 argmask;
} emex64_opfunc_entry_t;

extern const emex64_opfunc_entry_t kE64OpfuncTable[];

#endif /* __E64CORE_H */
