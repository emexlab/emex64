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

#include <EmexToolchain/vm/E64Machine.h>
#include <EmexToolchain/vm/E64Core.h>
#include <EmexToolchain/vm/E64Memory.h>

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

static inline void emex64_push_il(__E64Core core, UInt64 value)
{
    E64MemoryCoreAction(core->machine->memory, core, core->rl[kE64RegisterSP], sizeof(UInt64), &value, kE64MemoryActionTypeWrite);
    core->rl[kE64RegisterSP] -= 8;
}

static inline UInt64 emex64_pop_il(__E64Core core)
{
    core->rl[kE64RegisterSP] += 8;

    UInt64 value = 0;
    E64MemoryCoreAction(core->machine->memory, core, core->rl[kE64RegisterSP], sizeof(UInt64), &value, kE64MemoryActionTypeRead);
    return value;
}

#endif /* EMEX64VM_INSTRUCTION_CTRL_H */
