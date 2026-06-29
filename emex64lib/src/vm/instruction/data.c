/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
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
#include <emex64lib/vm/instruction/data.h>
#include <emex64lib/vm/machine.h>
#include <emex64lib/vm/memory.h>

void emex64_op_mov(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    *(core->op.param[0]) = *(core->op.param[1]);
}

void emex64_op_swp(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    uint64_t param_backup = *(core->op.param[0]);
    *(core->op.param[0]) = *(core->op.param[1]);
    *(core->op.param[1]) = param_backup;
}

void emex64_op_swpz(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    *(core->op.param[0]) = *(core->op.param[1]);
    *(core->op.param[1]) = 0;
}

void emex64_op_push(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt == 0);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        Emex64MemoryCoreAction(core->machine->memory, core, core->rl[kEmex64RegisterSP], sizeof(uint64_t), core->op.param[i], kEmex64MemoryActionWrite);
        core->rl[kEmex64RegisterSP] -= 8;
    }
}

void emex64_op_pop(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt == 0);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        core->rl[kEmex64RegisterSP] += 8;
        Emex64MemoryCoreAction(core->machine->memory, core, core->rl[kEmex64RegisterSP], sizeof(uint64_t), core->op.param[i], kEmex64MemoryActionRead);
    }
}

void emex64_op_ldb(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    Emex64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(uint8_t), core->op.param[0], kEmex64MemoryActionRead);
}

void emex64_op_ldw(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    Emex64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(uint16_t), core->op.param[0], kEmex64MemoryActionRead);
}

void emex64_op_ldd(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    Emex64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(uint32_t), core->op.param[0], kEmex64MemoryActionRead);
}

void emex64_op_ldq(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    Emex64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(uint64_t), core->op.param[0], kEmex64MemoryActionRead);
}

void emex64_op_stb(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    Emex64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(uint8_t), core->op.param[1], kEmex64MemoryActionWrite);
}

void emex64_op_stw(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    Emex64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(uint16_t), core->op.param[1], kEmex64MemoryActionWrite);
}

void emex64_op_std(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    Emex64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(uint32_t), core->op.param[1], kEmex64MemoryActionWrite);
}

void emex64_op_stq(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    Emex64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(uint64_t), core->op.param[1], kEmex64MemoryActionWrite);
}

void emex64_op_clr(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt < 1);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        *core->op.param[i] = 0;
    }
}

void emex64_op_cmov(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    if(core->cr_state.crel.level < kEmex64ElevationLevelKernel)
    {
        core->cr_state.crexc.exception = kEmex64ExceptionPermission;
        return;
    }

    uint8_t cr_select = *(core->op.param[0]);
    uint64_t cr_value = *(core->op.param[1]);

    switch(cr_select)
    {
        case kEmex64ControlRegisterCR0: /* elevation level */
            core->cr_state.crel.level = cr_value;
            break;
        case kEmex64ControlRegisterCR1: /* kernel stack pointer */
            core->cr_state.crksp.address = cr_value;
            break;
        case kEmex64ControlRegisterCR2: /* exception  */
            core->cr_state.crexc.exception = cr_value;
            break;
        case kEmex64ControlRegisterCR4: /* page table */
            core->cr_state.crptb.enabled = (cr_value & EMEX64_MEMORY_MMU_MASK_FLAGS) & kEmex64MMUPTPresent;
            core->cr_state.crptb.pgd_addr = ((cr_value & EMEX64_MEMORY_MMU_MASK_PFN) >> 8) << 13;
            break;
    }
}

void emex64_op_cmovb(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    uint64_t *cr_recv = core->op.param[0];
    uint8_t cr_select = *(core->op.param[1]);

    switch(cr_select)
    {
        case kEmex64ControlRegisterCR0: /* elevation level */
            *cr_recv = core->cr_state.crel.level;
            break;
        case kEmex64ControlRegisterCR1: /* kernel stack pointer */
            if(core->cr_state.crel.level < kEmex64ElevationLevelKernel)
            {
                core->cr_state.crexc.exception = kEmex64ExceptionPermission;
                return;
            }

            *cr_recv = core->cr_state.crksp.address;
            break;
        case kEmex64ControlRegisterCR2: /* exception  */
            *cr_recv = core->cr_state.crexc.exception;
            break;
        case kEmex64ControlRegisterCR4: /* page table */
            if(core->cr_state.crel.level < kEmex64ElevationLevelKernel)
            {
                core->cr_state.crexc.exception = kEmex64ExceptionPermission;
                return;
            }

            *cr_recv = 0;
            *cr_recv |= ((core->cr_state.crptb.pgd_addr >> 13) << 8) & EMEX64_MEMORY_MMU_MASK_PFN;
            *cr_recv |= -(uint64_t)core->cr_state.crptb.enabled & kEmex64MMUPTPresent;
            break;
    }
}
