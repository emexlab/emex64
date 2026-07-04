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

#ifndef EMEX64LD_SCRIPT_H
#define EMEX64LD_SCRIPT_H

#include <stdint.h>
#include <stdbool.h>
#include <EmexToolchain/support/file.h>
#include <EmexToolchain/linker/type.h>

typedef struct linker_invocation linker_invocation_t;

typedef struct {
    const char *script_path;    /* borrowed */
    char *name;
    char *expr;
} script_sym_t;

Boolean linker_script_parse(linker_invocation_t *inv, emex_file_t *script_file);
Boolean linker_script_apply(linker_invocation_t *inv, UInt64 image_end, UInt64 text_start, UInt64 data_start, UInt64 bss_start);

#endif /* EMEX64LD_SCRIPT_H */
