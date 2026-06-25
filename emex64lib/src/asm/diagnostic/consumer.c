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
#include <emex64lib/asm/diagnostic/consumer.h>
#include <emex64lib/asm/invocation.h>

static void __assembler_diagnostic_consumer_show_caret_preview(vfd_t *d, diagnostic_t *diagnostic)
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
        vfd_putc(d, ' ');
    }
    vfd_puts(d, numbuf);
    vfd_puts(d, " | ");
    vfd_puts(d, src);
    vfd_putc(d, '\n');

    for(int i = 0; i < w + 1; i++)
    {
        vfd_putc(d, ' ');
    }
    vfd_puts(d, "| ");
    size_t indent = diagnostic->location->range.start_col > 0 ? diagnostic->location->range.start_col - 1 : 0;
    for(size_t i = 0; i < indent && src[i] != '\0'; i++)
    {
        vfd_putc(d, src[i] == '\t' ? '\t' : ' ');
    }

    vfd_puts(d, C_BOLD);
    vfd_puts(d, C_CARET);
    vfd_putc(d, '^');
    size_t span = diagnostic->location->range.end_col > diagnostic->location->range.start_col ? diagnostic->location->range.end_col - diagnostic->location->range.start_col : 1;
    for(size_t i = 1; i < span; i++)
    {
        vfd_putc(d, '~');
    }
    vfd_puts(d, C_RESET);
    vfd_putc(d, '\n');
}

static void __assembler_diagnostic_consumer_consume_diagnostic_handler(diagnostic_consumer_t *consumer,
                                                                       diagnostic_t *diagnostic)
{
    assembler_diagnostic_consumer_context_t *ctx = consumer->ctx;

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

    if(((assembler_diagnostic_consumer_context_t*)consumer->ctx)->inv != NULL &&
       ((assembler_diagnostic_consumer_context_t*)consumer->ctx)->inv->options.caret_diagnostics &&
       diagnostic->location != NULL)
    {
        __assembler_diagnostic_consumer_show_caret_preview(ctx->d, diagnostic);
    }

    /* dont forget to flush the toilet otherwise things get stinky */
    fflush(stderr);

    diagnostic_dealloc(diagnostic);
}

assembler_diagnostic_consumer_t *assembler_diagnostic_consumer_alloc()
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
    ctx->inv = NULL;
    ctx->diagnostic = NULL;
    ctx->diagnostic_cnt = 0;
    ctx->d = vfd_open_fd(STDERR_FILENO);
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
    vfd_close(ctx->d);
    for(uint64_t i = 0; i < ctx->diagnostic_cnt; i++)
    {
        diagnostic_dealloc(ctx->diagnostic[i]);
    }
    free(ctx->diagnostic);
    free(consumer->ctx);
    free(consumer);
}

void assembler_diagnostic_consumer_bind_invocation(assembler_diagnostic_consumer_t *consumer,
                                                   assembler_invocation_t *inv)
{
    ((assembler_diagnostic_consumer_context_t*)consumer->ctx)->inv = inv;
}

void assembler_diagnostic_consumer_unbind_invocation(assembler_diagnostic_consumer_t *consumer)
{
    ((assembler_diagnostic_consumer_context_t*)consumer->ctx)->inv = NULL;
}

