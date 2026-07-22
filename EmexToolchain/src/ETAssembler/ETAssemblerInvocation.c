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
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETAssembler/preprocessor/preprocessor.h>
#include <EmexToolchain/ETAssembler/label/label.h>
#include <EmexToolchain/ETAssembler/emitter/emitter.h>
#include <EmexToolchain/ETAssembler/emitter/elf.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>
#include <EmexToolchain/ETAssembler/code.h>
#include <EmexToolchain/ETAssembler/section.h>

static void __ETAssemblerInvocationDeinit(EFObjectRef invocationRef)
{
    __ETAssemblerInvocation invocation = (__ETAssemblerInvocation)invocationRef;

    /* options have to be freed by who allocated them */

    /* skipping the tool managed input file object */
    for(size_t i = 1; i < invocation->file_cnt; i++)
    {
        EFRelease(invocation->file[i]);
    }
    free(invocation->file);

    for(UInt64 i = 0; i < invocation->line_cnt; i++)
    {
        free(invocation->line[i]->str);
        for(UInt64 j = 0; j < invocation->line[i]->token_cnt; j++)
        {
            if(invocation->line[i]->token[j]->type == kETAssemblerTokenTypeString)
            {
                free(invocation->line[i]->token[j]->string_literal.buf);
            }

            free(invocation->line[i]->token[j]->str);
            free(invocation->line[i]->token[j]);
        }
        free(invocation->line[i]->token);
        free(invocation->line[i]);
    }

    const void *key; size_t klen; assembler_label_t *val;
    for(hashmap_iter_t it = hashmap_iter_create(invocation->label_hashmap); hashmap_next(&it, &key, &klen, (void**)&val);)
    {
        free(val->name);
        free(val);
    }
    hashmap_dealloc(invocation->label_hashmap);

    /* definitions and include directories have to be released by who allocated them */

    reloc_table_entry_t *rtbe = invocation->rtbe;
    while(rtbe != NULL)
    {
        free(rtbe->name);
        reloc_table_entry_t *next = rtbe->next;
        free(rtbe);
        rtbe = next;
    }

    EFReleaseTry(invocation->out_vbitwalker);
    EFReleaseTry(invocation->diagnosticConsumer);
    
    EFReleaseTry(invocation->definitions);
    EFReleaseTry(invocation->includeSearchPaths);
}

EFClass ETAssemblerInvocationClass = {
    .name = "ETAssemblerInvocation",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __ETAssemblerInvocationDeinit,
    .equal = NULL,
    .copyDescription = NULL,
    .hash = NULL,
};

void ETAssemblerInvocationRegisterClass(void)
{
    EFClassRegister(&ETAssemblerInvocationClass);
}

EFTypeID ETAssemblerInvocationGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, ETAssemblerInvocationRegisterClass);
    return ETAssemblerInvocationClass.typeID;
}

ETAssemblerInvocationRef ETAssemblerInvocationCreate(EFAllocatorRef allocatorRef,
                                                     ETAssemblerDiagnosticConsumerRef diagnosticConsumer)
{
    EFAUTOREL __ETAssemblerInvocation invocation = (__ETAssemblerInvocation)EFObjectCreate(allocatorRef, ETAssemblerInvocationGetTypeID(), (EFIndex)sizeof(struct __ETAssemblerInvocation));
    if(invocation == NULL)
    {
        return NULL;
    }

    invocation->definitions = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksDefaultCallbacks, 0);
    invocation->includeSearchPaths = EFArrayCreateMutable(allocatorRef, kEFArrayCallbacksObjectCallbacks, 0);

    invocation->diagnosticConsumer = EFRetainTry(diagnosticConsumer);
    invocation->consumer = ETAssemblerDiagnosticConsumerGetPtr(invocation->diagnosticConsumer);
    if(invocation->consumer == NULL)
    {
        return NULL;
    }

    invocation->label_hashmap = hashmap_alloc();
    if(invocation->label_hashmap == NULL)
    {
        return NULL;
    }

    invocation->data_section_start = UINT64_MAX;
    invocation->data_section_end = UINT64_MAX;
    invocation->bss_section_start = UINT64_MAX;

    return (ETAssemblerInvocationRef)EFAUTOTRANSFER(invocation);
}

Boolean ETAssemblerInvocationAddMacroDefinition(ETAssemblerInvocationRef invocationRef,
                                                assembler_macro_definition_t *macro)
{
    __ETAssemblerInvocation invocation = (__ETAssemblerInvocation)invocationRef;
    if(invocation == NULL)
    {
        return false;
    }

    if(!EFArrayAppendValue(invocation->definitions, macro))
    {
        return false;
    }

    return true;
}

Boolean ETAssemblerInvocationAddIncludeSearchPath(ETAssemblerInvocationRef invocationRef,
                                                  EFStringRef includeSearchPath)
{
    __ETAssemblerInvocation invocation = (__ETAssemblerInvocation)invocationRef;
    if(invocation == NULL)
    {
        return false;
    }

    if(!EFArrayAppendValue(invocation->includeSearchPaths, includeSearchPath))
    {
        return false;
    }

    return true;
}

Boolean ETAssemblerInvocationEmit(ETAssemblerInvocationRef invocationRef,
                                  EFFileRef input,
                                  EFFileRef output)
{
    __ETAssemblerInvocation invocation = (__ETAssemblerInvocation)invocationRef;
    if(invocation == NULL || input == NULL || output == NULL)
    {
        return false;
    }

    /* need output */
    invocation->out_vbitwalker = EFFileCopyBitWalker(kEFAllocatorDefault, output, kEFEndianLittle);
    if(invocation->out_vbitwalker == NULL)
    {
        diagnostic_report(invocation->consumer, kDiagnosticSeverityFatal, NULL, "couldn't allocate fdwalker");
        return false;
    }

    EFBitWalkerSeek(invocation->out_vbitwalker, 10, 0);

    if(!assembler_code_preparse(invocation, input) ||
       !assembler_preprocessor_run(invocation) ||
       !assembler_code_postparse(invocation) ||
       !assembler_section_parse(invocation) ||
       !assembler_emit(invocation) ||
       !assembler_elf_emit(invocation))
    {
        EFFileUnlink(output);
        return false;
    }

    return true;
}
