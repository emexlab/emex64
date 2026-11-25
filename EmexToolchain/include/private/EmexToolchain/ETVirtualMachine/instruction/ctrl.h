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

#ifndef EMEX64VM_INSTRUCTION_CTRL_H
#define EMEX64VM_INSTRUCTION_CTRL_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETVirtualMachine/E64Machine.h>
#include <EmexToolchain/ETVirtualMachine/E64Core.h>
#include <EmexToolchain/ETVirtualMachine/E64Memory.h>
#include <EmexToolchain/ETVirtualMachine/__E64FastMem.h>

#define EMEX64_CALL_ARGS 6

void E64B(__E64Core core);
void E64CMP(__E64Core core);
void E64BE(__E64Core core);
void E64BNE(__E64Core core);
void E64BLT(__E64Core core);
void E64BGT(__E64Core core);
void E64BLE(__E64Core core);
void E64BGE(__E64Core core);
void E64BZ(__E64Core core);
void E64BNZ(__E64Core core);

void E64BLW(__E64Core core);
void E64WRET(__E64Core core);
void E64IRET(__E64Core core);

void E64BL(__E64Core core);
void E64RET(__E64Core core);
void E64BLA(__E64Core core);

static inline __attribute__((always_inline)) Boolean emex64_hot_call(__E64Core core,
                                                                     UInt64 ctx,
                                                                     UInt64 ret,
                                                                     Boolean *touched)
{
    UInt64 sp = core->rl[kE64RegisterSP];
    UInt64 pa;
    if(unlikely(!emex64_hot_paddr(core, ctx, sp - 8, 16, kE64MemoryActionTypeWrite, &pa)))
    {
        return false;
    }

    UInt64 frame[2] = { core->rl[kE64RegisterFP], ret };
    memcpy(core->ram + pa, frame, sizeof(frame));
    *touched = emex64_snoop_write(core->snoop, pa, sizeof(frame));
    core->rl[kE64RegisterSP] = sp - 16;
    core->rl[kE64RegisterFP] = sp - 16;
    return true;
}

static inline __attribute__((always_inline)) Boolean emex64_hot_frame(__E64Core core,
                                                                      UInt64 ctx,
                                                                      UInt64 frame,
                                                                      UInt64 *fp,
                                                                      UInt64 *ret)
{
    UInt64 pa;
    if(unlikely(!emex64_hot_paddr(core, ctx, frame + 8, 16, kE64MemoryActionTypeRead, &pa)))
    {
        return false;
    }
    memcpy(fp, core->ram + pa, 8);
    memcpy(ret, core->ram + pa + 8, 8);
    return true;
}

Boolean emex64_fast_reta(__E64Core core, UInt64 frame, UInt64 ret, UInt64 *pc);
Boolean emex64_fast_pacret(__E64Core core, Boolean *touched);

static inline UInt64 emex64_branch_pc(UInt64 pc,
                                      UInt64 v,
                                      E64ParameterCoding coding)
{
    switch(coding)
    {
        case kE64ParameterCodingImm4:
            return pc + ((SInt8)(v << 4) >> 4);
        case kE64ParameterCodingImm8:
            return pc + (SInt8)v;
        case kE64ParameterCodingImm16:
            return pc + (SInt16)v;
        case kE64ParameterCodingImm32:
            return pc + (SInt32)v;
        default:
            return v;
    }
}

static inline void emex64_push_il(__E64Core core, UInt64 value)
{
    if(unlikely(!emex64_fast_store(core, core->rl[kE64RegisterSP], sizeof(UInt64), value)))
    {
        __E64MemoryCoreAction(core->machine->memory, core, core->rl[kE64RegisterSP], sizeof(UInt64), &value, kE64MemoryActionTypeWrite);
    }
    core->rl[kE64RegisterSP] -= 8;
}

static inline UInt64 emex64_pop_il(__E64Core core)
{
    core->rl[kE64RegisterSP] += 8;

    UInt64 value = 0;
    if(unlikely(!emex64_fast_load(core, core->rl[kE64RegisterSP], sizeof(UInt64), &value)))
    {
        __E64MemoryCoreAction(core->machine->memory, core, core->rl[kE64RegisterSP], sizeof(UInt64), &value, kE64MemoryActionTypeRead);
    }
    return value;
}

void E64BBZ(__E64Core core);
void E64BBNZ(__E64Core core);
void E64CBLT(__E64Core core);

void E64CBEQ(__E64Core core);
void E64CBNE(__E64Core core);
void E64CBGE(__E64Core core);
void E64CBLTU(__E64Core core);
void E64CBGEU(__E64Core core);
void E64WFE(__E64Core core);
void E64SEV(__E64Core core);
void E64PAUSE(__E64Core core);
void E64TLBI(__E64Core core);

/* pointer authentication extension */
void E64PACIA(__E64Core core);
void E64AUTIA(__E64Core core);
void E64PACDA(__E64Core core);
void E64AUTDA(__E64Core core);
void E64XPAC(__E64Core core);
void E64PACRET(__E64Core core);
void E64RETA(__E64Core core);

void E64SVC(__E64Core core);
void E64ERET(__E64Core core);

#endif /* EMEX64VM_INSTRUCTION_CTRL_H */
