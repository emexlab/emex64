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
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/diagnostic/diagnostic.h>
#include <EmexToolchain/Support/diagnostic/consumer.h>

diagnostic_t *diagnostic_allocv(kDiagnosticSeverity severity,
                                diagnostic_location_t *location,
                                const char *str,
                                va_list args)
{
    assert(str != NULL);

    EFStringRef formatString = EFStringCreateWithCString(kEFAllocatorDefault, str, kEFStringEncodingASCII);
    if(formatString == NULL)
    {
        return NULL;
    }

    EFStringRef string = EFStringCreateWithFormatAndArguments(kEFAllocatorDefault, formatString, args);
    EFRelease(formatString);
    if(string == NULL)
    {
        return NULL;
    }

    EFIndex length = EFStringGetLength(string);
    char *newStringBuffer = malloc((EFSize)(length + 1));
    Boolean success = EFStringGetCString(string, newStringBuffer, length + 1, kEFStringEncodingASCII);
    EFRelease(string);
    if(!success)
    {
        free(newStringBuffer);
        return NULL;
    }

    diagnostic_t *diagnostic = malloc(sizeof(diagnostic_t));
    if(diagnostic == NULL)
    {
        free(newStringBuffer);
        return NULL;
    }

    diagnostic->str = newStringBuffer;

    if(location != NULL)
    {
        diagnostic->location = malloc(sizeof(diagnostic_location_t));
        if(diagnostic->location == NULL)
        {
            free(diagnostic->str);
            free(diagnostic);
            return NULL;
        }

        diagnostic->location->fileURL = EFRetain(location->fileURL);
        if(diagnostic->location->fileURL == NULL)
        {
            free(diagnostic->str);
            free(diagnostic->location);
            free(diagnostic);
            return NULL;
        }

        diagnostic->location->line = strdup(location->line);
        if(diagnostic->location->line == NULL)
        {
            EFRelease(diagnostic->location->fileURL);
            free(diagnostic->str);
            free(diagnostic->location);
            free(diagnostic);
            return NULL;
        }

        diagnostic->location->ln = location->ln;
        diagnostic->location->col = location->col;
        diagnostic->location->range = location->range;
    }
    else
    {
        diagnostic->location = NULL;
    }

    diagnostic->severity = severity;

    return diagnostic;
}

diagnostic_t *diagnostic_alloc(kDiagnosticSeverity severity,
                               diagnostic_location_t *location,
                               const char *str,
                               ...)
{
    va_list args;
    va_start(args, str);
    diagnostic_t *d = diagnostic_allocv(severity, location, str, args);
    va_end(args);
    return d;
}

void diagnostic_dealloc(diagnostic_t *diagnostic)
{
    if(diagnostic == NULL)
    {
        return;
    }

    if(diagnostic->location != NULL)
    {
        EFRelease(diagnostic->location->fileURL);
        free(diagnostic->location->line);
        free(diagnostic->location);
    }

    free(diagnostic->str);
    free(diagnostic);
}
