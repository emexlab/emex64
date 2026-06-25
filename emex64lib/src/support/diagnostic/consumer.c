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
#include <emex64lib/support/diagnostic/consumer.h>

#define C_BOLD "\x1b[1m"
#define C_CARET "\x1b[0;1;32m"
#define C_FIXIT "\x1b[0;1;32m"
#define C_NOTE "\x1b[0;1;36m"
#define C_REMARK "\x1b[0;1;34m"
#define C_WARN "\x1b[0;1;35m"
#define C_ERROR "\x1b[0;1;31m"
#define C_FATAL "\x1b[0;1;31m"
#define C_RESET "\x1b[0m"

static void __diagnostic_consumer_consume_diagnostic_fallback_handler(diagnostic_consumer_t *consumer,
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

    /* dont forget to flush the toilet otherwise things get stinky */
    fflush(stderr);

    diagnostic_dealloc(diagnostic);
}

static diagnostic_consumer_t fallback_consumer = {
    .consume_handler = __diagnostic_consumer_consume_diagnostic_fallback_handler,
};

void diagnostic_report(diagnostic_consumer_t *consumer,
                       kDiagnosticSeverity severity,
                       diagnostic_location_t *location,
                       char *str,
                       ...)
{
    va_list args;
    va_start(args, str);
    diagnostic_t *diagnostic = diagnostic_allocv(severity, location, str, args);
    va_end(args);
    if(diagnostic != NULL)
    {
        if(consumer != NULL)
        {
            consumer->consume_handler(consumer, diagnostic);
        }
        else
        {
            fallback_consumer.consume_handler(&fallback_consumer, diagnostic);
        }
    }
    else
    {
        fprintf(stderr, "failed to allocate diagnostic\n");
    }
}
