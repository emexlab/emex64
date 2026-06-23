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
#include <emex64lib/support/diagnostic/storage.h>
#include <emex64lib/support/diagnostic/log.h>

diagnostic_storage_t *diagnostic_storage_alloc()
{
    diagnostic_storage_t *storage = malloc(sizeof(diagnostic_storage_t));
    storage->tail = NULL;
    storage->most_severe_occured_message_type = kDiagnosticMessageTypeNote;
    return storage;
}

void diagnostic_storage_dealloc(diagnostic_storage_t *storage)
{
    if(storage == NULL)
    {
        return;
    }

    /* we need to find the head */
    diagnostic_message_t *head = storage->tail;
    for(;;)
    {
        if(head == NULL || head->prev == NULL)
        {
            break;
        }

        head = head->prev;
    }

    while(head != NULL)
    {
        diagnostic_message_t *next = head->next;
        diagnostic_message_dealloc(head);
        head = next;
    }

    /* now all messages are dead */
    free(storage);
}

static inline void __diagnostic_storage_append_tail(diagnostic_storage_t *storage,
                                                    diagnostic_message_t *message)
{
    if(storage->tail == NULL)
    {
        storage->tail = message;
    }
    else
    {
        /* gimme a needle and I stitch that linked list >:3 */
        storage->tail->next = message;
        message->prev = storage->tail;
        storage->tail = message;
    }
}

void diagnostic_storage_append_internal(diagnostic_storage_t *storage,
                                        kDiagnosticMessageType type,
                                        char *msg)
{
    if(storage->most_severe_occured_message_type < type)
    {
        storage->most_severe_occured_message_type = type;
    }

    diagnostic_message_t *next = diagnostic_message_alloc_internal(type, msg);
    if(next == NULL)
    {
        diag_fatal(NULL, "coudln't allocate diagnostic message\n");
        return;
    }
    __diagnostic_storage_append_tail(storage, next);
}

void diagnostic_storage_append_file(diagnostic_storage_t *storage,
                                    kDiagnosticMessageType type,
                                    char *file,
                                    char *msg,
                                    uint64_t ln,
                                    uint64_t col)
{
    if(storage->most_severe_occured_message_type < type)
    {
        storage->most_severe_occured_message_type = type;
    }
    
    diagnostic_message_t *next = diagnostic_message_alloc_file(type, file, msg, ln, col);
    if(next == NULL)
    {
        diag_fatal(NULL, "coudln't allocate diagnostic message\n");
        return;
    }
    __diagnostic_storage_append_tail(storage, next);
}
