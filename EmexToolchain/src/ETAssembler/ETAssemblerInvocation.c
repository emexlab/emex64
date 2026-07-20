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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/Support/file.h>
#include <EmexToolchain/ETAssembler/preprocessor/preprocessor.h>
#include <EmexToolchain/ETAssembler/label/label.h>
#include <EmexToolchain/ETAssembler/emitter/emitter.h>
#include <EmexToolchain/ETAssembler/emitter/elf.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>
#include <EmexToolchain/ETAssembler/code.h>
#include <EmexToolchain/ETAssembler/section.h>

assembler_invocation_t *assembler_invocation_alloc(assembler_diagnostic_consumer_t *consumer)
{
    assembler_invocation_t *inv = calloc(1, sizeof(assembler_invocation_t));
    if(inv == NULL)
    {
        return NULL;
    }

    inv->consumer = consumer;

    inv->label_hashmap = hashmap_alloc();
    if(inv->label_hashmap == NULL)
    {
        free(inv);
        return NULL;
    }

    inv->data_section_start = UINT64_MAX;
    inv->data_section_end = UINT64_MAX;
    inv->bss_section_start = UINT64_MAX;
    return inv;
}

void assembler_invocation_dealloc(assembler_invocation_t *inv)
{
    /* options have to be freed by who allocated them */

    /* skipping the tool managed input file object */
    for(size_t i = 1; i < inv->file_cnt; i++)
    {
        emex_file_dealloc(inv->file[i]);
    }
    free(inv->file);

    for(UInt64 i = 0; i < inv->line_cnt; i++)
    {
        free(inv->line[i]->str);
        for(UInt64 j = 0; j < inv->line[i]->token_cnt; j++)
        {
            if(inv->line[i]->token[j]->type == kETAssemblerTokenTypeString)
            {
                free(inv->line[i]->token[j]->string_literal.buf);
            }

            free(inv->line[i]->token[j]->str);
            free(inv->line[i]->token[j]);
        }
        free(inv->line[i]->token);
        free(inv->line[i]);
    }
    free(inv->line);

    const void *key; size_t klen; assembler_label_t *val;
    for(hashmap_iter_t it = hashmap_iter_create(inv->label_hashmap); hashmap_next(&it, &key, &klen, (void**)&val);)
    {
        free(val->name);
        free(val);
    }
    hashmap_dealloc(inv->label_hashmap);

    /* definitions and include directories have to be released by who allocated them */

    reloc_table_entry_t *rtbe = inv->rtbe;
    while(rtbe != NULL)
    {
        free(rtbe->name);
        reloc_table_entry_t *next = rtbe->next;
        free(rtbe);
        rtbe = next;
    }

    EFReleaseTry(inv->out_vbitwalker);
    free(inv);
}

Boolean assembler_invocation_emit(assembler_invocation_t *inv,
                                  emex_file_t *input,
                                  emex_file_t *output)
{
    /* need output */
    inv->out_vbitwalker = emex_file_dup_vbitwalker(output, kEFEndianLittle);
    if(inv->out_vbitwalker == NULL)
    {
        diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "couldn't allocate fdwalker");
        return false;
    }

    EFBitWalkerSeek(inv->out_vbitwalker, 10, 0);

    if(!assembler_code_preparse(inv, input) ||
       !assembler_preprocessor_run(inv) ||
       !assembler_code_postparse(inv) ||
       !assembler_section_parse(inv) ||
       !assembler_emit(inv) ||
       !assembler_elf_emit(inv))
    {
        emex_file_unlink(output);
        return false;
    }

    return true;
}
