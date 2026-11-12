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
#include <unistd.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/instruction/data.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/E64Memory.h>
#include <EmexToolchain/Support/likely.h>

void emex64_op_mov(E64CoreRef core)
{
    *(core->op.param[0]) = *(core->op.param[1]);
}

void emex64_op_swp(E64CoreRef core)
{
    UInt64 param_backup = *(core->op.param[0]);
    *(core->op.param[0]) = *(core->op.param[1]);
    *(core->op.param[1]) = param_backup;
}

void emex64_op_movz(E64CoreRef core)
{
    *(core->op.param[0]) = *(core->op.param[1]);
    *(core->op.param[1]) = 0;
}

void emex64_op_push(E64CoreRef core)
{
    for(UInt8 i = 0; i < core->op.param_cnt; i++)
    {
        E64MemoryCoreAction(core->machine->memory, core, core->rl[kE64RegisterSP], sizeof(UInt64), core->op.param[i], kE64MemoryActionTypeWrite);
        core->rl[kE64RegisterSP] -= 8;
    }
}

void emex64_op_pop(E64CoreRef core)
{
    for(UInt8 i = 0; i < core->op.param_cnt; i++)
    {
        core->rl[kE64RegisterSP] += 8;
        E64MemoryCoreAction(core->machine->memory, core, core->rl[kE64RegisterSP], sizeof(UInt64), core->op.param[i], kE64MemoryActionTypeRead);
    }
}

void emex64_op_ldb(E64CoreRef core)
{
    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(UInt8), core->op.param[0], kE64MemoryActionTypeRead);
}

void emex64_op_ldw(E64CoreRef core)
{
    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(UInt16), core->op.param[0], kE64MemoryActionTypeRead);
}

void emex64_op_ldd(E64CoreRef core)
{
    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(UInt32), core->op.param[0], kE64MemoryActionTypeRead);
}

void emex64_op_ldq(E64CoreRef core)
{
    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(UInt64), core->op.param[0], kE64MemoryActionTypeRead);
}

void emex64_op_stb(E64CoreRef core)
{
    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(UInt8), core->op.param[1], kE64MemoryActionTypeWrite);
}

void emex64_op_stw(E64CoreRef core)
{
    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(UInt16), core->op.param[1], kE64MemoryActionTypeWrite);
}

void emex64_op_std(E64CoreRef core)
{
    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(UInt32), core->op.param[1], kE64MemoryActionTypeWrite);
}

void emex64_op_stq(E64CoreRef core)
{
    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(UInt64), core->op.param[1], kE64MemoryActionTypeWrite);
}

void emex64_op_clr(E64CoreRef core)
{
    for(UInt8 i = 0; i < core->op.param_cnt; i++)
    {
        *core->op.param[i] = 0;
    }
}

static Boolean emex64_cr_write(E64CoreRef core, UInt8 cr_select, UInt64 cr_value)
{
    if(core->cr_state.crel.level < kE64ElevationLevelKernel)
    {
        core->cr_state.crexc.exception = kE64ExceptionPermission;
        return false;
    }

    switch(cr_select)
    {
        case kE64ControlRegisterCREL:
            if(kE64ElevationLevelSecureMonitor < cr_value)
            {
                core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
                return false;
            }
            if(core->cr_state.crel.level < cr_value)
            {
                core->cr_state.crexc.exception = kE64ExceptionPermission;
                return false;
            }
            core->cr_state.crel.level = cr_value;
            break;
        case kE64ControlRegisterCRKSP:
            core->cr_state.crksp.address = cr_value;
            break;
        case kE64ControlRegisterCREXC:
            core->cr_state.crexc.exception = (UInt8)cr_value;
            break;
        case kE64ControlRegisterCRPTB:
            __E64CoreWritePageTableBase(core, cr_value);
            break;
        case kE64ControlRegisterCRISA:
        {
            UInt16 requestedIsa = cr_value & 0xFFFF;
            if(requestedIsa > E64VM_ISA_MAX_VERSION || requestedIsa < E64VM_ISA_MIN_VERSION)
            {
                core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
                return false;
            }

            core->cr_state.crisa.activeIsa = requestedIsa;
            break;
        }
        case kE64ControlRegisterCRSTAT:
        {
            UInt64 st = cr_value & E64_CRSTAT_WRITABLE;
            if(((st & E64_CRSTAT_PEL_MASK) >> E64_CRSTAT_PEL_SHIFT) > core->cr_state.crel.level)
            {
                core->cr_state.crexc.exception = kE64ExceptionPermission;
                return false;
            }
            core->cr_state.crstat = st;
            break;
        }
        case kE64ControlRegisterCRTVEC:
            core->cr_state.crtvec = cr_value;
            break;
        case kE64ControlRegisterCREPC:
            core->cr_state.crepc = cr_value;
            break;
        case kE64ControlRegisterCRCAUSE:
            core->cr_state.crcause = cr_value;
            break;
        case kE64ControlRegisterCRTVAL:
            core->cr_state.crtval = cr_value;
            break;
        case kE64ControlRegisterCRSCRATCH:
            core->cr_state.crscratch = cr_value;
            break;
        case kE64ControlRegisterCRIAKEY:
            core->cr_state.criakey = cr_value;
            break;
        case kE64ControlRegisterCRDAKEY:
            core->cr_state.crdakey = cr_value;
            break;
        case kE64ControlRegisterCRPAC:
            core->cr_state.crpac = cr_value & E64_CRPAC_WRITABLE;
            break;
        default:
            core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
            return false;
    }
    return true;
}

static Boolean emex64_cr_read(E64CoreRef core, UInt8 cr_select, UInt64 *cr_recv)
{
    Boolean kernel = core->cr_state.crel.level >= kE64ElevationLevelKernel;

    switch(cr_select)
    {
        case kE64ControlRegisterCREL:
            *cr_recv = core->cr_state.crel.level;
            return true;
        case kE64ControlRegisterCREXC:
            *cr_recv = core->cr_state.crexc.exception;
            return true;
        case kE64ControlRegisterCRISA:
            *cr_recv = core->cr_state.crisa.activeIsa;
            *cr_recv |= E64VM_ISA_MAX_VERSION << 16;
            *cr_recv |= (UInt64)E64VM_CPU_MODEL << 32;
            *cr_recv |= E64_CRISA_EXT_PAC;
            return true;
        case kE64ControlRegisterCRIAKEY:
        case kE64ControlRegisterCRDAKEY:
            core->cr_state.crexc.exception = kE64ExceptionPermission;
            return false;
        case kE64ControlRegisterCRKSP:
        case kE64ControlRegisterCRPTB:
        case kE64ControlRegisterCRSTAT:
        case kE64ControlRegisterCRTVEC:
        case kE64ControlRegisterCREPC:
        case kE64ControlRegisterCRCAUSE:
        case kE64ControlRegisterCRTVAL:
        case kE64ControlRegisterCRSCRATCH:
        case kE64ControlRegisterCRPAC:
            if(!kernel)
            {
                core->cr_state.crexc.exception = kE64ExceptionPermission;
                return false;
            }
            break;
        default:
            core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
            return false;
    }

    switch(cr_select)
    {
        case kE64ControlRegisterCRKSP:
            *cr_recv = core->cr_state.crksp.address;
            break;
        case kE64ControlRegisterCRPTB:
            *cr_recv = __E64CoreReadPageTableBase(core);
            break;
        case kE64ControlRegisterCRSTAT:
            *cr_recv = core->cr_state.crstat;
            break;
        case kE64ControlRegisterCRTVEC:
            *cr_recv = core->cr_state.crtvec;
            break;
        case kE64ControlRegisterCREPC:
            *cr_recv = core->cr_state.crepc;
            break;
        case kE64ControlRegisterCRCAUSE:
            *cr_recv = core->cr_state.crcause;
            break;
        case kE64ControlRegisterCRTVAL:
            *cr_recv = core->cr_state.crtval;
            break;
        case kE64ControlRegisterCRSCRATCH:
            *cr_recv = core->cr_state.crscratch;
            break;
        case kE64ControlRegisterCRPAC:
            *cr_recv = core->cr_state.crpac;
            break;
    }
    return true;
}

void emex64_op_cmov(E64CoreRef core)
{
    emex64_cr_write(core, (UInt8)*(core->op.param[0]), *(core->op.param[1]));
}

void emex64_op_cmovb(E64CoreRef core)
{
    UInt64 value;
    if(emex64_cr_read(core, (UInt8)*(core->op.param[1]), &value))
    {
        *(core->op.param[0]) = value;
    }
}

void emex64_op_cswp(E64CoreRef core)
{
    UInt8 cr_select = (UInt8)*(core->op.param[0]);
    UInt64 old;
    if(core->cr_state.crel.level < kE64ElevationLevelKernel)
    {
        core->cr_state.crexc.exception = kE64ExceptionPermission;
        return;
    }
    if(!emex64_cr_read(core, cr_select, &old) || !emex64_cr_write(core, cr_select, *(core->op.param[1])))
    {
        return;
    }
    *(core->op.param[1]) = old;
}

static inline UInt64 *emex64_atomic_word(E64CoreRef core,
                                         UInt64 addr)
{
    if(addr & 7)
    {
        return NULL;
    }
    return (UInt64 *)E64MemoryCoreMap(core->machine->memory, core, addr, sizeof(UInt64), kE64MemoryActionTypeWrite);
}

static inline void emex64_atomic_written(E64CoreRef core,
                                         void *word,
                                         UInt64 size)
{
    emex64_snoop_write(core->snoop, (UInt64)((UInt8 *)word - core->ram), size);
}

void emex64_op_xchg(E64CoreRef core)
{
    UInt64 addr = *(core->op.param[1]);
    UInt64 old = 0;
    UInt64 value = *(core->op.param[0]);

    UInt64 *word = emex64_atomic_word(core, addr);
    if(likely(word != NULL))
    {
        *(core->op.param[0]) = TO_HOST64(__atomic_exchange_n(word, TO_HOST64(value), __ATOMIC_SEQ_CST));
        emex64_atomic_written(core, word, sizeof(UInt64));
        return;
    }

    E64MemoryCoreAction(core->machine->memory, core, addr, sizeof(UInt64), &old, kE64MemoryActionTypeRead);
    if(core->cr_state.crexc.exception != kE64ExceptionNone)
    {
        return;
    }

    E64MemoryCoreAction(core->machine->memory, core, addr, sizeof(UInt64), &value, kE64MemoryActionTypeWrite);
    if(core->cr_state.crexc.exception != kE64ExceptionNone)
    {
        return;
    }

    *(core->op.param[0]) = old;
}

void emex64_op_cas(E64CoreRef core)
{
    UInt64 addr = *(core->op.param[1]);
    UInt64 expected = *(core->op.param[0]);
    UInt64 value = *(core->op.param[2]);
    UInt64 old = 0;

    UInt64 *word = emex64_atomic_word(core, addr);
    if(likely(word != NULL))
    {
        UInt64 seen = TO_HOST64(expected);
        Boolean swapped = __atomic_compare_exchange_n(word, &seen, TO_HOST64(value), false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
        if(swapped)
        {
            emex64_atomic_written(core, word, sizeof(UInt64));
        }
        core->rl[kE64RegisterCF] = swapped ? kE64CompareFlagZ : 0;
        *(core->op.param[0]) = swapped ? expected : TO_HOST64(seen);
        return;
    }

    E64MemoryCoreAction(core->machine->memory, core, addr, sizeof(UInt64), &old, kE64MemoryActionTypeRead);
    if(core->cr_state.crexc.exception != kE64ExceptionNone)
    {
        return;
    }
    if(old == expected)
    {
        E64MemoryCoreAction(core->machine->memory, core, addr, sizeof(UInt64), &value, kE64MemoryActionTypeWrite);
        if(core->cr_state.crexc.exception != kE64ExceptionNone)
        {
            return;
        }
        core->rl[kE64RegisterCF] = kE64CompareFlagZ;
    }
    else
    {
        core->rl[kE64RegisterCF] = 0;
    }

    *(core->op.param[0]) = old;
}

void emex64_op_clar(__E64Core core)
{
    for(UInt8 index = kE64RegisterR0; index < kE64RegisterMAX; index++)
    {
        core->rl[index] = 0;
    }
}

void emex64_op_rdrnd(__E64Core core)
{
    UInt32 *cursor = (UInt32*)(core->op.param[0]);
    cursor[0] = arc4random();
    cursor[1] = arc4random();
}

void emex64_op_movm(__E64Core core)
{
    UInt64 dst = *(core->op.param[0]);
    UInt64 src = *(core->op.param[1]);
    UInt64 len = *(core->op.param[2]);
    if(len == 0 || dst == src)
    {
        return;
    }

    void *dptr = E64MemoryCoreMap(core->machine->memory, core, dst, len, kE64MemoryActionTypeWrite);
    void *sptr = E64MemoryCoreMap(core->machine->memory, core, src, len, kE64MemoryActionTypeRead);
    if(dptr != NULL && sptr != NULL)
    {
        memmove(dptr, sptr, len);
        return;
    }

    if(dst < src)
    {
        while(len--)
        {
            UInt64 value = 0;
            E64MemoryCoreAction(core->machine->memory, core, src, 1, &value, kE64MemoryActionTypeRead);
            E64MemoryCoreAction(core->machine->memory, core, dst, 1, &value, kE64MemoryActionTypeWrite);
            if(core->cr_state.crexc.exception != kE64ExceptionNone)
            {
                return;
            }
            src++;
            dst++;
        }
    }
    else
    {
        src += len;
        dst += len;
        while(len--)
        {
            UInt64 value = 0;
            src--;
            dst--;
            E64MemoryCoreAction(core->machine->memory, core, src, 1, &value, kE64MemoryActionTypeRead);
            E64MemoryCoreAction(core->machine->memory, core, dst, 1, &value, kE64MemoryActionTypeWrite);
            if(core->cr_state.crexc.exception != kE64ExceptionNone)
            {
                return;
            }
        }
    }
}

void emex64_op_cpym(__E64Core core)
{
    UInt64 dst = *(core->op.param[0]);
    UInt64 src = *(core->op.param[1]);
    UInt64 len = *(core->op.param[2]);
    if(len == 0)
    {
        return;
    }

    void *dptr = E64MemoryCoreMap(core->machine->memory, core, dst, len, kE64MemoryActionTypeWrite);
    void *sptr = E64MemoryCoreMap(core->machine->memory, core, src, len, kE64MemoryActionTypeRead);
    if(dptr != NULL && sptr != NULL)
    {
        memcpy(dptr, sptr, len);
        return;
    }

    while(len--)
    {
        UInt64 value = 0;
        E64MemoryCoreAction(core->machine->memory, core, src, 1, &value, kE64MemoryActionTypeRead);
        E64MemoryCoreAction(core->machine->memory, core, dst, 1, &value, kE64MemoryActionTypeWrite);
        if(core->cr_state.crexc.exception != kE64ExceptionNone)
        {
            return;
        }
        src++;
        dst++;
    }
}

void emex64_op_setm(__E64Core core)
{
    UInt64 dst = *(core->op.param[0]);
    UInt8 value = *(core->op.param[1]) & 0xFF;
    UInt64 len = *(core->op.param[2]);
    if(len == 0)
    {
        return;
    }

    void *ptr = E64MemoryCoreMap(core->machine->memory, core, dst, len, kE64MemoryActionTypeWrite);
    if(ptr != NULL)
    {
        memset(ptr, (int)value, len);
        return;
    }

    while(len--)
    {
        UInt64 byte = value;
        E64MemoryCoreAction(core->machine->memory, core, dst, 1, &byte, kE64MemoryActionTypeWrite);
        if(core->cr_state.crexc.exception != kE64ExceptionNone)
        {
            return;
        }
        dst++;
    }
}

void emex64_op_rdtime(E64CoreRef core)
{
    emex64_timer_t *timer = ((__E64Machine)core->machine)->timer;
    UInt64 value = 0;
    if(likely(timer != NULL) && !emex64_timer_read_count(timer, &value))
    {
        value = emex64_timer_read(core, timer, TIMER_REG_COUNT, sizeof(UInt64));
    }
    *(core->op.param[0]) = value;
}

static inline void emex64_load_signed(E64CoreRef core,
                                      UInt64 size)
{
    UInt64 value = 0;
    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), size, &value, kE64MemoryActionTypeRead);
    if(likely(core->cr_state.crexc.exception == kE64ExceptionNone))
    {
        unsigned shift = 64 - (unsigned)size * 8;
        *(core->op.param[0]) = (UInt64)((SInt64)(value << shift) >> shift);
    }
}

void emex64_op_ldsb(E64CoreRef core)
{
    emex64_load_signed(core, sizeof(UInt8));
}

void emex64_op_ldsw(E64CoreRef core)
{
    emex64_load_signed(core, sizeof(UInt16));
}

void emex64_op_ldsd(E64CoreRef core)
{
    emex64_load_signed(core, sizeof(UInt32));
}

typedef enum {
    kE64AtomicAdd,
    kE64AtomicAnd,
    kE64AtomicOr,
    kE64AtomicXor,
} emex64_atomic_op_t;

static inline UInt64 emex64_atomic_apply(emex64_atomic_op_t op,
                                         UInt64 old,
                                         UInt64 v)
{
    switch(op)
    {
        case kE64AtomicAdd:
            return old + v;
        case kE64AtomicAnd:
            return old & v;
        case kE64AtomicOr:
            return old | v;
        default:
            return old ^ v;
    }
}

static inline UInt32 *emex64_atomic_word32(E64CoreRef core,
                                           UInt64 addr)
{
    if(addr & 3)
    {
        return NULL;
    }
    return (UInt32 *)E64MemoryCoreMap(core->machine->memory, core, addr, sizeof(UInt32), kE64MemoryActionTypeWrite);
}

static void emex64_atomic_rmw(E64CoreRef core,
                              emex64_atomic_op_t op,
                              UInt8 vi,
                              UInt64 size)
{
    UInt64 addr = *(core->op.param[1]);
    UInt64 v = *(core->op.param[vi]);
    UInt64 old;

    if(size == sizeof(UInt64))
    {
        UInt64 *word = emex64_atomic_word(core, addr);
        if(likely(word != NULL))
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            switch(op)
            {
                case kE64AtomicAdd:
                    old = __atomic_fetch_add(word, v, __ATOMIC_SEQ_CST);
                    break;
                case kE64AtomicAnd:
                    old = __atomic_fetch_and(word, v, __ATOMIC_SEQ_CST);
                    break;
                case kE64AtomicOr:
                    old = __atomic_fetch_or(word, v, __ATOMIC_SEQ_CST);
                    break;
                default:
                    old = __atomic_fetch_xor(word, v, __ATOMIC_SEQ_CST);
                    break;
            }
#else
            UInt64 seen = __atomic_load_n(word, __ATOMIC_RELAXED);
            do
            {
                old = TO_HOST64(seen);
            } while(!__atomic_compare_exchange_n(word, &seen, TO_HOST64(emex64_atomic_apply(op, old, v)), true, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED));
#endif /* __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ */
            emex64_atomic_written(core, word, sizeof(UInt64));
            *(core->op.param[0]) = old;
            return;
        }
    }
    else
    {
        UInt32 *word = emex64_atomic_word32(core, addr);
        if(likely(word != NULL))
        {
            UInt32 v32 = (UInt32)v;
            UInt32 old32;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            switch(op)
            {
                case kE64AtomicAdd:
                    old32 = __atomic_fetch_add(word, v32, __ATOMIC_SEQ_CST);
                    break;
                case kE64AtomicAnd:
                    old32 = __atomic_fetch_and(word, v32, __ATOMIC_SEQ_CST);
                    break;
                case kE64AtomicOr:
                    old32 = __atomic_fetch_or(word, v32, __ATOMIC_SEQ_CST);
                    break;
                default:
                    old32 = __atomic_fetch_xor(word, v32, __ATOMIC_SEQ_CST);
                    break;
            }
#else
            UInt32 seen = __atomic_load_n(word, __ATOMIC_RELAXED);
            do
            {
                old32 = TO_HOST32(seen);
            } while(!__atomic_compare_exchange_n(word, &seen, TO_HOST32((UInt32)emex64_atomic_apply(op, old32, v32)), true, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED));
#endif /* __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ */
            emex64_atomic_written(core, word, sizeof(UInt32));
            *(core->op.param[0]) = old32;
            return;
        }
    }

    old = 0;
    E64MemoryCoreAction(core->machine->memory, core, addr, size, &old, kE64MemoryActionTypeRead);
    if(core->cr_state.crexc.exception != kE64ExceptionNone)
    {
        return;
    }
    UInt64 value = emex64_atomic_apply(op, old, v);
    E64MemoryCoreAction(core->machine->memory, core, addr, size, &value, kE64MemoryActionTypeWrite);
    if(core->cr_state.crexc.exception != kE64ExceptionNone)
    {
        return;
    }
    *(core->op.param[0]) = old;
}

void emex64_op_ldadd(E64CoreRef core)
{
    emex64_atomic_rmw(core, kE64AtomicAdd, 2, sizeof(UInt64));
}

void emex64_op_ldand(E64CoreRef core)
{
    emex64_atomic_rmw(core, kE64AtomicAnd, 2, sizeof(UInt64));
}

void emex64_op_ldor(E64CoreRef core)
{
    emex64_atomic_rmw(core, kE64AtomicOr, 2, sizeof(UInt64));
}

void emex64_op_ldxor(E64CoreRef core)
{
    emex64_atomic_rmw(core, kE64AtomicXor, 2, sizeof(UInt64));
}

void emex64_op_swpadd(E64CoreRef core)
{
    emex64_atomic_rmw(core, kE64AtomicAdd, 0, sizeof(UInt64));
}

void emex64_op_ldaddd(E64CoreRef core)
{
    emex64_atomic_rmw(core, kE64AtomicAdd, 2, sizeof(UInt32));
}

void emex64_op_ldandd(E64CoreRef core)
{
    emex64_atomic_rmw(core, kE64AtomicAnd, 2, sizeof(UInt32));
}

void emex64_op_ldord(E64CoreRef core)
{
    emex64_atomic_rmw(core, kE64AtomicOr, 2, sizeof(UInt32));
}

void emex64_op_ldxord(E64CoreRef core)
{
    emex64_atomic_rmw(core, kE64AtomicXor, 2, sizeof(UInt32));
}

void emex64_op_swpaddd(E64CoreRef core)
{
    emex64_atomic_rmw(core, kE64AtomicAdd, 0, sizeof(UInt32));
}
