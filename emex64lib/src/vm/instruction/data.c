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
        E64MemoryCoreAction(core->machine->memory, core, core->rl[kE64RegisterSP], sizeof(uint64_t), core->op.param[i], kE64MemoryActionWrite);
        core->rl[kE64RegisterSP] -= 8;
    }
}

void emex64_op_pop(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt == 0);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        core->rl[kE64RegisterSP] += 8;
        E64MemoryCoreAction(core->machine->memory, core, core->rl[kE64RegisterSP], sizeof(uint64_t), core->op.param[i], kE64MemoryActionRead);
    }
}

void emex64_op_ldb(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(uint8_t), core->op.param[0], kE64MemoryActionRead);
}

void emex64_op_ldw(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(uint16_t), core->op.param[0], kE64MemoryActionRead);
}

void emex64_op_ldd(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(uint32_t), core->op.param[0], kE64MemoryActionRead);
}

void emex64_op_ldq(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[1]), sizeof(uint64_t), core->op.param[0], kE64MemoryActionRead);
}

void emex64_op_stb(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(uint8_t), core->op.param[1], kE64MemoryActionWrite);
}

void emex64_op_stw(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(uint16_t), core->op.param[1], kE64MemoryActionWrite);
}

void emex64_op_std(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(uint32_t), core->op.param[1], kE64MemoryActionWrite);
}

void emex64_op_stq(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 2);

    E64MemoryCoreAction(core->machine->memory, core, *(core->op.param[0]), sizeof(uint64_t), core->op.param[1], kE64MemoryActionWrite);
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

    if(core->cr_state.crel.level < kE64ElevationLevelKernel)
    {
        core->cr_state.crexc.exception = kE64ExceptionPermission;
        return;
    }

    uint8_t cr_select = *(core->op.param[0]);
    uint64_t cr_value = *(core->op.param[1]);

    switch(cr_select)
    {
        case kE64ControlRegisterCR0: /* elevation level */
            core->cr_state.crel.level = cr_value;
            break;
        case kE64ControlRegisterCR1: /* kernel stack pointer */
            core->cr_state.crksp.address = cr_value;
            break;
        case kE64ControlRegisterCR2: /* exception  */
            core->cr_state.crexc.exception = cr_value;
            break;
        case kE64ControlRegisterCR4: /* page table */
            core->cr_state.crptb.enabled = (cr_value & EMEX64_MEMORY_MMU_MASK_FLAGS) & kE64MMUPTPresent;
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
        case kE64ControlRegisterCR0: /* elevation level */
            *cr_recv = core->cr_state.crel.level;
            break;
        case kE64ControlRegisterCR1: /* kernel stack pointer */
            if(core->cr_state.crel.level < kE64ElevationLevelKernel)
            {
                core->cr_state.crexc.exception = kE64ExceptionPermission;
                return;
            }

            *cr_recv = core->cr_state.crksp.address;
            break;
        case kE64ControlRegisterCR2: /* exception  */
            *cr_recv = core->cr_state.crexc.exception;
            break;
        case kE64ControlRegisterCR4: /* page table */
            if(core->cr_state.crel.level < kE64ElevationLevelKernel)
            {
                core->cr_state.crexc.exception = kE64ExceptionPermission;
                return;
            }

            *cr_recv = 0;
            *cr_recv |= ((core->cr_state.crptb.pgd_addr >> 13) << 8) & EMEX64_MEMORY_MMU_MASK_PFN;
            *cr_recv |= -(uint64_t)core->cr_state.crptb.enabled & kE64MMUPTPresent;
            break;
    }
}
