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

#include <EmexToolchain/asm/diagnostic/ETAssemblerDiagnosticConsumer.h>

typedef struct __ETAssemblerDiagnosticConsumer {
    EFObject header;
    assembler_diagnostic_consumer_t *consumer;
} *__ETAssemblerDiagnosticConsumer;

static void __ETAssemblerDiagnosticConsumerDeinit(EFObjectRef consumerRef)
{
    __ETAssemblerDiagnosticConsumer consumer = (__ETAssemblerDiagnosticConsumer)consumerRef;
    assembler_diagnostic_consumer_dealloc(consumer->consumer);
}

static EFClass ETAssemblerDiagnosticConsumerClass = {
    .name = "ETAssemblerDiagnosticConsumer",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = __ETAssemblerDiagnosticConsumerDeinit,
    .equal = NULL,
    .copyDescription = NULL,
};

static void ETAssemblerDiagnosticConsumerRegisterClass(void)
{
    EFClassRegister(&ETAssemblerDiagnosticConsumerClass);
}

EFTypeID ETAssemblerDiagnosticConsumerGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, ETAssemblerDiagnosticConsumerRegisterClass);
    return ETAssemblerDiagnosticConsumerClass.typeID;
}

ETAssemblerDiagnosticConsumerRef ETAssemblerDiagnosticConsumerCreate(EFAllocatorRef allocatorRef,
                                                                     ETAssemblerDiagnosticOptions diagnosticOptions)
{
    assembler_diagnostic_consumer_t *rawConsumer = assembler_diagnostic_consumer_alloc(diagnosticOptions);
    if(rawConsumer == NULL)
    {
        return NULL;
    }

    __ETAssemblerDiagnosticConsumer consumer = (__ETAssemblerDiagnosticConsumer)EFObjectAlloc(allocatorRef, ETAssemblerDiagnosticConsumerGetTypeID(), sizeof(struct __ETAssemblerDiagnosticConsumer));
    if(consumer == NULL)
    {
        assembler_diagnostic_consumer_dealloc(rawConsumer);
        return NULL;
    }

    consumer->consumer = rawConsumer;

    return (ETAssemblerDiagnosticConsumerRef)consumer;
}

assembler_diagnostic_consumer_t *ETAssemblerDiagnosticConsumerGetPtr(ETAssemblerDiagnosticConsumerRef consumerRef)
{
    __ETAssemblerDiagnosticConsumer consumer = (__ETAssemblerDiagnosticConsumer)consumerRef;
    if(consumer == NULL)
    {
        return NULL;
    }

    return consumer->consumer;
}
