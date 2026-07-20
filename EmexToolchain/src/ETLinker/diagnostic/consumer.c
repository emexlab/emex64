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
#include <EmexToolchain/ETLinker/diagnostic/consumer.h>

static void __linker_diagnostic_consumer_consume_diagnostic_handler(diagnostic_consumer_t *consumer,
                                                                    diagnostic_t *diagnostic)
{
    linker_diagnostic_consumer_context_t *ctx = consumer->ctx;

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

linker_diagnostic_consumer_t *linker_diagnostic_consumer_alloc()
{
    linker_diagnostic_consumer_t *consumer = malloc(sizeof(linker_diagnostic_consumer_t));
    if(consumer == NULL)
    {
        return NULL;
    }

    consumer->ctx = malloc(sizeof(linker_diagnostic_consumer_context_t));
    if(consumer->ctx == NULL)
    {
        free(consumer);
        return NULL;
    }

    linker_diagnostic_consumer_context_t *ctx = consumer->ctx;
    ctx->diagnostic = NULL;
    ctx->diagnostic_cnt = 0;
    ctx->d = EFFileHandleCreateWithFileDescriptor(kEFAllocatorDefault, STDERR_FILENO);
    if(ctx->d == NULL)
    {
        free(ctx);
        free(consumer);
        return NULL;
    }

    consumer->consume_handler = __linker_diagnostic_consumer_consume_diagnostic_handler;

    return consumer;
}

void linker_diagnostic_consumer_dealloc(linker_diagnostic_consumer_t *consumer)
{
    if(consumer == NULL)
    {
        return;
    }

    linker_diagnostic_consumer_context_t *ctx = consumer->ctx;
    EFRelease(ctx->d);
    for(UInt64 i = 0; i < ctx->diagnostic_cnt; i++)
    {
        diagnostic_dealloc(ctx->diagnostic[i]);
    }
    free(ctx->diagnostic);
    free(consumer->ctx);
    free(consumer);
}

void linker_diagnostic_consumer_emit(linker_diagnostic_consumer_t *consumer)
{
    linker_diagnostic_consumer_context_t *ctx = consumer->ctx;

    /* tiny diagnostic engine ^^ */
    for(UInt64 i = 0; i < ctx->diagnostic_cnt; i++)
    {
        diagnostic_t *diagnostic = ctx->diagnostic[i];
        if(diagnostic->location != NULL)
        {
            EFFileHandlePrintf(ctx->d, "%s:%llu:%llu: ", diagnostic->location->file_name, diagnostic->location->ln, diagnostic->location->col);
        }

        /* fallback when no consumer was specified */
        switch(diagnostic->severity)
        {
            case kDiagnosticSeverityNote:
                EFFileHandlePrintf(ctx->d, "%snote:", C_NOTE);
                break;
            case kDiagnosticSeverityWarning:
                EFFileHandlePrintf(ctx->d, "%swarning:", C_WARN);
                break;
            case kDiagnosticSeverityError:
                EFFileHandlePrintf(ctx->d, "%serror:", C_ERROR);
                break;
            case kDiagnosticSeverityFatal:
            default:
                EFFileHandlePrintf(ctx->d, "%sfatal:", C_ERROR);
                break;
        }
        EFFileHandlePrintf(ctx->d, "%s ", C_RESET);

        EFFileHandlePrintf(ctx->d, "%s\n", diagnostic->str);

        /* dont forget to flush the toilet otherwise things get stinky */
        EFFileHandleSync(ctx->d);
    }
}
