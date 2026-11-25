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

void E64FADD(__E64Core core);
void E64FSUB(__E64Core core);
void E64FMUL(__E64Core core);
void E64FDIV(__E64Core core);
void E64FMIN(__E64Core core);
void E64FMAX(__E64Core core);
void E64FSQRT(__E64Core core);
void E64FMADD(__E64Core core);
void E64FNEG(__E64Core core);
void E64FABS(__E64Core core);
void E64FCMP(__E64Core core);
void E64FADDS(__E64Core core);
void E64FSUBS(__E64Core core);
void E64FMULS(__E64Core core);
void E64FDIVS(__E64Core core);
void E64FMINS(__E64Core core);
void E64FMAXS(__E64Core core);
void E64FSQRTS(__E64Core core);
void E64FMADDS(__E64Core core);
void E64FNEGS(__E64Core core);
void E64FABSS(__E64Core core);
void E64FCMPS(__E64Core core);
void E64FCVTDS(__E64Core core);
void E64FCVTSD(__E64Core core);
void E64FCVTDI(__E64Core core);
void E64FCVTDU(__E64Core core);
void E64FCVTID(__E64Core core);
void E64FCVTUD(__E64Core core);
void E64FCVTSI(__E64Core core);
void E64FCVTSU(__E64Core core);
void E64FCVTIS(__E64Core core);
void E64FCVTUS(__E64Core core);

#endif /* EMEX64VM_INSTRUCTION_FPU_H */
