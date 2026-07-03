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

#include <emex64lib/vm/machine.h>
#include <emex64lib/vm/core.h>
#include <emex64lib/vm/memory.h>

void emex64_op_b(emex64_core_t *core);
void emex64_op_cmp(emex64_core_t *core);
void emex64_op_be(emex64_core_t *core);
void emex64_op_bne(emex64_core_t *core);
void emex64_op_blt(emex64_core_t *core);
void emex64_op_bgt(emex64_core_t *core);
void emex64_op_ble(emex64_core_t *core);
void emex64_op_bge(emex64_core_t *core);
void emex64_op_bz(emex64_core_t *core);
void emex64_op_bnz(emex64_core_t *core);

void emex64_op_blw(emex64_core_t *core);
void emex64_op_wret(emex64_core_t *core);
void emex64_op_iret(emex64_core_t *core);

void emex64_op_bl(emex64_core_t *core);
void emex64_op_ret(emex64_core_t *core);

static inline void emex64_push_il(emex64_core_t *core, uint64_t value)
{
    Emex64MemoryCoreAction(core->machine->memory, core, core->rl[kEmex64RegisterSP], sizeof(uint64_t), &value, kEmex64MemoryActionWrite);
    core->rl[kEmex64RegisterSP] -= 8;
}

static inline uint64_t emex64_pop_il(emex64_core_t *core)
{
    core->rl[kEmex64RegisterSP] += 8;

    uint64_t value = 0;
    Emex64MemoryCoreAction(core->machine->memory, core, core->rl[kEmex64RegisterSP], sizeof(uint64_t), &value, kEmex64MemoryActionRead);
    return value;
}

#endif /* EMEX64VM_INSTRUCTION_CTRL_H */
