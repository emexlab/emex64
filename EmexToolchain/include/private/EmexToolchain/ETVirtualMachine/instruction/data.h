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

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETVirtualMachine/E64Core.h>

void E64MOV(__E64Core core);
void E64SWP(__E64Core core);
void E64MOVZ(__E64Core core);
void E64PUSH(__E64Core core);
void E64POP(__E64Core core);
void E64LDB(__E64Core core);
void E64LDW(__E64Core core);
void E64LDD(__E64Core core);
void E64LDQ(__E64Core core);
void E64STB(__E64Core core);
void E64STW(__E64Core core);
void E64STD(__E64Core core);
void E64STQ(__E64Core core);

void E64CLR(__E64Core core);
void E64CMOV(__E64Core core);
void E64CMOVB(__E64Core core);
void E64CSWP(__E64Core core);
void E64XCHG(__E64Core core);
void E64CAS(__E64Core core);
void E64CLAR(__E64Core core);

void E64RDRND(__E64Core core);

void E64MOVM(__E64Core core);
void E64CPYM(__E64Core core);
void E64SETM(__E64Core core);

void E64RDTIME(__E64Core core);
void E64LDSB(__E64Core core);
void E64LDSW(__E64Core core);
void E64LDSD(__E64Core core);
void E64LDADD(__E64Core core);
void E64LDAND(__E64Core core);
void E64LDOR(__E64Core core);
void E64LDXOR(__E64Core core);
void E64SWPADD(__E64Core core);
void E64LDADDD(__E64Core core);
void E64LDANDD(__E64Core core);
void E64LDORD(__E64Core core);
void E64LDXORD(__E64Core core);
void E64SWPADDD(__E64Core core);

void E64IOLDB(__E64Core core);
void E64IOLDW(__E64Core core);
void E64IOLDD(__E64Core core);
void E64IOLDQ(__E64Core core);
void E64IOSTB(__E64Core core);
void E64IOSTW(__E64Core core);
void E64IOSTD(__E64Core core);
void E64IOSTQ(__E64Core core);

#endif /* EMEX64VM_INSTRUCTION_DATA_H */
