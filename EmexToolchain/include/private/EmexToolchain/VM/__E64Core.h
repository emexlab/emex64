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
#include <EmexToolchain/VM/E64Type.h>
#include <EmexToolchain/VM/__E64Cache.h>
#include <EmexToolchain/VM/device/internal/controller/apic.h>

#define EMEX64_MAX_ARGS 16
#define EMEX64_MAX_ILEN (1 + EMEX64_MAX_ARGS * 9)

typedef struct __E64Machine *E64MachineRef;

typedef union {
    UInt64 u64;
    UInt32 u32;
    SInt64 i64;
    SInt32 i32;
    double f64;
    float f32;
} FPReg;

#define EMEX64_PAC_CACHE_ENTRIES 128
#define EMEX64_FAST_PAGES 64

typedef struct emex64_pac_cache_entry {
    UInt64 key;
    UInt64 ptr;
    UInt64 modifier;
    UInt64 signature;
} emex64_pac_cache_entry_t;

typedef struct emex64_fast_page {
    UInt64 key;
    UInt64 ppage;
    UInt64 map_seq;
    UInt64 tlb_seq;
} emex64_fast_page_t;

static inline UInt64 emex64_pac_cache_index(UInt64 ptr, UInt64 modifier)
{
    return (ptr ^ (ptr >> 7) ^ modifier ^ (modifier >> 4)) & (EMEX64_PAC_CACHE_ENTRIES - 1);
}

typedef struct __E64Core {
    EFObject header;

    /* the pthread this core is running on on the host */
    pthread_t pthread;

    /* a array of all (control) registers */
    UInt64 rl[kE64RegisterMAX + 1];
    UInt64 erl[kE64RegisterExtendedMAX + 1];
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
        UInt64 **param;
        E64ParameterCoding *param_coding;
        const emex64_icache_entry_t *ce;
        UInt64 *param_store[EMEX64_MAX_ARGS];
        E64ParameterCoding coding_store[EMEX64_MAX_ARGS];
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

        struct {
            UInt16 activeIsa;
        } crisa;

        /* trap model */
        UInt64 crstat;
        UInt64 crtvec;
        UInt64 crepc;
        UInt64 crcause;
        UInt64 crtval;
        UInt64 crscratch;

        /* pointer authentication (PAC) */
        UInt64 criakey;
        UInt64 crdakey;
        UInt64 crpac;
    } cr_state;

    /* trap model bookkeeping */
    struct {
        Boolean fault_valid;
        UInt64 fault_addr;
        UInt8 fault_access;
        Boolean svc_pending;
        UInt64 rl[16];
        UInt64 erl[16];
    } trap;

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
    Boolean irq_poll;
    Boolean irq_ie;

    /* pointer back to machine */
    E64MachineRef machine;

    struct {
        UInt64 vpage;
        UInt64 ppage;
        UInt64 map_seq;
        UInt64 context;
    } fetch;

    emex64_fast_page_t pages[2][EMEX64_FAST_PAGES];
    UInt64 tlb_seq;

    Boolean run_break;

    /* more caching */
    emex64_tlb_t tlb;
    emex64_icache_entry_t *icache;
    emex64_pac_cache_entry_t pac_cache[EMEX64_PAC_CACHE_ENTRIES];
    emex64_snoop_t *snoop;
    UInt8 *ram;
    UInt64 ram_size;

    /* multicore support */
    UInt8 id;
    emex64_lapic_t lapic;
    UInt32 run_state;
    UInt32 request;
    struct {
        UInt64 start;
        UInt64 arg;
        E64ElevationLevel level;
        UInt16 isa;
    } startup;
    pthread_mutex_t wait_lock;
    pthread_cond_t wait_cond;

    Boolean sleeping;
    Boolean event;
    Boolean wfe;
} *__E64Core;

#define E64_CORE_RUNNING    1
#define E64_CORE_WAITING    0

typedef void (*emex64_opfunc_t)(__E64Core core);

typedef struct emex64_opfunc_entry {
    emex64_opfunc_t func;
    UInt8 minargs;
    UInt8 maxargs;
    UInt32 argmask;
    UInt16 isa;
} emex64_opfunc_entry_t;

extern const emex64_opfunc_entry_t kE64OpfuncTable[];

static inline void emex64_core_context_changed(__E64Core core)
{
    core->fetch.vpage = ~0ULL;
    core->run_break = true;
}

static inline void emex64_core_tlb_changed(__E64Core core)
{
    core->tlb_seq++;
    emex64_core_context_changed(core);
}

void __E64CoreWritePageTableBase(__E64Core core, UInt64 value);
struct __E64Machine;
struct __E64Core *__E64CoreCreateWithID(EFAllocatorRef allocatorRef, struct __E64Machine *machine, UInt8 id);
void __E64CoreSendIPI(__E64Core sender, __E64Core target, UInt64 type);
void __E64CoreKick(__E64Core core);
void __E64CoreRequestReset(__E64Core core);
void __E64CoreResetForPowerOn(__E64Core core);
UInt64 __E64CoreReadPageTableBase(__E64Core core);

#endif /* __E64CORE_H */
