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

#include <strings.h>
#include <EmexToolchain/Support/ratchet/args.h>

Boolean ratchet_args_init(ratchet_args_t *ra)
{
    bzero(ra, sizeof(ratchet_args_t));
    ra->array = EFArrayCreateMutable(kEFAllocatorDefault, kEFArrayCallbacksObjectCallbacks, 0);
    return ra->array != NULL;
}

void ratchet_args_deinit(ratchet_args_t *ra)
{
    EFRelease(ra->array);
    bzero(ra, sizeof(ratchet_args_t));
}

void ratchet_args_append(ratchet_args_t *ra,
                         const char *arg)
{
    if(ra->failed || arg == NULL || ra->array == NULL)
    {
        ra->failed = true;  /* when arg is NULL then it is automatically failed */
        return;
    }

    EFStringRef argument = EFStringCreateWithCString(kEFAllocatorDefault, arg, kEFStringEncodingASCII);
    if(argument == NULL)
    {
        ra->failed = true;
        return;
    }

    Boolean success = EFArrayAppendValue(ra->array, argument);
    EFRelease(argument);
    if(!success)
    {
        ra->failed = true;
        return;
    }
}
