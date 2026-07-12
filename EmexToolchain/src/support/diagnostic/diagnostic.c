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
#include <EmexToolchain/support/diagnostic/diagnostic.h>
#include <EmexToolchain/support/diagnostic/consumer.h>

#define DIAG_BUF_SIZE 4096
 
typedef struct {
    char data[DIAG_BUF_SIZE];
    size_t len;
} diag_buf_t;
 
static inline int buf_putc(diag_buf_t *b, char c)
{
    b->data[b->len++] = c;
    return 1;
}
 
static inline int buf_puts(diag_buf_t *b, const char *s)
{
    int count = 0;
 
    if(!s)
    {
        s = "(null)";
    }
 
    while(*s)
    {
        count += buf_putc(b, *s++);
    }
 
    return count;
}
 
static inline int buf_putnbr_base_unsigned(diag_buf_t *b,
                                           UInt64 n,
                                           const char *base)
{
    int count = 0;
    UInt64 radix = 0;
 
    while(base[radix])
    {
        radix++;
    }
 
    if(n >= radix)
    {
        count += buf_putnbr_base_unsigned(b, n / radix, base);
    }
 
    count += buf_putc(b, base[n % radix]);
    return count;
}
 
static inline int buf_putnbr_signed(diag_buf_t *b, long n)
{
    int count = 0;
 
    if(n < 0)
    {
        count += buf_putc(b, '-');
        n = -n;
    }
 
    count += buf_putnbr_base_unsigned(b, (UInt64)n, "0123456789");
 
    return count;
}
 
static inline int buf_put_binary(diag_buf_t *b, unsigned int n)
{
    return buf_putnbr_base_unsigned(b, n, "01");
}
 
static inline int buf_put_pointer(diag_buf_t *b, void *p)
{
    int count = 0;
    count += buf_puts(b, "0x");
    count += buf_putnbr_base_unsigned(b, (uintptr_t)p, "0123456789abcdef");
    return count;
}
 
static inline int buf_put_float(diag_buf_t *b, double n)
{
    int count = 0;
    long ipart = (long)n;
    double fpart = n - ipart;
 
    if(n < 0)
    {
        count += buf_putc(b, '-');
        n = -n;
        ipart = -ipart;
        fpart = -fpart;
    }
 
    count += buf_putnbr_signed(b, ipart);
    count += buf_putc(b, '.');
 
    for(int i = 0; i < 6; i++)
    {
        fpart *= 10;
        count += buf_putc(b, (int)fpart + '0');
        fpart -= (int)fpart;
    }
 
    return count;
}

diagnostic_t *diagnostic_allocv(kDiagnosticSeverity severity,
                                diagnostic_location_t *location,
                                const char *str,
                                va_list args)
{
    assert(str != NULL);

    diag_buf_t buf;
    buf.len = 0;

    int i = 0;
    while(str[i])
    {
        if(str[i] == '%' && str[i + 1])
        {
            i++;
            switch(str[i])
            {
                case 'c':
                    buf_putc(&buf, (char)va_arg(args, int));
                    break;
                case 's':
                    buf_puts(&buf, va_arg(args, char *));
                    break;
                case 'd':
                    buf_putnbr_signed(&buf, va_arg(args, int));
                    break;
                case 'u':
                    buf_putnbr_base_unsigned(&buf, va_arg(args, unsigned int), "0123456789");
                    break;
                case 'b':
                    buf_put_binary(&buf, va_arg(args, unsigned int));
                    break;
                case 'x':
                    buf_putnbr_base_unsigned(&buf, va_arg(args, unsigned int), "0123456789abcdef");
                    break;
                case 'X':
                    buf_putnbr_base_unsigned(&buf, va_arg(args, unsigned int), "0123456789ABCDEF");
                    break;
                case 'p':
                    buf_put_pointer(&buf, va_arg(args, void *));
                    break;
                case 'f':
                    buf_put_float(&buf, va_arg(args, double));
                    break;
                case 'l':
                    switch(str[i + 1])
                    {
                        case 'l':
                            switch(str[i + 2])
                            {
                                case 'd':
                                    i += 2;
                                    buf_putnbr_signed(&buf, va_arg(args, SInt64));
                                    break;
                                case 'u':
                                    i += 2;
                                    buf_putnbr_base_unsigned(&buf, va_arg(args, UInt64), "0123456789");
                                    break;
                                default:
                                    break;
                            }
                            break;
                        case 'd':
                            i++;
                            buf_putnbr_signed(&buf, va_arg(args, long));
                            break;
                        case 'u':
                            i++;
                            buf_putnbr_base_unsigned(&buf, va_arg(args, unsigned long), "0123456789");
                            break;
                        default:
                            break;
                    }
                    break;
                case '%':
                    buf_putc(&buf, '%');
                    break;
                default:
                    break;
            }
        }
        else
        {
            buf_putc(&buf, str[i]);
        }
        i++;
    }
    buf_putc(&buf, '\0');

    diagnostic_t *diagnostic = malloc(sizeof(diagnostic_t));
    if(diagnostic == NULL)
    {
        return NULL;
    }

    diagnostic->str = strdup(buf.data);
    if(diagnostic->str == NULL)
    {
        free(diagnostic);
        return NULL;
    }

    if(location != NULL)
    {
        diagnostic->location = malloc(sizeof(diagnostic_location_t));
        if(diagnostic->location == NULL)
        {
            free(diagnostic->str);
            free(diagnostic);
            return NULL;
        }

        diagnostic->location->file_name = strdup(location->file_name);
        if(diagnostic->location->file_name == NULL)
        {
            free(diagnostic->str);
            free(diagnostic->location);
            free(diagnostic);
            return NULL;
        }

        diagnostic->location->line = strdup(location->line);
        if(diagnostic->location->line == NULL)
        {
            free(diagnostic->location->file_name);
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
        free(diagnostic->location->file_name);
        free(diagnostic->location->line);
        free(diagnostic->location);
    }

    free(diagnostic->str);
    free(diagnostic);
}
