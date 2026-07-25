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

#ifndef EMEX64ASM_EMITTER_EMITTER_H
#define EMEX64ASM_EMITTER_EMITTER_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>
#include <EmexToolchain/ETAssembler/type.h>
#include <EmexToolchain/ETAssembler/emitter/opcode.h>
#include <EmexToolchain/ETAssembler/emitter/register.h>
#include <EmexToolchain/ETAssembler/emitter/immediate.h>

void assembler_emit_end(ETAssemblerInvocationRef inv);

Boolean assembler_emit_instruction(assembler_line_t *al);
Boolean assembler_emit(ETAssemblerInvocationRef inv);

#endif /* EMEX64ASM_EMITTER_EMITTER_H */
