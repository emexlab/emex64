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

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include <emex64lib/support/diagnostic/legacy.h>
#include <emex64lib/asm/invocation.h>

_Thread_local bool warning_error = false;
_Thread_local bool caret_diagnostics = true;

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

static void diag_print_caret_line(assembler_token_t *at)
{
    if(at == NULL || at->al == NULL || at->al->str == NULL)
    {
        return;
    }

    const char *src = at->al->str;
    size_t line_num = at->al->line_num;
    size_t start = (at->column_num > 0) ? at->column_num - 1 : 0;
    size_t tok_len = 0;
    if(at->str != NULL)
    {
        while(at->str[tok_len])
        {
            tok_len++;
        }
    }
    if(tok_len == 0)
    {
        tok_len = 1;
    }

    size_t n = line_num;
    int ndigits = 1;
    while(n >= 10)
    {
        n /= 10; ndigits++;
    }
    int w = ndigits + 3;

    printf("%*zu | %s\n", w, line_num, src);

    for(int i = 0; i < w + 1; i++)
    {
        putchar(' ');
    }
    printf("| ");

    for(size_t i = 0; i < start && src[i] != '\0'; i++)
    {
        putchar(src[i] == '\t' ? '\t' : ' ');
    }

    printf("\x1b[1m\x1b[32m^");
    for(size_t i = 1; i < tok_len; i++)
    {
        putchar('~');
    }
    printf("\x1b[0m\n");
}

void diag_log(diag_level_t level,
              assembler_token_t *at,
              const char *msg,
              ...)
{
    if(warning_error)
    {
        level = DIAG_ERROR;
    }

    if(at != NULL)
    {
        printf("%s:%zu:%zu: ", at->al->inv->file[at->al->file_idx], at->al->line_num, at->column_num);
    }

    switch(level)
    {
        case DIAG_NOTE:
            printf("\x1b[1m\033[35mnote:");
            break;
        case DIAG_WARN:
            printf("\x1b[1m\033[33mwarning:");
            break;
        case DIAG_ERROR:
            printf("\x1b[1m\033[31merror:");
            break;
        case DIAG_FATAL:
            printf("\x1b[1m\033[31mfatal:");
            break;
    }
    printf("\033[0m\x1b[0m ");

    va_list args;
    va_start(args, msg);

    /* dont forget to flush the toilet otherwise things get stinky */
    fflush(stdout);

    /* starting to parse arguments */
    int i = 0;
    while(msg[i])
    {
        if(msg[i] == '%' && msg[i + 1])
        {
            i++;
            /* clean handlinggg!! */
            switch(msg[i])
            {
                case 'c':
                    putchar_c(va_arg(args, int));
                    break;
                case 's':
                    putstr_c(va_arg(args, char *));
                    break;
                case 'd':
                    putnbr_signed(va_arg(args, int));
                    break;
                case 'u':
                    putnbr_base_unsigned(va_arg(args, unsigned int), "0123456789");
                    break;
                case 'b':
                    put_binary(va_arg(args, unsigned int));
                    break;
                case 'x':
                    putnbr_base_unsigned(va_arg(args, unsigned int), "0123456789abcdef");
                    break;
                case 'X':
                    putnbr_base_unsigned(va_arg(args, unsigned int), "0123456789ABCDEF");
                    break;
                case 'p':
                    put_pointer(va_arg(args, void *));
                    break;
                case 'f':
                    put_float(va_arg(args, double));
                    break;
                case 'l':
                    switch(msg[i + 1])
                    {
                        case 'd':
                            i++;
                            putnbr_signed(va_arg(args, long));
                            break;
                        case 'u':
                            i++;
                            putnbr_base_unsigned(va_arg(args, unsigned long), "0123456789");
                            break;
                        default:
                            break;
                    }
                    break;
                case '%':
                    putchar_c('%');
                    break;
                default:
                    break;
            }

        }
        else
        {
            putchar_c(msg[i]);
        }
        i++;
    }

    va_end(args);

    if(at != NULL && caret_diagnostics)
    {
        diag_print_caret_line(at);
    }
    fflush(stdout);
}
