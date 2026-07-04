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

#ifndef EMEX64VM_INSTRUCTION_ALU_H
#define EMEX64VM_INSTRUCTION_ALU_H

#include <EmexToolchain/vm/E64Core.h>

void emex64_op_add(__E64Core core);
void emex64_op_sub(__E64Core core);
void emex64_op_mul(__E64Core core);
void emex64_op_div(__E64Core core);
void emex64_op_idiv(__E64Core core);
void emex64_op_mod(__E64Core core);
void emex64_op_not(__E64Core core);
void emex64_op_neg(__E64Core core);
void emex64_op_and(__E64Core core);
void emex64_op_or(__E64Core core);
void emex64_op_xor(__E64Core core);
void emex64_op_shr(__E64Core core);
void emex64_op_shl(__E64Core core);
void emex64_op_sar(__E64Core core);
void emex64_op_ror(__E64Core core);
void emex64_op_rol(__E64Core core);

void emex64_op_pdep(__E64Core core);
void emex64_op_pext(__E64Core core);
void emex64_op_bswapw(__E64Core core);
void emex64_op_bswapd(__E64Core core);
void emex64_op_bswapq(__E64Core core);
void emex64_op_inc(__E64Core core);
void emex64_op_dec(__E64Core core);

#endif /* EMEX64VM_INSTRUCTION_ALU_H */
