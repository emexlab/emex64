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
#include <EmexToolchain/ETAssembler/diagnostic/consumer.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>

static const char *__assembler_diagnostic_color(assembler_diagnostic_consumer_context_t *ctx,
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

static void __assembler_diagnostic_consumer_show_caret_preview(assembler_diagnostic_consumer_context_t *ctx,
                                                               diagnostic_t *diagnostic)
{
    const char *src = diagnostic->location->line;
    size_t line_num = diagnostic->location->ln;

    size_t n = line_num;
    int ndigits = 1;
    while(n >= 10)
    {
        n /= 10; ndigits++;
    }
    int w = ndigits + 3;

    char numbuf[32];
    int nlen = 0;
    {
        size_t v = line_num;
        char tmp[32];
        int t = 0;
        do {
            tmp[t++] = '0' + (v % 10); v /= 10;
        } while(v);
        while(t > 0) numbuf[nlen++] = tmp[--t];
        numbuf[nlen] = '\0';
    }

    for(int i = 0; i < w - nlen; i++)
    {
        EFFileHandlePutc(ctx->d, ' ');
    }
    EFFileHandlePuts(ctx->d, numbuf);
    EFFileHandlePuts(ctx->d, " | ");
    EFFileHandlePuts(ctx->d, src);
    EFFileHandlePutc(ctx->d, '\n');

    for(int i = 0; i < w + 1; i++)
    {
        EFFileHandlePutc(ctx->d, ' ');
    }
    EFFileHandlePuts(ctx->d, "| ");
    size_t indent = diagnostic->location->range.start_col > 0 ? diagnostic->location->range.start_col - 1 : 0;
    for(size_t i = 0; i < indent && src[i] != '\0'; i++)
    {
        EFFileHandlePutc(ctx->d, src[i] == '\t' ? '\t' : ' ');
    }

    EFFileHandlePuts(ctx->d, __assembler_diagnostic_color(ctx, C_BOLD));
    EFFileHandlePuts(ctx->d, __assembler_diagnostic_color(ctx, C_CARET));
    EFFileHandlePutc(ctx->d, '^');
    size_t span = diagnostic->location->range.end_col > diagnostic->location->range.start_col ? diagnostic->location->range.end_col - diagnostic->location->range.start_col : 1;
    for(size_t i = 1; i < span; i++)
    {
        EFFileHandlePutc(ctx->d, '~');
    }
    EFFileHandlePuts(ctx->d, __assembler_diagnostic_color(ctx, C_RESET));
    EFFileHandlePutc(ctx->d, '\n');
}

static void __assembler_diagnostic_consumer_consume_diagnostic_handler(diagnostic_consumer_t *consumer,
                                                                       diagnostic_t *diagnostic)
{
    assembler_diagnostic_consumer_context_t *ctx = consumer->ctx;

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

assembler_diagnostic_consumer_t *assembler_diagnostic_consumer_alloc(ETAssemblerDiagnosticOptions options)
{
    assembler_diagnostic_consumer_t *consumer = malloc(sizeof(assembler_diagnostic_consumer_t));
    if(consumer == NULL)
    {
        return NULL;
    }

    consumer->ctx = malloc(sizeof(assembler_diagnostic_consumer_context_t));
    if(consumer->ctx == NULL)
    {
        free(consumer);
        return NULL;
    }

    assembler_diagnostic_consumer_context_t *ctx = consumer->ctx;
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

    consumer->consume_handler = __assembler_diagnostic_consumer_consume_diagnostic_handler;

    return consumer;
}

void assembler_diagnostic_consumer_dealloc(assembler_diagnostic_consumer_t *consumer)
{
    if(consumer == NULL)
    {
        return;
    }

    assembler_diagnostic_consumer_context_t *ctx = consumer->ctx;
    EFRelease(ctx->d);
    for(UInt64 i = 0; i < ctx->diagnostic_cnt; i++)
    {
        diagnostic_dealloc(ctx->diagnostic[i]);
    }
    free(ctx->diagnostic);
    free(consumer->ctx);
    free(consumer);
}

void assembler_diagnostic_consumer_emit(assembler_diagnostic_consumer_t *consumer)
{
    assembler_diagnostic_consumer_context_t *ctx = consumer->ctx;

    /* tiny diagnostic engine ^^ */
    for(UInt64 i = 0; i < ctx->diagnostic_cnt; i++)
    {
        diagnostic_t *diagnostic = ctx->diagnostic[i];
        if(diagnostic->location != NULL)
        {
            EFAUTOREL EFStringRef pathStr = EFURLCopyPath(EFGetAllocator(diagnostic->location->fileURL), diagnostic->location->fileURL);
            EFFileHandlePrintf(ctx->d, "%s:%llu:%llu: ", EFStringGetCStringPtr(pathStr, kEFStringEncodingUTF8), diagnostic->location->ln, diagnostic->location->col);
        }

        /* fallback when no consumer was specified */
        Boolean isNotRaw = true;
        switch(diagnostic->severity)
        {
            case kDiagnosticSeverityNote:
                EFFileHandlePrintf(ctx->d, "%snote:", __assembler_diagnostic_color(ctx, C_NOTE));
                break;
            case kDiagnosticSeverityWarning:
                EFFileHandlePrintf(ctx->d, "%swarning:", __assembler_diagnostic_color(ctx, C_WARN));
                break;
            case kDiagnosticSeverityError:
                EFFileHandlePrintf(ctx->d, "%serror:", __assembler_diagnostic_color(ctx, C_ERROR));
                break;
            case kDiagnosticSeverityFatal:
                EFFileHandlePrintf(ctx->d, "%sfatal:", __assembler_diagnostic_color(ctx, C_ERROR));
                break;
            default:
                isNotRaw = false;
                break;
        }

        if(isNotRaw)
        {
            EFFileHandlePrintf(ctx->d, "%s ", __assembler_diagnostic_color(ctx, C_RESET));
            EFFileHandlePrintf(ctx->d, "%s\n", diagnostic->str);
        }
        else
        {
            EFFileHandlePrintf(ctx->d, "%s", diagnostic->str);
        }

        if(ctx->options.caret_diagnostics &&
           diagnostic->location != NULL)
        {
            __assembler_diagnostic_consumer_show_caret_preview(ctx, diagnostic);
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
