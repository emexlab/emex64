/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <emexutils/parser.h>

static bool parse_type_is_hex(const char *line,
                              uint64_t *num)
{
    /* checking if user specified it as type hexadecimal  */
    if(line[0] != '0' || (line[1] != 'x' && line[1] != 'X')) return false;

    /* nect check is to make sure if the string really is a hexadecimal  */
    for(uint64_t i = 2;; i++)
    {
        if(line[i] == '\0')
        {
            if(num != NULL)
            {
                *num = strtoul(line + 2, NULL, 16);
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

static bool parse_type_is_bin(const char *line,
                              uint64_t *num)
{
    /* checking if used specified it as a type binary */
    if(line[0] != '0' || (line[1] != 'b' && line[1] != 'B')) return false;

    /* checking if rest of the string complies to a binary */
    for(uint64_t i = 2;; i++)
    {
        if(line[i] == '\0')
        {
            if(num != NULL)
            {
                *num = strtoul(line + 2, NULL, 2);
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

static bool parse_type_is_dec(const char *line,
                              uint64_t *num)
{
    /* checking if string complies to a decimal */
    for(uint64_t i = 0;; i++)
    {
        if(line[i] == '\0')
        {
            if(num != NULL)
            {
                *num = strtoul(line, NULL, 10);
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

static bool parse_type_is_char(const char *line,
                               uint64_t *num)
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
        *num = (uint64_t)c;
    }

    return true;
}

static bool parse_type_is_buffer(const char *line,
                                 uint64_t *num,
                                 uint64_t *blen)
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

/*
 * Main parser
 */
inline static parser_value_type_t parse_type(const char *line,
                                             uint64_t *fastdec,
                                             uint64_t *len)
{
    if(parse_type_is_hex(line, fastdec) ||
       parse_type_is_bin(line, fastdec) ||
       parse_type_is_dec(line, fastdec) ||
       parse_type_is_char(line, fastdec))
    {
        return emexParserValueTypeNumber;
    }
    else if(parse_type_is_buffer(line, fastdec, len))
    {
        return emexParserValueTypeBuffer;
    }
    return emexParserValueTypeString;
}

parser_return_t parse_value_from_string(const char *s)
{
    parser_return_t pr;
    pr.type = parse_type(s, &(pr.value), &(pr.len));
    return pr;
}
