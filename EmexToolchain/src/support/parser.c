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
#include <EmexToolchain/support/parser.h>

static _Thread_local jmp_buf overflow_jmp;

static Boolean parse_base(const char *digits,
                       int base,
                       UInt64 *num)
{
    errno = 0;
    UInt64 v = strtoull(digits, NULL, base);
    if(errno == ERANGE)
    {
        return false;
    }
    if(num != NULL)
    {
        *num = v;
    }
    return true;
}

static Boolean parse_type_is_hex(const char *line,
                              UInt64 *num)
{
    /* checking if user specified it as type hexadecimal  */
    if(line[0] != '0' || (line[1] != 'x' && line[1] != 'X')) return false;

    /* next check is to make sure if the string really is a hexadecimal  */
    for(UInt64 i = 2;; i++)
    {
        if(line[i] == '\0')
        {
            if(!parse_base(line + 2, 16, num))
            {
                longjmp(overflow_jmp, 1);
            }
            return true;
        }

        if((line[i] < '0' || line[i] > '9') &&
           (line[i] < 'a' || line[i] > 'f') &&
           (line[i] < 'A' || line[i] > 'F'))
        {
            return false;
        }
    }

    return false;
}

static Boolean parse_type_is_bin(const char *line,
                              UInt64 *num)
{
    /* checking if used specified it as a type binary */
    if(line[0] != '0' || (line[1] != 'b' && line[1] != 'B')) return false;

    /* checking if rest of the string complies to a binary */
    for(UInt64 i = 2;; i++)
    {
        if(line[i] == '\0')
        {
            if(!parse_base(line + 2, 2, num))
            {
                longjmp(overflow_jmp, 1);
            }
            return true;
        }

        if(line[i] < '0' || line[i] > '1')
        {
            return false;
        }
    }

    return false;
}

static Boolean parse_type_is_dec(const char *line,
                              UInt64 *num)
{
    /* checking if string complies to a decimal */
    for(UInt64 i = 0;; i++)
    {
        if(line[i] == '\0')
        {
            if(!parse_base(line, 10, num))
            {
                longjmp(overflow_jmp, 1);
            }
            return true;
        }

        if(line[i] < '0' || line[i] > '9')
        {
            return false;
        }
    }

    // If it passed all its a hexadecimal string
    return false;
}

static Boolean parse_type_is_char(const char *line,
                               UInt64 *num)
{
    /* checking if this is a string */
    if(line[0] != '\'' || line[2] == '\0')
    {
        return false;
    }

    /* finding closed quote */
    size_t len = strlen(line);
    if(len < 3 || line[len - 1] != '\'')
    {
        return false;
    }

    char c;

    /* checking if user specified it as normal or special character */
    if(line[1] == '\\')
    {
        switch(line[2])
        {
            case 'n': c = '\n'; break;
            case 't': c = '\t'; break;
            case 'r': c = '\r'; break;
            case 'b': c = '\b'; break;
            case '0': c = '\0'; break;
            case '\\': c = '\\'; break;
            case '\'': c = '\''; break;
            default: return false;
        }
    }
    else
    {
        if(len != 3)
        {
            /* uhm whats up?! */
            return false;
        }
        c = line[1];
    }

    if(num != NULL)
    {
        *num = (UInt64)c;
    }

    return true;
}

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
    if(setjmp(overflow_jmp) != 0)
    {
        return (parser_return_t){ .type = emexParserValueTypeOverflow };
    }

    UInt64 num = 0, len = 0;

    if(parse_type_is_hex(str, &num) ||
       parse_type_is_bin(str, &num) ||
       parse_type_is_dec(str, &num) ||
       parse_type_is_char(str, &num))
    {
        return (parser_return_t){ .type = emexParserValueTypeNumber, .value = num, .len = len };
    }
    else if(parse_type_is_buffer(str, &num, &len))
    {
        return (parser_return_t){ .type = emexParserValueTypeBuffer, .value = num, .len = len };
    }

    return (parser_return_t){ .type = emexParserValueTypeString };
}
