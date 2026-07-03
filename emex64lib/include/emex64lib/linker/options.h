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

#ifndef EMEX64LD_OPTIONS_H
#define EMEX64LD_OPTIONS_H

#include <stdbool.h>
#include <emex64lib/linker/type.h>

typedef struct linker_options {
    bool verbose;
    kEmitMode emit_mode;
    const char *entry_name; /* borrowed */
} linker_options_t;

extern linker_options_t linker_options_default;

#endif /* EMEX64LD_OPTIONS_H */
