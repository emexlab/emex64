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

#ifndef EMEX64VM_INSTRUCTION_DATA_H
#define EMEX64VM_INSTRUCTION_DATA_H

#include <EmexToolchain/vm/E64Core.h>

void emex64_op_mov(__E64Core core);
void emex64_op_swp(__E64Core core);
void emex64_op_movz(__E64Core core);
void emex64_op_push(__E64Core core);
void emex64_op_pop(__E64Core core);
void emex64_op_ldb(__E64Core core);
void emex64_op_ldw(__E64Core core);
void emex64_op_ldd(__E64Core core);
void emex64_op_ldq(__E64Core core);
void emex64_op_stb(__E64Core core);
void emex64_op_stw(__E64Core core);
void emex64_op_std(__E64Core core);
void emex64_op_stq(__E64Core core);

void emex64_op_clr(__E64Core core);

void emex64_op_cmov(__E64Core core);
void emex64_op_cmovb(__E64Core core);

void emex64_op_ldbi(__E64Core core);
void emex64_op_ldwi(__E64Core core);
void emex64_op_lddi(__E64Core core);
void emex64_op_ldqi(__E64Core core);
void emex64_op_stbi(__E64Core core);
void emex64_op_stwi(__E64Core core);
void emex64_op_stdi(__E64Core core);
void emex64_op_stqi(__E64Core core);

#endif /* EMEX64VM_INSTRUCTION_DATA_H */
