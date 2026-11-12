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

#ifndef EMEX64VM_INSTRUCTION_FPU_H
#define EMEX64VM_INSTRUCTION_FPU_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/E64Core.h>

void emex64_op_fadd(__E64Core core);
void emex64_op_fsub(__E64Core core);
void emex64_op_fmul(__E64Core core);
void emex64_op_fdiv(__E64Core core);
void emex64_op_fmin(__E64Core core);
void emex64_op_fmax(__E64Core core);
void emex64_op_fsqrt(__E64Core core);
void emex64_op_fmadd(__E64Core core);
void emex64_op_fneg(__E64Core core);
void emex64_op_fabs(__E64Core core);
void emex64_op_fcmp(__E64Core core);
void emex64_op_fadds(__E64Core core);
void emex64_op_fsubs(__E64Core core);
void emex64_op_fmuls(__E64Core core);
void emex64_op_fdivs(__E64Core core);
void emex64_op_fmins(__E64Core core);
void emex64_op_fmaxs(__E64Core core);
void emex64_op_fsqrts(__E64Core core);
void emex64_op_fmadds(__E64Core core);
void emex64_op_fnegs(__E64Core core);
void emex64_op_fabss(__E64Core core);
void emex64_op_fcmps(__E64Core core);
void emex64_op_fcvtds(__E64Core core);
void emex64_op_fcvtsd(__E64Core core);
void emex64_op_fcvtdi(__E64Core core);
void emex64_op_fcvtdu(__E64Core core);
void emex64_op_fcvtid(__E64Core core);
void emex64_op_fcvtud(__E64Core core);
void emex64_op_fcvtsi(__E64Core core);
void emex64_op_fcvtsu(__E64Core core);
void emex64_op_fcvtis(__E64Core core);
void emex64_op_fcvtus(__E64Core core);

#endif /* EMEX64VM_INSTRUCTION_FPU_H */
