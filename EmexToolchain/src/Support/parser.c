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
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <EmexToolchain/Support/parser.h>

static Boolean parse_type_is_buffer(const char *line,
                                    UInt64 *num,
                                    UInt64 *blen)
{
    /* checking if user specified value as character buffer */
    size_t len = strlen(line);
    if(len < 3 ||
       line[0] != '\"' ||
       line[len - 1] != '\"')
    {
        return false;
    }

    /* allocate temporary buffer */
    char *buf = malloc(len - 1);

    /* null pointer check */
    if(buf == NULL)
    {
        return false;
    }

    /* copying buffer byte for byte */
    size_t out = 0;
    for(size_t i = 1; i < len - 1; i++)
    {
        /* getting character at position */
        char c = line[i];

        /* checking for escape sequence */
        if(c == '\\')
        {
            /* sanity check */
            if(i + 1 >= len - 1)
            {
                free(buf);
                return false;
            }

            /* performing escape code check */
            char esc = line[++i];
            switch(esc)
            {
                case 'n':  buf[out++] = '\n'; break;
                case 't':  buf[out++] = '\t'; break;
                case 'r':  buf[out++] = '\r'; break;
                case 'b':  buf[out++] = '\b'; break;
                case '0':  buf[out++] = '\0'; break;
                case '\\': buf[out++] = '\\'; break;
                case '\'': buf[out++] = '\''; break;
                case '"': buf[out++] = '"'; break;
                default:
                    free(buf);
                    return false; /* unknown escape code */
            }
        }
        else
        {
            buf[out++] = c;
        }
    }

    /* nullterminating buffer */
    buf[out] = '\0';
    *blen = out;

    /* null pointer check */
    if(num != NULL)
    {
        *num = (unsigned long)buf;
    }

    return true;
}

parser_return_t parse_value_from_string(const char *str)
{
    UInt64 num = 0, len = 0;
    if(parse_type_is_buffer(str, &num, &len))
    {
        return (parser_return_t){ .type = emexParserValueTypeBuffer, .value = num, .len = len };
    }

    EFStringRef stringRef = EFStringCreateWithCString(kEFAllocatorDefault, str, kEFStringEncodingASCII);
    if(stringRef == NULL)
    {
        return (parser_return_t){ .type = emexParserValueTypeOverflow };
    }

    EFStringConvertibility convert = EFStringIsNumber(stringRef);
    if(convert == kEFStringConvertibilityNormal)
    {
        EFNumberRef numberRef = EFStringCopyNumber(kEFAllocatorDefault, stringRef);
        EFRelease(stringRef);
        if(numberRef == NULL)
        {
            return (parser_return_t){ .type = emexParserValueTypeOverflow };
        }

        if(!EFNumberGetValue(numberRef, kEFNumberTypeUInt64, &num))
        {
            EFRelease(numberRef);
            return (parser_return_t){ .type = emexParserValueTypeOverflow };
        }

        EFRelease(numberRef);
        return (parser_return_t){ .type = emexParserValueTypeNumber, .value = num, .len = len };
    }
    else
    {
        EFRelease(stringRef);
    }

    return (parser_return_t){ .type = emexParserValueTypeString };
}
