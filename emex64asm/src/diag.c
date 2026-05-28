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

#include <emex64asm/diag.h>

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

thread_local bool warning_error = false;

static inline int putchar_c(char c)
{
    return write(1, &c, 1);
}

static inline int putstr_c(char *s)
{
    int count = 0;

    if(!s)
    {
        s = "(null)";
    }

    while(*s)
    {
        count += write(1, s++, 1);
    }

    return count;
}

static inline int putnbr_base_unsigned(uint64_t n,
                                       char *base)
{
    int count = 0;
    uint64_t b = 0;

    while(base[b])
    {
        b++;
    }

    if(n >= b)
    {
        count += putnbr_base_unsigned(n / b, base);
    }

    count += putchar_c(base[n % b]);
    return count;
}

static inline int putnbr_signed(long n)
{
    int count = 0;

    if(n < 0)
    {
        count += putchar_c('-');
        n = -n;
    }

    count += putnbr_base_unsigned(n, "0123456789");

    return count;
}

static inline int put_binary(unsigned int n)
{
    return putnbr_base_unsigned(n, "01");
}

static inline int put_pointer(void *p)
{
    int count = 0;
    count += putstr_c("0x");
    count += putnbr_base_unsigned((uintptr_t)p, "0123456789abcdef");
    return count;
}

static inline int put_float(double n)
{
    int count = 0;
    long ipart = (long)n;
    double fpart = n - ipart;

    if(n < 0)
    {
        count += putchar_c('-');
        n = -n;
        ipart = -ipart;
        fpart = -fpart;
    }

    count += putnbr_signed(ipart);
    count += putchar_c('.');

    for(int i = 0; i < 6; i++)
    {
        fpart *= 10;
        count += putchar_c((int)fpart + '0');
        fpart -= (int)fpart;
    }

    return count;
}

static inline int handle_format(const char *fmt,
                                int *i,
                                va_list *args)
{
    int count = 0;

    /* clean handlinggg!! */
    switch(fmt[*i])
    {
        case 'c':
            count += putchar_c(va_arg(*args, int));
            break;
        case 's':
            count += putstr_c(va_arg(*args, char *));
            break;
        case 'd':
            count += putnbr_signed(va_arg(*args, int));
            break;
        case 'u':
            count += putnbr_base_unsigned(va_arg(*args, unsigned int), "0123456789");
            break;
        case 'b':
            count += put_binary(va_arg(*args, unsigned int));
            break;
        case 'x':
            count += putnbr_base_unsigned(va_arg(*args, unsigned int), "0123456789abcdef");
            break;
        case 'X':
            count += putnbr_base_unsigned(va_arg(*args, unsigned int), "0123456789ABCDEF");
            break;
        case 'p':
            count += put_pointer(va_arg(*args, void *));
            break;
        case 'f':
            count += put_float(va_arg(*args, double));
            break;
        case 'l':
            switch(fmt[*i + 1])
            {
                case 'd':
                    (*i)++;
                    count += putnbr_signed(va_arg(*args, long));
                    break;
                case 'u':
                    (*i)++;
                    count += putnbr_base_unsigned(va_arg(*args, unsigned long), "0123456789");
                    break;
                default:
                    break;
            }
            break;
        case '%':
            count += putchar_c('%');
            break;
        default:
            break;
    }

    return count;
}

static inline void diag_helper(const char *msg,
                               va_list *args)
{
    /* dont forget to flush the toilet otherwise things get stinky */
    fflush(stdout);

    /* starting to parse arguments */
    int i = 0;
    int count = 0;

    /* handling format */
    while(msg[i])
    {
        if(msg[i] == '%' && msg[i + 1])
        {
            i++;
            count += handle_format(msg, &i, args);
        }
        else
        {
            count += putchar_c(msg[i]);
        }
        i++;
    }
}

void diag_note(compiler_token_t *ct,
               const char *msg,
               ...)
{
    /* handling compiler token if passed */
    if(ct != NULL)
    {
        printf("%s:%zu:%zu: ", ct->cl->ci->file[ct->cl->file_idx].path, ct->cl->line_num, ct->column_num);
    }

    /* initial debug print */
    printf("\x1b[1m\033[35mnote:\033[0m\x1b[0m ");

    /* starting to parse arguments */
    va_list args;
    int i = 0;
    int count = 0;

    /* starting to parse */
    va_start(args, msg);

    /* invoking helper */
    diag_helper(msg, &args);

    /* ending the parse */
    va_end(args);
}

typedef enum {
    DIAG_WARN,
    DIAG_ERROR
} diag_level_t;

static void diag_vemit(diag_level_t level,
                       compiler_token_t *ct,
                       const char *msg,
                       va_list args)
{
    if(warning_error)
    {
        level = DIAG_ERROR;
    }

    if(ct != NULL)
    {
        printf("%s:%zu:%zu: ", ct->cl->ci->file[ct->cl->file_idx].path, ct->cl->line_num, ct->column_num);
    }

    switch(level)
    {
        case DIAG_WARN:
            printf("\x1b[1m\033[33mwarning:\033[0m\x1b[0m ");
            break;
        case DIAG_ERROR:
            printf("\x1b[1m\033[31merror:\033[0m\x1b[0m ");
            break;
    }

    diag_helper(msg, &args);

    if(level == DIAG_ERROR)
    {
        exit(1);
    }
}

void diag_warn(compiler_token_t *ct,
               const char *msg,
               ...)
{
    va_list args;

    va_start(args, msg);
    diag_vemit(DIAG_WARN, ct, msg, args);
    va_end(args);
}

void diag_error(compiler_token_t *ct,
                const char *msg,
                ...)
{
    va_list args;

    va_start(args, msg);
    diag_vemit(DIAG_ERROR, ct, msg, args);
    va_end(args);
}