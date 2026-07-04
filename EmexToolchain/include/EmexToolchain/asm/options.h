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

#ifndef EMEX64ASM_OPTIONS_H
#define EMEX64ASM_OPTIONS_H

#include <stdbool.h>

typedef struct assembler_driver_options {
    bool assemble_only;
    bool verbose;
    bool in_process;
} assembler_driver_options_t;

typedef struct assembler_invocation_options {
    /* features */
    bool caret_diagnostics;         /* default: true */
    bool color_diagnostics;         /* default: true */

    /* warnings */
    bool warning_error;             /* default: false */
    bool warning_deprecated;        /* default: true */
} assembler_diagnostic_options_t;

extern assembler_driver_options_t assembler_driver_options_default;
extern assembler_diagnostic_options_t assembler_diagnostic_options_default;

#endif /* EMEX64ASM_OPTIONS_H */
