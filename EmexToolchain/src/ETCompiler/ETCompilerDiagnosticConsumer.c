
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
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETCompiler/ETCompilerDiagnosticConsumer.h>
#include <EmexToolchain/Support/diagnostic/consumer.h>

typedef diagnostic_consumer_t compiler_diagnostic_consumer_t;

typedef struct assembler_diagnostic_consumer_context {
    ETCompilerDiagnosticOptions options;
    diagnostic_t **diagnostic;
    UInt64 diagnostic_cnt;
    EFFileHandleRef d;
} compiler_diagnostic_consumer_context_t;

static const char *__compiler_diagnostic_color(compiler_diagnostic_consumer_context_t *ctx,
                                               char *str)
{
    if(ctx->options.color_diagnostics)
    {
        return str;
    }
    else
    {
        return "";
    }
}

static void __compiler_diagnostic_consumer_show_caret_preview(compiler_diagnostic_consumer_context_t *ctx,
                                                              diagnostic_t *diagnostic)
{
    const char *src = diagnostic->location->line;
    EFSize line_num = diagnostic->location->ln;

    EFSize n = line_num;
    SInt32 ndigits = 1;
    while(n >= 10)
    {
        n /= 10; ndigits++;
    }
    SInt32 w = ndigits + 3;

    char numbuf[32];
    SInt32 nlen = 0;
    {
        EFSize v = line_num;
        char tmp[32];
        SInt32 t = 0;
        do {
            tmp[t++] = '0' + (v % 10); v /= 10;
        } while(v);
        while(t > 0) numbuf[nlen++] = tmp[--t];
        numbuf[nlen] = '\0';
    }

    for(SInt32 i = 0; i < w - nlen; i++)
    {
        EFFileHandlePutc(ctx->d, ' ');
    }
    EFFileHandlePuts(ctx->d, numbuf);
    EFFileHandlePuts(ctx->d, " | ");
    EFFileHandlePuts(ctx->d, src);
    EFFileHandlePutc(ctx->d, '\n');

    for(SInt32 i = 0; i < w + 1; i++)
    {
        EFFileHandlePutc(ctx->d, ' ');
    }
    EFFileHandlePuts(ctx->d, "| ");
    EFSize indent = diagnostic->location->range.start_col > 0 ? diagnostic->location->range.start_col - 1 : 0;
    for(EFSize i = 0; i < indent && src[i] != '\0'; i++)
    {
        EFFileHandlePutc(ctx->d, src[i] == '\t' ? '\t' : ' ');
    }

    EFFileHandlePuts(ctx->d, __compiler_diagnostic_color(ctx, C_BOLD));
    EFFileHandlePuts(ctx->d, __compiler_diagnostic_color(ctx, C_CARET));
    EFFileHandlePutc(ctx->d, '^');
    EFSize span = diagnostic->location->range.end_col > diagnostic->location->range.start_col ? diagnostic->location->range.end_col - diagnostic->location->range.start_col : 1;
    for(EFSize i = 1; i < span; i++)
    {
        EFFileHandlePutc(ctx->d, '~');
    }
    EFFileHandlePuts(ctx->d, __compiler_diagnostic_color(ctx, C_RESET));
    EFFileHandlePutc(ctx->d, '\n');
}

static void __compiler_diagnostic_consumer_consume_diagnostic_handler(compiler_diagnostic_consumer_t *consumer,
                                                                      diagnostic_t *diagnostic)
{
    compiler_diagnostic_consumer_context_t *ctx = consumer->ctx;

    void *newp = realloc(ctx->diagnostic, (ctx->diagnostic_cnt + 1) * sizeof(diagnostic_t*));
    if(newp != NULL)
    {
        ctx->diagnostic = newp;
        ctx->diagnostic[ctx->diagnostic_cnt++] = diagnostic;
        return;
    }

    /* failed to be appended so to the waste with it with it.. */
    diagnostic_dealloc(diagnostic);
}

static compiler_diagnostic_consumer_t *compiler_diagnostic_consumer_alloc(ETCompilerDiagnosticOptions options)
{
    compiler_diagnostic_consumer_t *consumer = malloc(sizeof(compiler_diagnostic_consumer_t));
    if(consumer == NULL)
    {
        return NULL;
    }

    consumer->ctx = malloc(sizeof(compiler_diagnostic_consumer_context_t));
    if(consumer->ctx == NULL)
    {
        free(consumer);
        return NULL;
    }

    compiler_diagnostic_consumer_context_t *ctx = consumer->ctx;
    ctx->options = options;
    ctx->diagnostic = NULL;
    ctx->diagnostic_cnt = 0;
    ctx->d = EFFileHandleCreateWithFileDescriptor(kEFAllocatorDefault, STDERR_FILENO);
    if(ctx->d == NULL)
    {
        free(ctx);
        free(consumer);
        return NULL;
    }

    consumer->consume_handler = __compiler_diagnostic_consumer_consume_diagnostic_handler;

    return consumer;
}

static void compiler_diagnostic_consumer_dealloc(compiler_diagnostic_consumer_t *consumer)
{
    if(consumer == NULL)
    {
        return;
    }

    compiler_diagnostic_consumer_context_t *ctx = consumer->ctx;
    EFRelease(ctx->d);
    for(UInt64 i = 0; i < ctx->diagnostic_cnt; i++)
    {
        diagnostic_dealloc(ctx->diagnostic[i]);
    }
    free(ctx->diagnostic);
    free(consumer->ctx);
    free(consumer);
}

void compiler_diagnostic_consumer_emit(compiler_diagnostic_consumer_t *consumer)
{
    compiler_diagnostic_consumer_context_t *ctx = consumer->ctx;

    /* tiny diagnostic engine ^^ */
    for(UInt64 i = 0; i < ctx->diagnostic_cnt; i++)
    {
        diagnostic_t *diagnostic = ctx->diagnostic[i];
        if(diagnostic->location != NULL)
        {
            EFFileHandlePrintf(ctx->d, "%s:%llu:%llu: ", EFStringGetCStringPtr(EFURLGetPath(diagnostic->location->fileURL), kEFStringEncodingUTF8), diagnostic->location->ln, diagnostic->location->col);
        }

        /* fallback when no consumer was specified */
        Boolean isNotRaw = true;
        switch(diagnostic->severity)
        {
            case kDiagnosticSeverityNote:
                EFFileHandlePrintf(ctx->d, "%snote:", __compiler_diagnostic_color(ctx, C_NOTE));
                break;
            case kDiagnosticSeverityWarning:
                EFFileHandlePrintf(ctx->d, "%swarning:", __compiler_diagnostic_color(ctx, C_WARN));
                break;
            case kDiagnosticSeverityError:
                EFFileHandlePrintf(ctx->d, "%serror:", __compiler_diagnostic_color(ctx, C_ERROR));
                break;
            case kDiagnosticSeverityFatal:
                EFFileHandlePrintf(ctx->d, "%sfatal:", __compiler_diagnostic_color(ctx, C_ERROR));
                break;
            default:
                isNotRaw = false;
                break;
        }

        if(isNotRaw)
        {
            EFFileHandlePrintf(ctx->d, "%s ", __compiler_diagnostic_color(ctx, C_RESET));
            EFFileHandlePrintf(ctx->d, "%s\n", diagnostic->str);
        }
        else
        {
            EFFileHandlePrintf(ctx->d, "%s", diagnostic->str);
        }

        if(ctx->options.caret_diagnostics &&
           diagnostic->location != NULL)
        {
            __compiler_diagnostic_consumer_show_caret_preview(ctx, diagnostic);
        }

        /* dont forget to flush the toilet otherwise things get stinky */
        EFFileHandleSync(ctx->d);
    }

    for(UInt64 i = 0; i < ctx->diagnostic_cnt; i++)
    {
        diagnostic_dealloc(ctx->diagnostic[i]);
    }
    free(ctx->diagnostic);
    ctx->diagnostic = NULL;
    ctx->diagnostic_cnt = 0;
}

typedef struct __ETCompilerDiagnosticConsumer {
    EFObject header;
    compiler_diagnostic_consumer_t *consumer;
} *__ETCompilerDiagnosticConsumer;

static void __ETCompilerDiagnosticConsumerDeinit(EFObjectRef consumerRef)
{
    __ETCompilerDiagnosticConsumer consumer = (__ETCompilerDiagnosticConsumer)consumerRef;
    compiler_diagnostic_consumer_dealloc(consumer->consumer);
}

static EFClassDefinitionV2 ETCompilerDiagnosticConsumerClass = {
    .header = {
        .version = 2,
        .typeID = kEFTypeIDNone,
        .name = NULL,
    },
    .name = "ETCompilerDiagnosticConsumer",
    .init = NULL,
    .deinit = __ETCompilerDiagnosticConsumerDeinit,
    .equal = NULL,
    .copyDescription = NULL,
};

static void ETCompilerDiagnosticConsumerRegisterClass(void)
{
    EFClassRegister(&ETCompilerDiagnosticConsumerClass);
}

EFTypeID ETCompilerDiagnosticConsumerGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, ETCompilerDiagnosticConsumerRegisterClass);
    return ETCompilerDiagnosticConsumerClass.header.typeID;
}

ETCompilerDiagnosticConsumerRef ETCompilerDiagnosticConsumerCreate(EFAllocatorRef allocatorRef,
                                                                   ETCompilerDiagnosticOptions diagnosticOptions)
{
    compiler_diagnostic_consumer_t *rawConsumer = compiler_diagnostic_consumer_alloc(diagnosticOptions);
    if(rawConsumer == NULL)
    {
        return NULL;
    }

    __ETCompilerDiagnosticConsumer consumer = (__ETCompilerDiagnosticConsumer)EFObjectCreate(allocatorRef, ETCompilerDiagnosticConsumerGetTypeID(), (EFIndex)sizeof(struct __ETCompilerDiagnosticConsumer));
    if(consumer == NULL)
    {
        compiler_diagnostic_consumer_dealloc(rawConsumer);
        return NULL;
    }

    consumer->consumer = rawConsumer;

    return (ETCompilerDiagnosticConsumerRef)consumer;
}

compiler_diagnostic_consumer_t *ETCompilerDiagnosticConsumerGetPtr(ETCompilerDiagnosticConsumerRef consumerRef)
{
    __ETCompilerDiagnosticConsumer consumer = (__ETCompilerDiagnosticConsumer)consumerRef;
    if(consumer == NULL)
    {
        return NULL;
    }

    return consumer->consumer;
}

ETCompilerDiagnosticOptions ETCompilerDiagnosticConsumerGetDiagnosticOptions(ETCompilerDiagnosticConsumerRef consumerRef)
{
    __ETCompilerDiagnosticConsumer consumer = (__ETCompilerDiagnosticConsumer)consumerRef;
    if(consumer == NULL)
    {
        return ETCompilerDiagnosticOptionsDefault;
    }

    compiler_diagnostic_consumer_context_t *ctx = (compiler_diagnostic_consumer_context_t*)consumer->consumer->ctx;
    return ctx->options;
}

void ETCompilerDiagnosticConsumerSetDiagnosticOptions(ETCompilerDiagnosticConsumerRef consumerRef,
                                                      ETCompilerDiagnosticOptions diagnosticOptions)
{
    __ETCompilerDiagnosticConsumer consumer = (__ETCompilerDiagnosticConsumer)consumerRef;
    if(consumer == NULL)
    {
        return;
    }

    compiler_diagnostic_consumer_context_t *ctx = (compiler_diagnostic_consumer_context_t*)consumer->consumer->ctx;
    ctx->options = diagnosticOptions;
}

void ETCompilerDiagnosticConsumerReport(ETCompilerDiagnosticConsumerRef consumerRef,
                                        kDiagnosticSeverity severity,
                                        diagnostic_location_t *location,
                                        EFStringRef format,
                                        ...)
{
    __ETCompilerDiagnosticConsumer consumer = (__ETCompilerDiagnosticConsumer)consumerRef;
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

void ETCompilerDiagnosticConsumerEmit(ETCompilerDiagnosticConsumerRef consumerRef)
{
    __ETCompilerDiagnosticConsumer consumer = (__ETCompilerDiagnosticConsumer)consumerRef;
    if(consumer == NULL)
    {
        return;
    }

    compiler_diagnostic_consumer_emit(consumer->consumer);
}
