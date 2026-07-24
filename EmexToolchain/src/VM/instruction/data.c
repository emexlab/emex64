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

#include <EmexToolchain/VM/instruction/data.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/E64Memory.h>

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

void emex64_op_cmov(E64CoreRef core)
{
    if(core->cr_state.crel.level < kE64ElevationLevelKernel)
    {
        core->cr_state.crexc.exception = kE64ExceptionPermission;
        return;
    }

    UInt8 cr_select = *(core->op.param[0]);
    UInt64 cr_value = *(core->op.param[1]);

    switch(cr_select)
    {
        case kE64ControlRegisterCR0: /* elevation level */
            if(kE64ElevationLevelSecureMonitor < cr_value)
            {
                core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
                return;
            }
            if(core->cr_state.crel.level < cr_value)
            {
                core->cr_state.crexc.exception = kE64ExceptionPermission;
                return;
            }
            core->cr_state.crel.level = cr_value;
            break;
        case kE64ControlRegisterCR1: /* kernel stack pointer */
            core->cr_state.crksp.address = cr_value;
            break;
        case kE64ControlRegisterCR2: /* exception  */
            core->cr_state.crexc.exception = (UInt8)cr_value;
            break;
        case kE64ControlRegisterCR4: /* page table */
            core->cr_state.crptb.enabled = (cr_value & EMEX64_MEMORY_MMU_MASK_FLAGS) & kE64MMUPTPresent;
            core->cr_state.crptb.pgd_addr = ((cr_value & EMEX64_MEMORY_MMU_MASK_PFN) >> 8) << 13;
            break;
        default:
            core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
            return;
    }
}

void emex64_op_cmovb(E64CoreRef core)
{
    UInt64 *cr_recv = core->op.param[0];
    UInt8 cr_select = *(core->op.param[1]);

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
            *cr_recv |= -(UInt64)core->cr_state.crptb.enabled & kE64MMUPTPresent;
            break;
        default:
            core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
            return;
    }
}

void emex64_op_clar(__E64Core core)
{
    for(UInt8 index = kE64RegisterR0; index < kE64RegisterMAX; index++)
    {
        core->rl[index] = 0;
    }
}
