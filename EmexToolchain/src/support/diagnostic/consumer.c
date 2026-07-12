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
#include <inttypes.h>
#include <EmexToolchain/support/diagnostic/consumer.h>

static void __diagnostic_consumer_consume_diagnostic_fallback_handler(diagnostic_consumer_t *consumer,
                                                                      diagnostic_t *diagnostic)
{
    if(diagnostic->location != NULL)
    {
        fprintf(stderr, "%s:%" PRIu64 ":%" PRIu64 ":", diagnostic->location->file_name, diagnostic->location->ln, diagnostic->location->col);
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
                       const char *str,
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
