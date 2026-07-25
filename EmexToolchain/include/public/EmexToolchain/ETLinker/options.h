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

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETLinker/type.h>

typedef struct linker_options {
    Boolean verbose;        /* default: false */
    Boolean use_old_magic;  /* default: false */
    kEmitMode emit_mode;    /* default: kEmitModeFirmware */
    const char *entry_name; /* borrowed, default: _start */
} linker_options_t;

extern linker_options_t linker_options_default;

#endif /* EMEX64LD_OPTIONS_H */
