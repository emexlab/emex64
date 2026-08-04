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

#include <stdarg.h>
#include <pthread.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETAssembler/ETAssemblerDiagnosticConsumer.h>

typedef struct __ETAssemblerDiagnosticConsumer {
    EFObject header;
    assembler_diagnostic_consumer_t *consumer;
} *__ETAssemblerDiagnosticConsumer;

static void __ETAssemblerDiagnosticConsumerDeinit(EFObjectRef consumerRef)
{
    __ETAssemblerDiagnosticConsumer consumer = (__ETAssemblerDiagnosticConsumer)consumerRef;
    assembler_diagnostic_consumer_dealloc(consumer->consumer);
}

static EFClassDefinitionV2 ETAssemblerDiagnosticConsumerClass = {
    .header = {
        .version = 2,
        .typeID = kEFTypeIDNone,
        .name = NULL,
    },
    .name = "ETAssemblerDiagnosticConsumer",
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
    return ETAssemblerDiagnosticConsumerClass.header.typeID;
}

ETAssemblerDiagnosticConsumerRef ETAssemblerDiagnosticConsumerCreate(EFAllocatorRef allocatorRef,
                                                                     ETAssemblerDiagnosticOptions diagnosticOptions)
{
    assembler_diagnostic_consumer_t *rawConsumer = assembler_diagnostic_consumer_alloc(diagnosticOptions);
    if(rawConsumer == NULL)
    {
        return NULL;
    }

    __ETAssemblerDiagnosticConsumer consumer = (__ETAssemblerDiagnosticConsumer)EFObjectCreate(allocatorRef, ETAssemblerDiagnosticConsumerGetTypeID(), (EFIndex)sizeof(struct __ETAssemblerDiagnosticConsumer));
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

ETAssemblerDiagnosticOptions ETAssemblerDiagnosticConsumerGetDiagnosticOptions(ETAssemblerDiagnosticConsumerRef consumerRef)
{
    __ETAssemblerDiagnosticConsumer consumer = (__ETAssemblerDiagnosticConsumer)consumerRef;
    if(consumer == NULL)
    {
        return ETAssemblerDiagnosticOptionsDefault;
    }

    assembler_diagnostic_consumer_context_t *ctx = (assembler_diagnostic_consumer_context_t*)consumer->consumer->ctx;
    return ctx->options;
}

void ETAssemblerDiagnosticConsumerSetDiagnosticOptions(ETAssemblerDiagnosticConsumerRef consumerRef,
                                                       ETAssemblerDiagnosticOptions diagnosticOptions)
{
    __ETAssemblerDiagnosticConsumer consumer = (__ETAssemblerDiagnosticConsumer)consumerRef;
    if(consumer == NULL)
    {
        return;
    }

    assembler_diagnostic_consumer_context_t *ctx = (assembler_diagnostic_consumer_context_t*)consumer->consumer->ctx;
    ctx->options = diagnosticOptions;
}

void ETAssemblerDiagnosticConsumerReport(ETAssemblerDiagnosticConsumerRef consumerRef,
                                         kDiagnosticSeverity severity,
                                         diagnostic_location_t *location,
                                         EFStringRef format,
                                         ...)
{
    __ETAssemblerDiagnosticConsumer consumer = (__ETAssemblerDiagnosticConsumer)consumerRef;
    if(consumer == NULL || format == NULL)
    {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    EFStringRef result = EFStringCreateWithFormatAndArguments(kEFAllocatorDefault, format, arguments);
    va_end(arguments);
    if(result == NULL)
    {
        return;
    }

    const char *cptr = EFStringGetCStringPtr(result, kEFStringEncodingASCII);
    if(cptr == NULL)
    {
        EFRelease(result);
        return;
    }

    diagnostic_report(consumer->consumer, severity, location, "%s", cptr);
    EFRelease(result);
}

void ETAssemblerDiagnosticConsumerEmit(ETAssemblerDiagnosticConsumerRef consumerRef)
{
    __ETAssemblerDiagnosticConsumer consumer = (__ETAssemblerDiagnosticConsumer)consumerRef;
    if(consumer == NULL)
    {
        return;
    }

    assembler_diagnostic_consumer_emit(consumer->consumer);
}
