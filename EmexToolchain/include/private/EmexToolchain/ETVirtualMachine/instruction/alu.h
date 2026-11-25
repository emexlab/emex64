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

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETVirtualMachine/E64Core.h>

void E64ADD(__E64Core core);
void E64SUB(__E64Core core);
void E64MUL(__E64Core core);
void E64DIV(__E64Core core);
void E64IDIV(__E64Core core);
void E64MOD(__E64Core core);
void E64NOT(__E64Core core);
void E64NEG(__E64Core core);
void E64AND(__E64Core core);
void E64OR(__E64Core core);
void E64XOR(__E64Core core);
void E64SHR(__E64Core core);
void E64SHL(__E64Core core);
void E64SAR(__E64Core core);
void E64ROR(__E64Core core);
void E64ROL(__E64Core core);

void E64PDEP(__E64Core core);
void E64PEXT(__E64Core core);
void E64BSWAPW(__E64Core core);
void E64BSWAPD(__E64Core core);
void E64BSWAPQ(__E64Core core);
void E64INC(__E64Core core);
void E64DEC(__E64Core core);

void E64SEXT(__E64Core core);
void E64CLZ(__E64Core core);
void E64CTZ(__E64Core core);
void E64POPCNT(__E64Core core);
void E64MULH(__E64Core core);
void E64UMULH(__E64Core core);
void E64ADC(__E64Core core);
void E64SBB(__E64Core core);

void E64ADDD(__E64Core core);
void E64IADDD(__E64Core core);
void E64SUBD(__E64Core core);
void E64ISUBD(__E64Core core);
void E64MULD(__E64Core core);
void E64IMULD(__E64Core core);
void E64SHLD(__E64Core core);
void E64ISHLD(__E64Core core);
void E64SHRD(__E64Core core);
void E64SARD(__E64Core core);
void E64DIVD(__E64Core core);
void E64IDIVD(__E64Core core);
void E64MODD(__E64Core core);

#endif /* EMEX64VM_INSTRUCTION_ALU_H */
