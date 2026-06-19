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
#include <string.h>
#include <emex64lib/support/diagnostic/message.h>

diagnostic_message_t *diagnostic_message_alloc_internal(kDiagnosticMessageType type,
                                                        char *msg)
{
    diagnostic_message_t *message = malloc(sizeof(diagnostic_message_t));
    if(message == NULL)
    {
        return NULL;
    }

    message->source.type = kDiagnosticMessageSourceTypeInternal;
    message->type = type;
    message->str = strdup(msg);
    if(message->str == NULL)
    {
        free(message);
        return NULL;
    }

    message->prev = NULL;
    message->next = NULL;

    return NULL;
}

diagnostic_message_t *diagnostic_message_alloc_file(kDiagnosticMessageType type,
                                                    char *file,
                                                    char *msg,
                                                    uint64_t ln,
                                                    uint64_t col)
{
    diagnostic_message_t *message = malloc(sizeof(diagnostic_message_t));
    if(message == NULL)
    {
        return NULL;
    }

    message->source.type = kDiagnosticMessageSourceTypeFile;
    message->type = type;
    message->str = strdup(msg);
    if(message->str == NULL)
    {
        free(message);
        return NULL;
    }

    message->source.file.file = strdup(file);
    message->source.file.ln = ln;
    message->source.file.col = col;
    if(message->source.file.file == NULL)
    {
        free(message->str);
        free(message);
        return NULL;
    }

    message->prev = NULL;
    message->next = NULL;

    return NULL;
}

void diagnostic_message_dealloc(diagnostic_message_t *message)
{
    switch(message->source.type)
    {
        case kDiagnosticMessageSourceTypeFile:
            free(message->source.file.file);
            [[fallthrough]];
        case kDiagnosticMessageSourceTypeInternal:
        default:
            break;
    }

    free(message);
}
