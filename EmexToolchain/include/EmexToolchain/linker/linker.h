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

#ifndef EMEX64LD_LINKER_H
#define EMEX64LD_LINKER_H

#include <EmexToolchain/linker/type.h>
#include <EmexToolchain/linker/header.h>
#include <EmexToolchain/linker/sym.h>
#include <EmexToolchain/linker/obj.h>
#include <EmexToolchain/linker/options.h>
#include <EmexToolchain/linker/script.h>
#include <EmexToolchain/linker/emit.h>
#include <EmexToolchain/linker/driver.h>
#include <EmexToolchain/linker/diagnostic/consumer.h>

typedef struct linker_invocation {
    linker_options_t options;
    linker_diagnostic_consumer_t *consumer; /* borowwed */

    linker_symbol_t *sym;
    linker_object_t *obj;

    script_sym_t *script_syms;
    size_t script_sym_cnt;

    uint64_t out_text_off;
    uint64_t out_data_off;
    uint64_t out_bss_off;
} linker_invocation_t;

linker_invocation_t *linker_invocation_alloc(linker_options_t options, linker_diagnostic_consumer_t *diagnostic_consumer);
void linker_invocation_dealloc(linker_invocation_t *inv);

bool linker_symbol_append_definition(linker_invocation_t *inv, const char *name, const char *object_path, uint64_t addr);
linker_symbol_t *linker_symbol_lookup(linker_invocation_t *inv, const char *name);

#endif /* EMEX64LD_LINKER_H */
