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
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/E64Core.h>
#include <EmexToolchain/VM/E64Memory.h>
#include <EmexToolchain/VM/__E64FastMem.h>

void emex64_op_b(__E64Core core);
void emex64_op_cmp(__E64Core core);
void emex64_op_be(__E64Core core);
void emex64_op_bne(__E64Core core);
void emex64_op_blt(__E64Core core);
void emex64_op_bgt(__E64Core core);
void emex64_op_ble(__E64Core core);
void emex64_op_bge(__E64Core core);
void emex64_op_bz(__E64Core core);
void emex64_op_bnz(__E64Core core);

void emex64_op_blw(__E64Core core);
void emex64_op_wret(__E64Core core);
void emex64_op_iret(__E64Core core);

void emex64_op_bl(__E64Core core);
void emex64_op_ret(__E64Core core);

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
        E64MemoryCoreAction(core->machine->memory, core, core->rl[kE64RegisterSP], sizeof(UInt64), &value, kE64MemoryActionTypeWrite);
    }
    core->rl[kE64RegisterSP] -= 8;
}

static inline UInt64 emex64_pop_il(__E64Core core)
{
    core->rl[kE64RegisterSP] += 8;

    UInt64 value = 0;
    if(unlikely(!emex64_fast_load(core, core->rl[kE64RegisterSP], sizeof(UInt64), &value)))
    {
        E64MemoryCoreAction(core->machine->memory, core, core->rl[kE64RegisterSP], sizeof(UInt64), &value, kE64MemoryActionTypeRead);
    }
    return value;
}

void emex64_op_bbz(__E64Core core);
void emex64_op_bbnz(__E64Core core);
void emex64_op_cblt(__E64Core core);

void emex64_op_cbeq(__E64Core core);
void emex64_op_cbne(__E64Core core);
void emex64_op_cbge(__E64Core core);
void emex64_op_cbltu(__E64Core core);
void emex64_op_cbgeu(__E64Core core);
void emex64_op_wfe(__E64Core core);
void emex64_op_sev(__E64Core core);
void emex64_op_pause(__E64Core core);
void emex64_op_tlbi(__E64Core core);

/* pointer authentication extension */
void emex64_op_pacia(__E64Core core);
void emex64_op_autia(__E64Core core);
void emex64_op_pacda(__E64Core core);
void emex64_op_autda(__E64Core core);
void emex64_op_xpac(__E64Core core);
void emex64_op_pacret(__E64Core core);
void emex64_op_reta(__E64Core core);

void emex64_op_svc(__E64Core core);
void emex64_op_eret(__E64Core core);

#endif /* EMEX64VM_INSTRUCTION_CTRL_H */
