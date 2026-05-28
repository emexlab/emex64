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
#include <emex64vm/instruction/data.h>
#include <emex64vm/machine.h>
#include <emex64vm/memory.h>

void la64_op_mov(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    *(core->op.param[0]) = *(core->op.param[1]);
}

void la64_op_swp(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    uint64_t param_backup = *(core->op.param[0]);
    *(core->op.param[0]) = *(core->op.param[1]);
    *(core->op.param[1]) = param_backup;
}

void la64_op_swpz(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    *(core->op.param[0]) = *(core->op.param[1]);
    *(core->op.param[1]) = 0;
}

void la64_op_push(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt == 0);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        if(!la64_memory_write(core, core->rl[LA64_REGISTER_SP], *(core->op.param[i]), sizeof(uint64_t)))
        {
            core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
            return;
        }

        core->rl[LA64_REGISTER_SP] -= 8;
    }
}

void la64_op_pop(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt == 0);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        core->rl[LA64_REGISTER_SP] += 8;

        if(!la64_memory_read(core, core->rl[LA64_REGISTER_SP], sizeof(uint64_t), core->op.param[i]))
        {
            core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
            return;
        }
    }
}

void la64_op_ldb(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    if(!la64_memory_read(core, *(core->op.param[1]), sizeof(uint8_t), core->op.param[0]))
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }
}

void la64_op_ldw(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    if(!la64_memory_read(core, *(core->op.param[1]), sizeof(uint16_t), core->op.param[0]))
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }
}

void la64_op_ldd(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    if(!la64_memory_read(core, *(core->op.param[1]), sizeof(uint32_t), core->op.param[0]))
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }
}

void la64_op_ldq(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    if(!la64_memory_read(core, *(core->op.param[1]), sizeof(uint64_t), core->op.param[0]))
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }
}

void la64_op_stb(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    if(!la64_memory_write(core, *(core->op.param[0]), *(core->op.param[1]), sizeof(uint8_t)))
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }
}

void la64_op_stw(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    if(!la64_memory_write(core, *(core->op.param[0]), *(core->op.param[1]), sizeof(uint16_t)))
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }
}

void la64_op_std(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    if(!la64_memory_write(core, *(core->op.param[0]), *(core->op.param[1]), sizeof(uint32_t)))
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }
}

void la64_op_stq(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 2);

    if(!la64_memory_write(core, *(core->op.param[0]), *(core->op.param[1]), sizeof(uint64_t)))
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }
}