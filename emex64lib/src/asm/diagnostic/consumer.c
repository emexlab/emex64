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

#include <stdlib.h>
#include <stdint.h>
#include <emex64lib/asm/diagnostic/consumer.h>
#include <emex64lib/asm/invocation.h>

static void __assembler_diagnostic_consumer_show_caret_preview(diagnostic_t *diagnostic)
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

    fprintf(stderr, "%*zu | %s\n", w, line_num, src);

    for(int i = 0; i < w + 1; i++)
    {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "| ");

    size_t indent = diagnostic->location->range.start_col > 0 ? diagnostic->location->range.start_col - 1 : 0;
    for(size_t i = 0; i < indent && src[i] != '\0'; i++)
    {
        fprintf(stderr, src[i] == '\t' ? "\t" : " ");
    }

    fprintf(stderr, "%s%s^", C_BOLD, C_CARET);   /* bold green */
    size_t span = diagnostic->location->range.end_col > diagnostic->location->range.start_col ? diagnostic->location->range.end_col - diagnostic->location->range.start_col : 1;
    for(size_t i = 1; i < span; i++)
    {
        fprintf(stderr, "~");
    }
    fprintf(stderr, "%s\n", C_RESET);
}

static void __assembler_diagnostic_consumer_consume_diagnostic_handler(diagnostic_consumer_t *consumer,
                                                                       diagnostic_t *diagnostic)
{
    if(diagnostic->location != NULL)
    {
        fprintf(stderr, "%s:%llu:%llu: ", diagnostic->location->file_name, diagnostic->location->ln, diagnostic->location->col);
    }

    /* fallback when no consumer was specified */
    switch(diagnostic->severity)
    {
        case kDiagnosticSeverityNote:
            fprintf(stderr, "%snote:", C_NOTE);
            break;
        case kDiagnosticSeverityWarning:
            fprintf(stderr, "%swarning:", C_WARN); 
            break;
        case kDiagnosticSeverityError:
            fprintf(stderr, "%serror:", C_ERROR);
            break;
        case kDiagnosticSeverityFatal:
        default:
            fprintf(stderr, "%sfatal:", C_ERROR);
            break;
    }
    fprintf(stderr, "%s ", C_RESET);

    fprintf(stderr, "%s\n", diagnostic->str);

    if(((assembler_diagnostic_consumer_context_t*)consumer->ctx)->inv != NULL &&
       ((assembler_diagnostic_consumer_context_t*)consumer->ctx)->inv->options.caret_diagnostics &&
       diagnostic->location != NULL)
    {
        __assembler_diagnostic_consumer_show_caret_preview(diagnostic);
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

    ((assembler_diagnostic_consumer_context_t*)consumer->ctx)->inv = NULL;
    ((assembler_diagnostic_consumer_context_t*)consumer->ctx)->diagnostic = NULL;
    ((assembler_diagnostic_consumer_context_t*)consumer->ctx)->diagnostic_cnt = 0;

    consumer->consume_handler = __assembler_diagnostic_consumer_consume_diagnostic_handler;

    return consumer;
}

void assembler_diagnostic_consumer_dealloc(assembler_diagnostic_consumer_t *consumer)
{
    if(consumer == NULL)
    {
        return;
    }

    for(uint64_t i = 0; i < ((assembler_diagnostic_consumer_context_t*)consumer->ctx)->diagnostic_cnt; i++)
    {
        diagnostic_dealloc(((assembler_diagnostic_consumer_context_t*)consumer->ctx)->diagnostic[i]);
    }
    free(((assembler_diagnostic_consumer_context_t*)consumer->ctx)->diagnostic);
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

