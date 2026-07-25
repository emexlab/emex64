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

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETLinker/type.h>
#include <EmexToolchain/ETLinker/header.h>
#include <EmexToolchain/ETLinker/sym.h>
#include <EmexToolchain/ETLinker/obj.h>
#include <EmexToolchain/ETLinker/options.h>
#include <EmexToolchain/ETLinker/script.h>
#include <EmexToolchain/ETLinker/emit.h>
#include <EmexToolchain/ETLinker/driver.h>
#include <EmexToolchain/ETLinker/diagnostic/consumer.h>

typedef struct linker_invocation {
    linker_options_t options;
    linker_diagnostic_consumer_t *consumer; /* borowwed */

    linker_symbol_t *sym;
    linker_object_t *obj;

    script_sym_t *script_syms;
    EFSize script_sym_cnt;

    UInt64 out_text_off;
    UInt64 out_data_off;
    UInt64 out_bss_off;

    Boolean needs_fw_hdr;
} linker_invocation_t;

linker_invocation_t *linker_invocation_alloc(linker_options_t options, linker_diagnostic_consumer_t *diagnostic_consumer);
void linker_invocation_dealloc(linker_invocation_t *inv);

Boolean linker_symbol_append_definition(linker_invocation_t *inv, const char *name, const char *object_path, UInt64 addr);
linker_symbol_t *linker_symbol_lookup(linker_invocation_t *inv, const char *name);

#endif /* EMEX64LD_LINKER_H */
