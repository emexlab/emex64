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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETLinker/linker.h>

const UInt8 ident[EI_NIDENT] = { ELF_MAGIC_0, ELF_MAGIC_1, ELF_MAGIC_2, ELF_MAGIC_3, ELF_CLASS64, ELF_DATA2LSB, EV_CURRENT };

linker_invocation_t *linker_invocation_alloc(linker_options_t options,
                                             linker_diagnostic_consumer_t *diagnostic_consumer)
{
    linker_invocation_t *inv = malloc(sizeof(linker_invocation_t));
    if(inv == NULL)
    {
        return NULL;
    }

    inv->consumer = diagnostic_consumer;

    inv->sym = NULL;
    inv->obj = NULL;
    inv->script_syms = NULL;
    inv->script_sym_cnt = 0;

    /* a lot of problems to solve for today :3 */
    inv->out_text_off = BOOT_HEADER_SIZE;
    inv->out_data_off = 0;
    inv->out_bss_off = 0;

    inv->needs_fw_hdr = true;

    inv->options = options;

    return inv;
}

void linker_invocation_dealloc(linker_invocation_t *inv)
{
    linker_symbol_t *sym = inv->sym;
    while(sym != NULL)
    {
        linker_symbol_t *next = sym->next;
        linker_symbol_dealloc(sym);
        sym = next;
    }

    linker_object_t *obj = inv->obj;
    while(obj != NULL)
    {
        linker_object_t *next = obj->next;
        linker_object_dealloc(obj);
        obj = next;
    }

    for(size_t i = 0; i < inv->script_sym_cnt; i++)
    {
        free(inv->script_syms[i].name);
        free(inv->script_syms[i].expr);
    }
    free(inv->script_syms);

    free(inv);
}

Boolean linker_symbol_append_definition(linker_invocation_t *inv,
                                        const char *name,
                                        const char *object_path,
                                        UInt64 addr)
{
    linker_symbol_t *sym = linker_symbol_lookup(inv, name);
    if(sym == NULL)
    {
        sym = linker_symbol_alloc(name, object_path, addr, true);
        if(sym == NULL)
        {
            return false;
        }
    }
    if(sym->defined && sym->addr != addr)
    {
        diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "duplicate symbol '%s' in '%s'", name, object_path);
        diagnostic_report(inv->consumer, kDiagnosticSeverityNote, NULL, "symbol '%s' also exists in '%s'", name, sym->object_path);
        return false;
    }

    if(inv->sym == NULL)
    {
        inv->sym = sym;
    }
    else
    {
        sym->next = inv->sym;
        inv->sym = sym;
    }

    return true;
}

linker_symbol_t *linker_symbol_lookup(linker_invocation_t *inv,
                                      const char *name)
{
    linker_symbol_t *sym = inv->sym;
    while(sym != NULL)
    {
        if(strcmp(sym->name, name) == 0)
        {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}
