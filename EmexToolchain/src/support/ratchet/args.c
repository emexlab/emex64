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

#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <EmexToolchain/support/ratchet/args.h>

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
