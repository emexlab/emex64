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

#ifndef EMEX64ASM_EMITTER_OPCODE_H
#define EMEX64ASM_EMITTER_OPCODE_H

#include <stdbool.h>
#include <emex64lib/vm/core.h>
#include <emex64lib/asm/invocation.h>

kEmex64Opcode opcode_from_string(const char *name);
bool opcode_arg_accepts_reg_only(const emex64_opfunc_entry_t *opce, uint8_t arg);

void assembler_emit_opcode(assembler_invocation_t *inv, kEmex64Opcode op);

#endif /* EMEX64ASM_EMITTER_OPCODE_H */
