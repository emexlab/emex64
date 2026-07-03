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

#include <emex64lib/vm/instruction/instruction.h>
#include <emex64lib/vm/instruction/ctrl.h>
#include <emex64lib/vm/machine.h>
#include <stdio.h>

static inline uint64_t emex64_branch_pc(uint64_t pc, uint64_t v, kEmex64ParameterCoding coding)
{
    switch(coding)
    {
        case kEmex64ParameterCodingImm5:
            return pc + ((int8_t)(v << 3) >> 3);
        case kEmex64ParameterCodingImm8:
            return pc + (int8_t)v;
        case kEmex64ParameterCodingImm16:
            return pc + (int16_t)v;
        case kEmex64ParameterCodingImm32:
            return pc + (int32_t)v;
        default:
            return v;
    }
}

void emex64_op_b(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);
    core->op.ilen = 0;
    core->rl[kEmex64RegisterPC] = emex64_branch_pc(core->rl[kEmex64RegisterPC], *(core->op.param[0]), core->op.param_coding[0]);
}

void emex64_op_cmp(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    int64_t a = (int64_t)*(core->op.param[0]);
    int64_t b = (int64_t)*(core->op.param[1]);
    
    core->rl[kEmex64RegisterCF] = (a == b) * kEmex64CompareFlagZ | (a <  b) * kEmex64CompareFlagL | (a >  b) * kEmex64CompareFlagG;
}

void emex64_op_be(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[kEmex64RegisterCF] & kEmex64CompareFlagZ)
    {
        emex64_op_b(core);
    }
}

void emex64_op_bne(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);
    
    if(!(core->rl[kEmex64RegisterCF] & kEmex64CompareFlagZ))
    {
        emex64_op_b(core);
    }
}

void emex64_op_blt(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[kEmex64RegisterCF] & kEmex64CompareFlagL)
    {
        emex64_op_b(core);
    }
}

void emex64_op_bgt(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[kEmex64RegisterCF] & kEmex64CompareFlagG)
    {
        emex64_op_b(core);
    }
}

void emex64_op_ble(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[kEmex64RegisterCF] & (kEmex64CompareFlagL | kEmex64CompareFlagZ))
    {
        emex64_op_b(core);
    }
}

void emex64_op_bge(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[kEmex64RegisterCF] & (kEmex64CompareFlagG | kEmex64CompareFlagZ))
    {
        emex64_op_b(core);
    }
}

void emex64_op_bz(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);
    
    if(*(core->op.param[0]) == 0)
    {
        core->op.ilen = 0;
        core->rl[kEmex64RegisterPC] = emex64_branch_pc(core->rl[kEmex64RegisterPC], *(core->op.param[1]), core->op.param_coding[1]);
    }
}

void emex64_op_bnz(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);
    
    if(*(core->op.param[0]) != 0)
    {
        core->op.ilen = 0;
        core->rl[kEmex64RegisterPC] = emex64_branch_pc(core->rl[kEmex64RegisterPC], *(core->op.param[1]), core->op.param_coding[1]);
    }
}

/* call convention not needed, emex64 supports arguments directly in bl (biggest L ever, we gotta make a call convention) */
void emex64_op_blw(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt < 1);

    /* backup all parameters from pointer to not malform double passed registers for example */
    uint64_t param_imm[32] = {};
    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        param_imm[i] = *(core->op.param[i]);
    }

    /* pushing all relevant registers onto stack */
    emex64_push_il(core, core->rl[kEmex64RegisterPC] + core->op.ilen);
    emex64_push_il(core, core->rl[kEmex64RegisterFP]);
    emex64_push_il(core, core->rl[kEmex64RegisterCF]);
    emex64_push_il(core, core->rl[kEmex64RegisterFPC]);
    emex64_push_il(core, core->rl[kEmex64RegisterR0]);
    emex64_push_il(core, core->rl[kEmex64RegisterR1]);
    emex64_push_il(core, core->rl[kEmex64RegisterR2]);
    emex64_push_il(core, core->rl[kEmex64RegisterR3]);
    emex64_push_il(core, core->rl[kEmex64RegisterR4]);
    emex64_push_il(core, core->rl[kEmex64RegisterR5]);
    emex64_push_il(core, core->rl[kEmex64RegisterR6]);
    emex64_push_il(core, core->rl[kEmex64RegisterR7]);
    emex64_push_il(core, core->rl[kEmex64RegisterR8]);
    emex64_push_il(core, core->rl[kEmex64RegisterR9]);
    emex64_push_il(core, core->rl[kEmex64RegisterR10]);
    emex64_push_il(core, core->rl[kEmex64RegisterR11]);
    emex64_push_il(core, core->rl[kEmex64RegisterR12]);
    emex64_push_il(core, core->rl[kEmex64RegisterR13]);
    emex64_push_il(core, core->rl[kEmex64RegisterR14]);
    emex64_push_il(core, core->rl[kEmex64RegisterR15]);
    emex64_push_il(core, core->rl[kEmex64RegisterR16]);
    emex64_push_il(core, core->rl[kEmex64RegisterR17]);
    emex64_push_il(core, core->rl[kEmex64RegisterR18]);
    emex64_push_il(core, core->rl[kEmex64RegisterR19]);
    emex64_push_il(core, core->rl[kEmex64RegisterR20]);
    emex64_push_il(core, core->rl[kEmex64RegisterR21]);
    emex64_push_il(core, core->rl[kEmex64RegisterR22]);
    emex64_push_il(core, core->rl[kEmex64RegisterR23]);
    emex64_push_il(core, core->rl[kEmex64RegisterR24]);
    emex64_push_il(core, core->rl[kEmex64RegisterR25]);

    /* writing parameters */
    for(uint8_t i = 1; i < core->op.param_cnt && i < (kEmex64RegisterR25 - 1); i++)
    {
        core->rl[(kEmex64RegisterR0 - 1) + i] = param_imm[i];
    }

    /* setting current frame pointer to stack pointer to point to stack frame */
    core->rl[kEmex64RegisterFP] = core->rl[kEmex64RegisterSP];

    /* initiating jump */
    core->op.ilen = 0;
    core->rl[kEmex64RegisterPC] = emex64_branch_pc(core->rl[kEmex64RegisterPC], param_imm[0], core->op.param_coding[0]);
}

void emex64_op_wret(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 0);

    core->rl[kEmex64RegisterSP] = core->rl[kEmex64RegisterFP];

    core->rl[kEmex64RegisterR25] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR24] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR23] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR22] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR21] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR20] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR19] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR18] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR17] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR16] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR15] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR14] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR13] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR12] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR11] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR10] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR9] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR8] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR7] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR6] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR5] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR4] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR3] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR2] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR1] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR0] = emex64_pop_il(core);
    core->rl[kEmex64RegisterFPC] = emex64_pop_il(core);
    core->rl[kEmex64RegisterCF] = emex64_pop_il(core);
    core->rl[kEmex64RegisterFP] = emex64_pop_il(core);
    core->rl[kEmex64RegisterPC] = emex64_pop_il(core);
    core->op.ilen = 0;
}

void emex64_op_iret(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 0);

    if(!core->in_interrupt)
    {
        core->cr_state.crexc.exception = kEmex64ExceptionBadInstruction;
        return;
    }

    core->rl[kEmex64RegisterSP] = core->rl[kEmex64RegisterFP];

    core->rl[kEmex64RegisterR25] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR24] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR23] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR22] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR21] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR20] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR19] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR18] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR17] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR16] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR15] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR14] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR13] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR12] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR11] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR10] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR9] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR8] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR7] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR6] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR5] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR4] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR3] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR2] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR1] = emex64_pop_il(core);
    core->rl[kEmex64RegisterR0] = emex64_pop_il(core);
    core->rl[kEmex64RegisterFPC] = emex64_pop_il(core);
    core->rl[kEmex64RegisterCF] = emex64_pop_il(core);
    core->rl[kEmex64RegisterFP] = emex64_pop_il(core);
    uint64_t oldsp = emex64_pop_il(core);
    core->rl[kEmex64RegisterPC] = emex64_pop_il(core);
    core->cr_state.crel.level = emex64_pop_il(core);
    core->op.ilen = 0;

    core->rl[kEmex64RegisterSP] = oldsp;

    core->machine->intc->current_irq = -1;
    core->in_interrupt = false;
    core->halted = false;
}

void emex64_op_bl(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt < 1);

    /* pushing all relevant registers onto stack */
    emex64_push_il(core, core->rl[kEmex64RegisterPC] + core->op.ilen);
    emex64_push_il(core, core->rl[kEmex64RegisterFP]);

    /* setting current frame pointer to stack pointer to point to stack frame */
    core->rl[kEmex64RegisterFP] = core->rl[kEmex64RegisterSP];

    /* initiating jump */
    core->op.ilen = 0;
    core->rl[kEmex64RegisterPC] = emex64_branch_pc(core->rl[kEmex64RegisterPC], *(core->op.param[0]), core->op.param_coding[0]);
}

void emex64_op_ret(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 0);

    core->rl[kEmex64RegisterSP] = core->rl[kEmex64RegisterFP];

    core->rl[kEmex64RegisterFP] = emex64_pop_il(core);
    core->rl[kEmex64RegisterPC] = emex64_pop_il(core);
    core->op.ilen = 0;
}
