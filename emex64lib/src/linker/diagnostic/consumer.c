/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <emex64lib/linker/diagnostic/consumer.h>

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
    ctx->d = vfd_open_fd(STDERR_FILENO);
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
    vfd_close(ctx->d);
    for(uint64_t i = 0; i < ctx->diagnostic_cnt; i++)
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
    for(uint64_t i = 0; i < ctx->diagnostic_cnt; i++)
    {
        diagnostic_t *diagnostic = ctx->diagnostic[i];
        if(diagnostic->location != NULL)
        {
            vfdprintf(ctx->d, "%s:%llu:%llu: ", diagnostic->location->file_name, diagnostic->location->ln, diagnostic->location->col);
        }

        /* fallback when no consumer was specified */
        switch(diagnostic->severity)
        {
            case kDiagnosticSeverityNote:
                vfdprintf(ctx->d, "%snote:", C_NOTE);
                break;
            case kDiagnosticSeverityWarning:
                vfdprintf(ctx->d, "%swarning:", C_WARN);
                break;
            case kDiagnosticSeverityError:
                vfdprintf(ctx->d, "%serror:", C_ERROR);
                break;
            case kDiagnosticSeverityFatal:
            default:
                vfdprintf(ctx->d, "%sfatal:", C_ERROR);
                break;
        }
        vfdprintf(ctx->d, "%s ", C_RESET);

        vfdprintf(ctx->d, "%s\n", diagnostic->str);

        /* dont forget to flush the toilet otherwise things get stinky */
        vfd_sync(ctx->d);
    }
}
