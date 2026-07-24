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

#include <EmexToolchain/VM/instruction/instruction.h>
#include <EmexToolchain/VM/instruction/ctrl.h>
#include <EmexToolchain/VM/E64Machine.h>

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

void emex64_op_b(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);
    core->op.ilen = 0;
    core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[0]), core->op.param_coding[0]);
}

void emex64_op_cmp(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    SInt64 a = (SInt64)*(core->op.param[0]);
    SInt64 b = (SInt64)*(core->op.param[1]);

    core->rl[kE64RegisterCF] = (a == b) * kE64CompareFlagZ | (a <  b) * kE64CompareFlagL | (a >  b) * kE64CompareFlagG;
}

void emex64_op_be(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);

    if(core->rl[kE64RegisterCF] & kE64CompareFlagZ)
    {
        emex64_op_b(core);
    }
}

void emex64_op_bne(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);

    if(!(core->rl[kE64RegisterCF] & kE64CompareFlagZ))
    {
        emex64_op_b(core);
    }
}

void emex64_op_blt(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);

    if(core->rl[kE64RegisterCF] & kE64CompareFlagL)
    {
        emex64_op_b(core);
    }
}

void emex64_op_bgt(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);

    if(core->rl[kE64RegisterCF] & kE64CompareFlagG)
    {
        emex64_op_b(core);
    }
}

void emex64_op_ble(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);

    if(core->rl[kE64RegisterCF] & (kE64CompareFlagL | kE64CompareFlagZ))
    {
        emex64_op_b(core);
    }
}

void emex64_op_bge(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);

    if(core->rl[kE64RegisterCF] & (kE64CompareFlagG | kE64CompareFlagZ))
    {
        emex64_op_b(core);
    }
}

void emex64_op_bz(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    if(*(core->op.param[0]) == 0)
    {
        core->op.ilen = 0;
        core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[1]), core->op.param_coding[1]);
    }
}

void emex64_op_bnz(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    if(*(core->op.param[0]) != 0)
    {
        core->op.ilen = 0;
        core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[1]), core->op.param_coding[1]);
    }
}

/* call convention not needed, emex64 supports arguments directly in bl (biggest L ever, we gotta make a call convention) */
void emex64_op_blw(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt < 1);

    /* backup all parameters from pointer to not malform double passed registers for example */
    UInt64 param_imm[32] = {};
    for(UInt8 i = 0; i < core->op.param_cnt; i++)
    {
        param_imm[i] = *(core->op.param[i]);
    }

    /* pushing all relevant registers onto stack */
    emex64_push_il(core, core->rl[kE64RegisterPC] + core->op.ilen);
    emex64_push_il(core, core->rl[kE64RegisterFP]);
    emex64_push_il(core, core->rl[kE64RegisterCF]);
    emex64_push_il(core, core->rl[kE64RegisterFPC]);
    emex64_push_il(core, core->rl[kE64RegisterR0]);
    emex64_push_il(core, core->rl[kE64RegisterR1]);
    emex64_push_il(core, core->rl[kE64RegisterR2]);
    emex64_push_il(core, core->rl[kE64RegisterR3]);
    emex64_push_il(core, core->rl[kE64RegisterR4]);
    emex64_push_il(core, core->rl[kE64RegisterR5]);
    emex64_push_il(core, core->rl[kE64RegisterR6]);
    emex64_push_il(core, core->rl[kE64RegisterR7]);
    emex64_push_il(core, core->rl[kE64RegisterR8]);
    emex64_push_il(core, core->rl[kE64RegisterR9]);

    /* writing parameters */
    for(UInt8 i = 1; i < core->op.param_cnt && i < (kE64RegisterR9 - 1); i++)
    {
        core->rl[(kE64RegisterR0 - 1) + i] = param_imm[i];
    }

    /* setting current frame pointer to stack pointer to point to stack frame */
    core->rl[kE64RegisterFP] = core->rl[kE64RegisterSP];

    /* initiating jump */
    core->op.ilen = 0;
    core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], param_imm[0], core->op.param_coding[0]);
}

void emex64_op_wret(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 0);

    core->rl[kE64RegisterSP] = core->rl[kE64RegisterFP];

    core->rl[kE64RegisterR9] = emex64_pop_il(core);
    core->rl[kE64RegisterR8] = emex64_pop_il(core);
    core->rl[kE64RegisterR7] = emex64_pop_il(core);
    core->rl[kE64RegisterR6] = emex64_pop_il(core);
    core->rl[kE64RegisterR5] = emex64_pop_il(core);
    core->rl[kE64RegisterR4] = emex64_pop_il(core);
    core->rl[kE64RegisterR3] = emex64_pop_il(core);
    core->rl[kE64RegisterR2] = emex64_pop_il(core);
    core->rl[kE64RegisterR1] = emex64_pop_il(core);
    core->rl[kE64RegisterR0] = emex64_pop_il(core);
    core->rl[kE64RegisterFPC] = emex64_pop_il(core);
    core->rl[kE64RegisterCF] = emex64_pop_il(core);
    core->rl[kE64RegisterFP] = emex64_pop_il(core);
    core->rl[kE64RegisterPC] = emex64_pop_il(core);
    core->op.ilen = 0;
}

void emex64_op_iret(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 0);

    if(!core->in_interrupt)
    {
        core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
        return;
    }

    core->rl[kE64RegisterSP] = core->rl[kE64RegisterFP];

    core->rl[kE64RegisterR9] = emex64_pop_il(core);
    core->rl[kE64RegisterR8] = emex64_pop_il(core);
    core->rl[kE64RegisterR7] = emex64_pop_il(core);
    core->rl[kE64RegisterR6] = emex64_pop_il(core);
    core->rl[kE64RegisterR5] = emex64_pop_il(core);
    core->rl[kE64RegisterR4] = emex64_pop_il(core);
    core->rl[kE64RegisterR3] = emex64_pop_il(core);
    core->rl[kE64RegisterR2] = emex64_pop_il(core);
    core->rl[kE64RegisterR1] = emex64_pop_il(core);
    core->rl[kE64RegisterR0] = emex64_pop_il(core);
    core->rl[kE64RegisterFPC] = emex64_pop_il(core);
    core->rl[kE64RegisterCF] = emex64_pop_il(core);
    core->rl[kE64RegisterFP] = emex64_pop_il(core);
    UInt64 oldsp = emex64_pop_il(core);
    core->rl[kE64RegisterPC] = emex64_pop_il(core);
    core->cr_state.crel.level = emex64_pop_il(core);
    core->op.ilen = 0;

    core->rl[kE64RegisterSP] = oldsp;

    core->machine->intc->current_irq = -1;
    core->in_interrupt = false;
    core->halted = false;
}

void emex64_op_bl(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt < 1);

    /* pushing all relevant registers onto stack */
    emex64_push_il(core, core->rl[kE64RegisterPC] + core->op.ilen);
    emex64_push_il(core, core->rl[kE64RegisterFP]);

    /* setting current frame pointer to stack pointer to point to stack frame */
    core->rl[kE64RegisterFP] = core->rl[kE64RegisterSP];

    /* initiating jump */
    core->op.ilen = 0;
    core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[0]), core->op.param_coding[0]);
}

void emex64_op_ret(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 0);

    core->rl[kE64RegisterSP] = core->rl[kE64RegisterFP];

    core->rl[kE64RegisterFP] = emex64_pop_il(core);
    core->rl[kE64RegisterPC] = emex64_pop_il(core);
    core->op.ilen = 0;
}

void emex64_op_bbz(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 3);

    if((*(core->op.param[0]) & *(core->op.param[1])) == 0)
    {
        core->op.ilen = 0;
        core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[2]), core->op.param_coding[2]);
    }
}

void emex64_op_bbnz(__E64Core core)
{
    emex64_instr_termcond(core->op.param_cnt != 3);

    if((*(core->op.param[0]) & *(core->op.param[1])) != 0)
    {
        core->op.ilen = 0;
        core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[2]), core->op.param_coding[2]);
    }
}
