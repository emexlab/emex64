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

#include <emex64lib/vm/instruction/instruction.h>
#include <emex64lib/vm/instruction/ctrl.h>
#include <emex64lib/vm/machine.h>
#include <emex64lib/vm/memory.h>
#include <stdio.h>

void la64_op_b(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    core->op.ilen = 0;
    core->rl[kEmex64RegisterPC] = *(core->op.param[0]);
}

void la64_op_cmp(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    int64_t a = (int64_t)*(core->op.param[0]);
    int64_t b = (int64_t)*(core->op.param[1]);
    
    core->rl[kEmex64RegisterCF] = (a == b) * kEmex64CompareFlagZ | (a <  b) * kEmex64CompareFlagL | (a >  b) * kEmex64CompareFlagG;
}

void la64_op_be(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[kEmex64RegisterCF] & kEmex64CompareFlagZ)
    {
        la64_op_b(core);
    }
}

void la64_op_bne(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(!(core->rl[kEmex64RegisterCF] & kEmex64CompareFlagZ))
    {
        la64_op_b(core);
    }
}

void la64_op_blt(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[kEmex64RegisterCF] & kEmex64CompareFlagL)
    {
        la64_op_b(core);
    }
}

void la64_op_bgt(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[kEmex64RegisterCF] & kEmex64CompareFlagG)
    {
        la64_op_b(core);
    }
}

void la64_op_ble(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[kEmex64RegisterCF] & (kEmex64CompareFlagL | kEmex64CompareFlagZ))
    {
        la64_op_b(core);
    }
}

void la64_op_bge(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[kEmex64RegisterCF] & (kEmex64CompareFlagG | kEmex64CompareFlagZ))
    {
        la64_op_b(core);
    }
}

void la64_op_bz(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);
    
    if(*(core->op.param[0]) == 0)
    {
        core->op.ilen = 0;
        core->rl[kEmex64RegisterPC] = *(core->op.param[1]);
    }
}

void la64_op_bnz(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);
    
    if(*(core->op.param[0]) != 0)
    {
        core->op.ilen = 0;
        core->rl[kEmex64RegisterPC] = *(core->op.param[1]);
    }
}

void la64_push(la64_core_t *core, uint64_t value)
{
    if(!la64_memory_write(core, core->rl[kEmex64RegisterSP], value, sizeof(uint64_t)))
    {
        core->rl[kEmex64RegisterCR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }

    core->rl[kEmex64RegisterSP] -= 8;
}

uint64_t la64_pop(la64_core_t *core)
{
    core->rl[kEmex64RegisterSP] += 8;

    uint64_t value = 0;

    if(!la64_memory_read(core, core->rl[kEmex64RegisterSP], sizeof(uint64_t), &value))
    {
        core->rl[kEmex64RegisterCR2] = LA64_EXCEPTION_BAD_ACCESS;
        return 0;
    }

    return value;
}

/* call convention not needed, la64 supports arguments directly in bl (biggest win ever) */
void la64_op_bl(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt < 1);

    /* backup all parameters from pointer to not malform double passed registers for example */
    uint64_t param_imm[32] = {};
    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        param_imm[i] = *(core->op.param[i]);
    }

    /* pushing all relevant registers onto stack */
    la64_push(core, core->rl[kEmex64RegisterPC] + core->op.ilen);
    la64_push(core, core->rl[kEmex64RegisterFP]);
    la64_push(core, core->rl[kEmex64RegisterCF]);
    la64_push(core, core->rl[kEmex64RegisterR0]);
    la64_push(core, core->rl[kEmex64RegisterR1]);
    la64_push(core, core->rl[kEmex64RegisterR2]);
    la64_push(core, core->rl[kEmex64RegisterR3]);
    la64_push(core, core->rl[kEmex64RegisterR4]);
    la64_push(core, core->rl[kEmex64RegisterR5]);
    la64_push(core, core->rl[kEmex64RegisterR6]);
    la64_push(core, core->rl[kEmex64RegisterR7]);
    la64_push(core, core->rl[kEmex64RegisterR8]);
    la64_push(core, core->rl[kEmex64RegisterR9]);
    la64_push(core, core->rl[kEmex64RegisterR10]);
    la64_push(core, core->rl[kEmex64RegisterR11]);
    la64_push(core, core->rl[kEmex64RegisterR12]);
    la64_push(core, core->rl[kEmex64RegisterR13]);
    la64_push(core, core->rl[kEmex64RegisterR14]);
    la64_push(core, core->rl[kEmex64RegisterR15]);
    la64_push(core, core->rl[kEmex64RegisterR16]);

    /* writing parameters */
    for(uint8_t i = 1; i < core->op.param_cnt && i < (kEmex64RegisterR16 - 1); i++)
    {
        core->rl[(kEmex64RegisterR0 - 1) + i] = param_imm[i];
    }

    /* setting current frame pointer to stack pointer to point to stack frame */
    core->rl[kEmex64RegisterFP] = core->rl[kEmex64RegisterSP];

    /* manipulating ilen */
    core->op.ilen = 0;

    /* jump! */
    core->rl[kEmex64RegisterPC] = param_imm[0];
}

void la64_op_ret(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 0);

    core->rl[kEmex64RegisterSP] = core->rl[kEmex64RegisterFP];

    core->rl[kEmex64RegisterR16] = la64_pop(core);
    core->rl[kEmex64RegisterR15] = la64_pop(core);
    core->rl[kEmex64RegisterR14] = la64_pop(core);
    core->rl[kEmex64RegisterR13] = la64_pop(core);
    core->rl[kEmex64RegisterR12] = la64_pop(core);
    core->rl[kEmex64RegisterR11] = la64_pop(core);
    core->rl[kEmex64RegisterR10] = la64_pop(core);
    core->rl[kEmex64RegisterR9] = la64_pop(core);
    core->rl[kEmex64RegisterR8] = la64_pop(core);
    core->rl[kEmex64RegisterR7] = la64_pop(core);
    core->rl[kEmex64RegisterR6] = la64_pop(core);
    core->rl[kEmex64RegisterR5] = la64_pop(core);
    core->rl[kEmex64RegisterR4] = la64_pop(core);
    core->rl[kEmex64RegisterR3] = la64_pop(core);
    core->rl[kEmex64RegisterR2] = la64_pop(core);
    core->rl[kEmex64RegisterR1] = la64_pop(core);
    core->rl[kEmex64RegisterR0] = la64_pop(core);
    core->rl[kEmex64RegisterCF] = la64_pop(core);
    core->rl[kEmex64RegisterFP] = la64_pop(core);
    core->rl[kEmex64RegisterPC] = la64_pop(core);
    core->op.ilen = 0;
}

void la64_op_iret(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 0);

    if(!core->in_interrupt)
    {
        core->rl[kEmex64RegisterCR2] = LA64_EXCEPTION_BAD_INSTRUCTION;
        return;
    }

    core->rl[kEmex64RegisterSP] = core->rl[kEmex64RegisterFP];

    core->rl[kEmex64RegisterR16] = la64_pop(core);
    core->rl[kEmex64RegisterR15] = la64_pop(core);
    core->rl[kEmex64RegisterR14] = la64_pop(core);
    core->rl[kEmex64RegisterR13] = la64_pop(core);
    core->rl[kEmex64RegisterR12] = la64_pop(core);
    core->rl[kEmex64RegisterR11] = la64_pop(core);
    core->rl[kEmex64RegisterR10] = la64_pop(core);
    core->rl[kEmex64RegisterR9] = la64_pop(core);
    core->rl[kEmex64RegisterR8] = la64_pop(core);
    core->rl[kEmex64RegisterR7] = la64_pop(core);
    core->rl[kEmex64RegisterR6] = la64_pop(core);
    core->rl[kEmex64RegisterR5] = la64_pop(core);
    core->rl[kEmex64RegisterR4] = la64_pop(core);
    core->rl[kEmex64RegisterR3] = la64_pop(core);
    core->rl[kEmex64RegisterR2] = la64_pop(core);
    core->rl[kEmex64RegisterR1] = la64_pop(core);
    core->rl[kEmex64RegisterR0] = la64_pop(core);
    core->rl[kEmex64RegisterCF] = la64_pop(core);
    core->rl[kEmex64RegisterFP] = la64_pop(core);
    uint64_t oldsp = la64_pop(core);
    core->rl[kEmex64RegisterPC] = la64_pop(core);
    core->rl[kEmex64RegisterCR0] = la64_pop(core);
    core->op.ilen = 0;

    core->rl[kEmex64RegisterSP] = oldsp;

    core->machine->intc->current_irq = -1;
    core->in_interrupt = false;
    core->halted = false;
}
