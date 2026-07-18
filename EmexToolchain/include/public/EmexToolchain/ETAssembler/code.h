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

#ifndef EMEX64ASM_CODE_H
#define EMEX64ASM_CODE_H

#include <stdlib.h>
#include <EmexToolchain/Support/file.h>
#include <EmexToolchain/ETAssembler/type.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>

char *assembler_code_find_header(const char *name, const char *source_file);
char *assembler_code_find_system_header(const char *name, const char **inc_dirs, size_t inc_cnt);

Boolean assembler_code_inject_file(assembler_invocation_t *inv, UInt64 at_line_index, emex_file_t *inj_file);

Boolean assembler_code_preparse(assembler_invocation_t *inv, emex_file_t *input);
Boolean assembler_code_postparse(assembler_invocation_t *inv);

#endif /* EMEX64ASM_CODE_H */
