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
#include <strings.h>
#include <emex64lib/support/ratchet/args.h>

void ratchet_args_init(ratchet_args_t *ra)
{
    bzero(ra, sizeof(ratchet_args_t));
}

void ratchet_args_deinit(ratchet_args_t *ra)
{
    for(size_t i = 0; i < ra->argc; i++)
    {
        /* free usually accepts null pointers */
        free(ra->args[i]);
    }
    free(ra->args);

    /* making sure it is entirely blank */
    bzero(ra, sizeof(ratchet_args_t));
}

static inline bool __ratchet_args_gib(ratchet_args_t *ra)
{
    if(ra->argc < ra->csize)
    {
        /* still enough size */
        return true;
    }

    size_t old_csize = ra->csize;
    size_t new_csize = old_csize + 100;
    if(ra->args == NULL)
    {
        ra->args = calloc(new_csize, sizeof(char*));
        if(ra->args == NULL)
        {
            return false;
        }
    }
    else
    {
        void *newp = realloc(ra->args, new_csize * sizeof(char*));
        if(newp == NULL)
        {
            return false;
        }
        ra->args = newp;
    }
    ra->csize = new_csize;

    return true;
}

void ratchet_args_append(ratchet_args_t *ra,
                         const char *arg)
{
    if(ra->failed || arg == NULL)
    {
        return;
    }

    if(!__ratchet_args_gib(ra))
    {
        ra->failed = true;
        return;
    }

    char *newp = strdup(arg);
    if(newp == NULL)
    {
        ra->failed = true;
        return;
    }

    ra->args[ra->argc++] = newp;
}
