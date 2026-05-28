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

#include <emex64vm/instruction/instruction.h>
#include <emex64vm/instruction/ctrl.h>
#include <emex64vm/machine.h>
#include <emex64vm/memory.h>
#include <stdio.h>

void la64_op_b(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    core->op.ilen = 0;
    core->rl[LA64_REGISTER_PC] = *(core->op.param[0]);
}

void la64_op_cmp(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    int64_t a = (int64_t)*(core->op.param[0]);
    int64_t b = (int64_t)*(core->op.param[1]);
    
    core->rl[LA64_REGISTER_CF] = (a == b) * LA64_CMP_Z | (a <  b) * LA64_CMP_L | (a >  b) * LA64_CMP_G;
}

void la64_op_be(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[LA64_REGISTER_CF] & LA64_CMP_Z)
    {
        la64_op_b(core);
    }
}

void la64_op_bne(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(!(core->rl[LA64_REGISTER_CF] & LA64_CMP_Z))
    {
        la64_op_b(core);
    }
}

void la64_op_blt(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[LA64_REGISTER_CF] & LA64_CMP_L)
    {
        la64_op_b(core);
    }
}

void la64_op_bgt(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[LA64_REGISTER_CF] & LA64_CMP_G)
    {
        la64_op_b(core);
    }
}

void la64_op_ble(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[LA64_REGISTER_CF] & LA64_CMP_L || core->rl[LA64_REGISTER_CF] & LA64_CMP_Z)
    {
        la64_op_b(core);
    }
}

void la64_op_bge(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    
    if(core->rl[LA64_REGISTER_CF] & LA64_CMP_G || core->rl[LA64_REGISTER_CF] & LA64_CMP_Z)
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
        core->rl[LA64_REGISTER_PC] = *(core->op.param[1]);
    }
}

void la64_op_bnz(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);
    
    if(*(core->op.param[0]) != 0)
    {
        core->op.ilen = 0;
        core->rl[LA64_REGISTER_PC] = *(core->op.param[1]);
    }
}

void la64_push(la64_core_t *core, uint64_t value)
{
    if(!la64_memory_write(core, core->rl[LA64_REGISTER_SP], value, sizeof(uint64_t)))
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }

    core->rl[LA64_REGISTER_SP] -= 8;
}

uint64_t la64_pop(la64_core_t *core)
{
    core->rl[LA64_REGISTER_SP] += 8;

    uint64_t value = 0;

    if(!la64_memory_read(core, core->rl[LA64_REGISTER_SP], sizeof(uint64_t), &value))
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
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
    la64_push(core, core->rl[LA64_REGISTER_PC] + core->op.ilen);
    la64_push(core, core->rl[LA64_REGISTER_FP]);
    la64_push(core, core->rl[LA64_REGISTER_CF]);
    la64_push(core, core->rl[LA64_REGISTER_R0]);
    la64_push(core, core->rl[LA64_REGISTER_R1]);
    la64_push(core, core->rl[LA64_REGISTER_R2]);
    la64_push(core, core->rl[LA64_REGISTER_R3]);
    la64_push(core, core->rl[LA64_REGISTER_R4]);
    la64_push(core, core->rl[LA64_REGISTER_R5]);
    la64_push(core, core->rl[LA64_REGISTER_R6]);
    la64_push(core, core->rl[LA64_REGISTER_R7]);
    la64_push(core, core->rl[LA64_REGISTER_R8]);
    la64_push(core, core->rl[LA64_REGISTER_R9]);
    la64_push(core, core->rl[LA64_REGISTER_R10]);
    la64_push(core, core->rl[LA64_REGISTER_R11]);

    /* writing parameters */
    for(uint8_t i = 1; i < core->op.param_cnt && i < (LA64_REGISTER_R11 - 1); i++)
    {
        core->rl[(LA64_REGISTER_R0 - 1) + i] = param_imm[i];
    }

    /* setting current frame pointer to stack pointer to point to stack frame */
    core->rl[LA64_REGISTER_FP] = core->rl[LA64_REGISTER_SP];

    /* manipulating ilen */
    core->op.ilen = 0;

    /* jump! */
    core->rl[LA64_REGISTER_PC] = param_imm[0];
}

void la64_op_ret(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 0);

    core->rl[LA64_REGISTER_SP] = core->rl[LA64_REGISTER_FP];

    core->rl[LA64_REGISTER_R11] = la64_pop(core);
    core->rl[LA64_REGISTER_R10] = la64_pop(core);
    core->rl[LA64_REGISTER_R9] = la64_pop(core);
    core->rl[LA64_REGISTER_R8] = la64_pop(core);
    core->rl[LA64_REGISTER_R7] = la64_pop(core);
    core->rl[LA64_REGISTER_R6] = la64_pop(core);
    core->rl[LA64_REGISTER_R5] = la64_pop(core);
    core->rl[LA64_REGISTER_R4] = la64_pop(core);
    core->rl[LA64_REGISTER_R3] = la64_pop(core);
    core->rl[LA64_REGISTER_R2] = la64_pop(core);
    core->rl[LA64_REGISTER_R1] = la64_pop(core);
    core->rl[LA64_REGISTER_R0] = la64_pop(core);
    core->rl[LA64_REGISTER_CF] = la64_pop(core);
    core->rl[LA64_REGISTER_FP] = la64_pop(core);
    core->rl[LA64_REGISTER_PC] = la64_pop(core);
    core->op.ilen = 0;
}

void la64_op_iret(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 0);

    if(!core->in_interrupt)
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_INSTRUCTION;
        return;
    }

    core->rl[LA64_REGISTER_SP] = core->rl[LA64_REGISTER_FP];

    core->rl[LA64_REGISTER_R11] = la64_pop(core);
    core->rl[LA64_REGISTER_R10] = la64_pop(core);
    core->rl[LA64_REGISTER_R9] = la64_pop(core);
    core->rl[LA64_REGISTER_R8] = la64_pop(core);
    core->rl[LA64_REGISTER_R7] = la64_pop(core);
    core->rl[LA64_REGISTER_R6] = la64_pop(core);
    core->rl[LA64_REGISTER_R5] = la64_pop(core);
    core->rl[LA64_REGISTER_R4] = la64_pop(core);
    core->rl[LA64_REGISTER_R3] = la64_pop(core);
    core->rl[LA64_REGISTER_R2] = la64_pop(core);
    core->rl[LA64_REGISTER_R1] = la64_pop(core);
    core->rl[LA64_REGISTER_R0] = la64_pop(core);
    core->rl[LA64_REGISTER_CF] = la64_pop(core);
    core->rl[LA64_REGISTER_FP] = la64_pop(core);
    uint64_t oldsp = la64_pop(core);
    core->rl[LA64_REGISTER_PC] = la64_pop(core);
    core->rl[LA64_REGISTER_CR0] = la64_pop(core);
    core->op.ilen = 0;

    core->rl[LA64_REGISTER_SP] = oldsp;

    core->machine->intc->current_irq = -1;
    core->in_interrupt = false;
    core->halted = false;
}
