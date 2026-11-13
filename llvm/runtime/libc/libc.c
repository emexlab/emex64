#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <emex64.h>

int putchar(int c)
{
    while(!(emex64_mmio_read(EMEX64_UART_REG_STATUS) & EMEX64_UART_STATUS_TX_EMPTY));
    emex64_mmio_write(EMEX64_UART_REG_DATA, (unsigned char)c);
    return (unsigned char)c;
}

int getchar(void)
{
    while(!(emex64_mmio_read(EMEX64_UART_REG_STATUS) & EMEX64_UART_STATUS_RX_READY));
    return (int)(emex64_mmio_read(EMEX64_UART_REG_DATA) & 0xff);
}

int puts(const char *s)
{
    while(*s) putchar(*s++);
    putchar('\n');
    return 0;
}

struct __emex64_file { int fd; };
static FILE file_stdout = { 1 }, file_stderr = { 2 };
FILE *stdout = &file_stdout, *stderr = &file_stderr;

int fputc(int c, FILE *f) { (void)f; return putchar(c); }
int fputs(const char *s, FILE *f) { (void)f; while (*s) putchar(*s++); return 0; }
int fflush(FILE *f) { (void)f; return 0; }

long write(int fd, const void *buf, unsigned long n)
{
    const char *p = buf;
    unsigned long i;
    for(i = 0; i < n; i++) putchar(p[i]);
    return (long)n;
}

static void (*atexit_fns[32])(void);
static int atexit_count;

int atexit(void (*fn)(void))
{
    if(atexit_count >= 32) return -1;
    atexit_fns[atexit_count++] = fn;
    return 0;
}

void exit(int code)
{
    while(atexit_count > 0) atexit_fns[--atexit_count]();
    emex64_mmio_write(EMEX64_PLATFORM_REG_PWR, 0);
    for(;;);
}

void __assert_fail(const char *expr, const char *file, int line)
{
    printf("%s:%d: assertion failed: %s\n", file, line, expr);
    abort();
}

void abort(void)
{
    puts("abort()");
    exit(134);
}

int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *p = a, *q = b;
    for(; n; n--, p++, q++) if(*p != *q) return *p - *q;
    return 0;
}

void *memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = s;
    for(; n; n--, p++) if (*p == (unsigned char)c) return (void *)p;
    return NULL;
}

size_t strlen(const char *s)
{
    const char *p = s;
    while(*p) p++;
    return p - s;
}

int strcmp(const char *a, const char *b)
{
    while(*a && *a == *b) a++, b++;
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n)
{
    for(; n; n--, a++, b++)
    {
        if(*a != *b) return (unsigned char)*a - (unsigned char)*b;
        if(!*a) break;
    }
    return 0;
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;
    while((*d++ = *src++));
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
    char *d = dst;
    for(; n && *src; n--) *d++ = *src++;
    for(; n; n--) *d++ = 0;
    return dst;
}

char *strcat(char *dst, const char *src)
{
    strcpy(dst + strlen(dst), src);
    return dst;
}

char *strncat(char *dst, const char *src, size_t n)
{
    char *d = dst + strlen(dst);
    while(n-- && *src) *d++ = *src++;
    *d = 0;
    return dst;
}

char *strchr(const char *s, int c)
{
    for(;; s++)
    {
        if(*s == (char)c) return (char *)s;
        if(!*s) return NULL;
    }
}

char *strrchr(const char *s, int c)
{
    const char *r = NULL;
    for(;; s++)
    {
        if(*s == (char)c) r = s;
        if(!*s) return (char *)r;
    }
}

char *strstr(const char *h, const char *n)
{
    size_t l = strlen(n);
    for(; *h; h++) if(!strncmp(h, n, l)) return (char *)h;
    return l ? NULL : (char *)h;
}

char *strdup(const char *s)
{
    size_t l = strlen(s) + 1;
    char *p = malloc(l);
    if(p) memcpy(p, s, l);
    return p;
}

int abs(int x) { return x < 0 ? -x : x; }
long labs(long x) { return x < 0 ? -x : x; }
long long llabs(long long x) { return x < 0 ? -x : x; }

unsigned long strtoul(const char *s, char **end, int base)
{
    unsigned long v = 0;
    int neg = 0, d;
    while(*s == ' ' || (*s >= '\t' && *s <= '\r')) s++;
    if(*s == '-' || *s == '+') neg = *s++ == '-';
    if((base == 0 || base == 16) && s[0] == '0' && (s[1] | 32) == 'x') s += 2, base = 16;
    else if(base == 0) base = *s == '0' ? 8 : 10;
    for(;; s++)
    {
        if(*s >= '0' && *s <= '9') d = *s - '0';
        else if((*s | 32) >= 'a' && (*s | 32) <= 'z') d = (*s | 32) - 'a' + 10;
        else break;
        if(d >= base) break;
        v = v * base + d;
    }
    if(end) *end = (char *)s;
    return neg ? -v : v;
}

long strtol(const char *s, char **end, int base)
{
    return (long)strtoul(s, end, base);
}

double strtod(const char *s, char **end)
{
    unsigned long m = 0;
    int neg = 0, e = 0, digits = 0, es = 0, ev = 0;
    const char *p = s;
    double r, p10 = 10;
    while(*p == ' ' || (*p >= '\t' && *p <= '\r')) p++;
    if(*p == '-' || *p == '+') neg = *p++ == '-';
    for(; *p >= '0' && *p <= '9'; p++, digits++)
    {
        if(m < 1000000000000000000UL) m = m * 10 + (*p - '0');
        else e++;
    }
    if(*p == '.')
    {
        for(p++; *p >= '0' && *p <= '9'; p++, digits++)
        {
            if(m < 1000000000000000000UL) m = m * 10 + (*p - '0'), e--;
        }
    }
    if(!digits)
    {
        if(end) *end = (char *)s;
        return 0;
    }
    if((*p | 32) == 'e')
    {
        const char *q = p + 1;
        if(*q == '-' || *q == '+') es = *q++ == '-';
        if(*q >= '0' && *q <= '9')
        {
            for(; *q >= '0' && *q <= '9'; q++) ev = ev * 10 + (*q - '0');
            p = q;
            e += es ? -ev : ev;
        }
    }
    r = (double)m;
    if(e < 0)
    {
        for(ev = -e; ev; ev >>= 1, p10 *= p10) if(ev & 1) r /= p10;
    }
    else
    {
        for(ev = e; ev; ev >>= 1, p10 *= p10) if(ev & 1) r *= p10;
    }
    if(end) *end = (char *)p;
    return neg ? -r : r;
}

float strtof(const char *s, char **end) { return (float)strtod(s, end); }
long double strtold(const char *s, char **end) { return strtod(s, end); }
double atof(const char *s) { return strtod(s, NULL); }

int atoi(const char *s) { return (int)strtol(s, NULL, 10); }
long atol(const char *s) { return strtol(s, NULL, 10); }

static unsigned long rand_state = 1;

void srand(unsigned seed)
{
    rand_state = seed;
}

int rand(void)
{
    rand_state = rand_state * 6364136223846793005UL + 1442695040888963407UL;
    return (int)(rand_state >> 33);
}

void qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *))
{
    char *b = base, tmp[256];
    size_t i, j;
    if(size > sizeof tmp) abort();
    for(i = 1; i < n; i++)
    {
        memcpy(tmp, b + i * size, size);
        for(j = i; j > 0 && cmp(b + (j - 1) * size, tmp) > 0; j--) memcpy(b + j * size, b + (j - 1) * size, size);
        memcpy(b + j * size, tmp, size);
    }
}

#ifndef EMEX64_HEAP_SIZE
#define EMEX64_HEAP_SIZE (1024 * 1024)
#endif

typedef struct block {
    size_t size;
    struct block *next;
} block_t;

static unsigned long heap[EMEX64_HEAP_SIZE / sizeof(unsigned long)];
static size_t heap_used;
static block_t *free_list;

void *malloc(size_t n)
{
    block_t **pp, *b;
    n = (n + 15) & ~(size_t)15;
    for(pp = &free_list; (b = *pp); pp = &b->next)
    {
        if(b->size >= n)
        {
            *pp = b->next;
            return b + 1;
        }
    }
    if(heap_used + sizeof(block_t) + n > sizeof heap) return NULL;
    b = (block_t *)((char *)heap + heap_used);
    heap_used += sizeof(block_t) + n;
    b->size = n;
    return b + 1;
}

void free(void *p)
{
    block_t *b;
    if(!p) return;
    b = (block_t *)p - 1;
    b->next = free_list;
    free_list = b;
}

void *calloc(size_t n, size_t m)
{
    void *p = malloc(n * m);
    if(p) memset(p, 0, n * m);
    return p;
}

void *realloc(void *p, size_t n)
{
    void *q;
    if(!p) return malloc(n);
    if(((block_t *)p - 1)->size >= n) return p;
    q = malloc(n);
    if(q)
    {
        memcpy(q, p, ((block_t *)p - 1)->size);
        free(p);
    }
    return q;
}

typedef struct {
    char *buf;
    size_t size, len;
} out_t;

static void out_c(out_t *o, char c)
{
    if(o->size == 0) putchar(c);
    else if(o->len + 1 < o->size) o->buf[o->len] = c;
    o->len++;
}

static void out_pad(out_t *o, char c, int n)
{
    while (n-- > 0)
        out_c(o, c);
}

static void out_field(out_t *o, const char *prefix, const char *digits, int len, int width, int prec, int left, char padc)
{
    int plen = strlen(prefix);
    int zeros = prec > len ? prec - len : 0;
    int total = plen + zeros + len;
    if(!left && padc == ' ') out_pad(o, ' ', width - total);
    while(*prefix) out_c(o, *prefix++);
    if(!left && padc == '0') out_pad(o, '0', width - total);
    out_pad(o, '0', zeros);
    while(len--) out_c(o, *digits++);
    if(left) out_pad(o, ' ', width - total);
}

static int utoa_rev(unsigned long v, unsigned base, int upper, char *tmp)
{
    const char *dig = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    int n = 0;
    do {
        tmp[n++] = dig[v % base];
        v /= base;
    } while (v);
    return n;
}

static int dtoa_digits(double d, int n, char *dig, int *exp10)
{
    int e = 0, i;
    unsigned long m, lim = 1;
    if(n > 17) n = 17;
    if(d == 0)
    {
        for(i = 0; i < n; i++) dig[i] = '0';
        *exp10 = 1;
        return n;
    }
    while(d >= 1e16) d /= 1e16, e += 16;
    while(d >= 10) d /= 10, e++;
    while(d < 1e-16) d *= 1e16, e -= 16;
    while(d < 1) d *= 10, e--;
    for(i = 1; i < n; i++) d *= 10, lim *= 10;
    m = (unsigned long)(d + 0.5);
    if(m >= lim * 10) m /= 10, e++;
    for(i = n - 1; i >= 0; i--) dig[i] = '0' + m % 10, m /= 10;
    *exp10 = e + 1;
    return n;
}

static void out_double(out_t *o, double d, int prec, int width, int left, char padc, const char *sign, int conv)
{
    char dig[24], buf[400];
    char pre[2] = { 0, 0 };
    int len = 0, e10, nd, i, upper = conv == 'E' || conv == 'G';
    int strip = 0;
    if(d != d)
    {
        out_field(o, "", upper ? "NAN" : "nan", 3, width, 0, left, ' ');
        return;
    }
    if(d < 0 || (d == 0 && 1 / d < 0))
    {
        pre[0] = '-';
        d = -d;
    }
    else if (*sign)
    {
        pre[0] = *sign;
    }
    if(d > 1.7976931348623157e308)
    {
        out_field(o, pre, upper ? "INF" : "inf", 3, width, 0, left, ' ');
        return;
    }
    if(prec < 0) prec = 6;
    if(conv == 'g' || conv == 'G')
    {
        if(prec == 0) prec = 1;
        dtoa_digits(d, prec, dig, &e10);
        if(e10 - 1 < -4 || e10 - 1 >= prec)
        {
            conv = upper ? 'E' : 'e';
            prec = prec - 1;
        }
        else
        {
            conv = 'f';
            prec = prec - e10;
        }
        strip = 1;
    }
    if((conv == 'f' || conv == 'F') && d < 9.0e18 && prec <= 18)
    {
        unsigned long ip = (unsigned long)d, fp, scale = 1;
        double frac = d - (double)ip;
        char tmp[24];
        int n;
        for(i = 0; i < prec; i++) scale *= 10;
        fp = (unsigned long)(frac * (double)scale + 0.5);
        if(fp >= scale && prec < 19)
        {
            ip++;
            fp -= scale;
        }
        n = utoa_rev(ip, 10, 0, tmp);
        while(n) buf[len++] = tmp[--n];
        if(prec)
        {
            buf[len++] = '.';
            n = utoa_rev(fp, 10, 0, tmp);
            for(i = n; i < prec; i++) buf[len++] = '0';
            while(n) buf[len++] = tmp[--n];
        }
    }
    else if(conv == 'f' || conv == 'F')
    {
        if(prec > 300) prec = 300;
        dtoa_digits(d, 1, dig, &e10);
        nd = e10 + prec;
        if(nd > 16)
        {
            nd = 16;
        }
        if(nd <= 0)
        {
            double scale = 1;
            for(i = 0; i < prec; i++) scale *= 10;
            buf[len++] = '0';
            if(prec)
            {
                unsigned long q = (unsigned long)(d * scale + 0.5);
                buf[len++] = '.';
                for(i = 0; i < prec - 1; i++) buf[len++] = '0';
                buf[len++] = '0' + (int)q;
            }
        }
        else
        {
            int pos;
            dtoa_digits(d, nd, dig, &e10);
            if(e10 + prec > nd && e10 + prec <= 17)
            {
                dtoa_digits(d, e10 + prec, dig, &e10);
                nd = e10 + prec;
            }
            pos = 0;
            if(e10 <= 0)
            {
                buf[len++] = '0';
            }
            else
            {
                for(i = 0; i < e10; i++) buf[len++] = pos < nd ? dig[pos++] : '0';
            }
            if(prec)
            {
                buf[len++] = '.';
                for(i = 0; i < prec; i++)
                {
                    if(e10 + i < 0) buf[len++] = '0';
                    else buf[len++] = pos < nd ? dig[pos++] : '0';
                }
            }
        }
    }
    else
    {
        int ex;
        if(prec > 16) prec = 16;
        dtoa_digits(d, prec + 1, dig, &e10);
        ex = d == 0 ? 0 : e10 - 1;
        buf[len++] = dig[0];
        if(prec)
        {
            buf[len++] = '.';
            for(i = 1; i <= prec; i++) buf[len++] = dig[i];
        }
        if(strip && prec)
        {
            while (buf[len - 1] == '0') len--;
            if(buf[len - 1] == '.') len--;
            strip = 0;
        }
        buf[len++] = upper ? 'E' : 'e';
        buf[len++] = ex < 0 ? '-' : '+';
        if(ex < 0) ex = -ex;
        if(ex >= 100) buf[len++] = '0' + ex / 100;
        buf[len++] = '0' + ex / 10 % 10;
        buf[len++] = '0' + ex % 10;
    }
    if(strip)
    {
        for(i = 0; i < len && buf[i] != '.'; i++);
        if(i < len)
        {
            while(buf[len - 1] == '0') len--;
            if(buf[len - 1] == '.') len--;
        }
    }
    out_field(o, pre, buf, len, width, 0, left, padc);
}

static int do_printf(out_t *o, const char *fmt, va_list ap)
{
    char tmp[72], digits[72];
    for(; *fmt; fmt++)
    {
        int left = 0, width = 0, prec = -1, lng = 0, n, i;
        char padc = ' ', sign[2] = { 0, 0 };
        const char *prefix = "";
        unsigned long u;
        long s;
        if(*fmt != '%')
        {
            out_c(o, *fmt);
            continue;
        }
        fmt++;
        for(;; fmt++)
        {
            if(*fmt == '-') left = 1;
            else if(*fmt == '0') padc = '0';
            else if(*fmt == '+') sign[0] = '+';
            else if(*fmt == ' ') { if(!sign[0]) sign[0] = ' '; }
            else if(*fmt == '#') ;
            else break;
        }
        if(*fmt == '*')
        {
            width = va_arg(ap, int);
            if(width < 0) left = 1, width = -width;
            fmt++;
        }
        else
        {
            while(*fmt >= '0' && *fmt <= '9') width = width * 10 + *fmt++ - '0';
        }
        if(*fmt == '.')
        {
            fmt++;
            prec = 0;
            if(*fmt == '*')
            {
                prec = va_arg(ap, int);
                fmt++;
            }
            else
            {
                while(*fmt >= '0' && *fmt <= '9') prec = prec * 10 + *fmt++ - '0';
            }
        }
        while(*fmt == 'l' || *fmt == 'z' || *fmt == 'j' || *fmt == 't' || *fmt == 'L') lng = 1, fmt++;
        while(*fmt == 'h') fmt++;
        if(left) padc = ' ';

        switch(*fmt)
        {
            case 'd':
            case 'i':
                s = lng ? va_arg(ap, long) : va_arg(ap, int);
                if(s < 0)
                {
                    sign[0] = '-';
                    u = -(unsigned long)s;
                }
                else
                {
                    u = s;
                }
                prefix = sign;
                goto number10;
            case 'u':
                u = lng ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
            number10:
                n = utoa_rev(u, 10, 0, tmp);
                goto emit;
            case 'x':
            case 'X':
                u = lng ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
                n = utoa_rev(u, 16, *fmt == 'X', tmp);
                goto emit;
            case 'o':
                u = lng ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
                n = utoa_rev(u, 8, 0, tmp);
                goto emit;
            case 'p':
                u = (unsigned long)va_arg(ap, void *);
                prefix = "0x";
                n = utoa_rev(u, 16, 0, tmp);
            emit:
                if(prec == 0 && u == 0) n = 0;
                for(i = 0; i < n; i++) digits[i] = tmp[n - 1 - i];
                if(prec >= 0) padc = ' ';
                out_field(o, prefix, digits, n, width, prec, left, padc);
                break;
            case 'c':
                tmp[0] = (char)va_arg(ap, int);
                out_field(o, "", tmp, 1, width, 0, left, ' ');
                break;
            case 's':
            {
                const char *str = va_arg(ap, const char *);
                if(!str) str = "(null)";
                n = strlen(str);
                if(prec >= 0 && n > prec) n = prec;
                out_field(o, "", str, n, width, 0, left, ' ');
                break;
            }
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            case 'e':
            case 'E':
                out_double(o, va_arg(ap, double), prec, width, left, padc, sign, *fmt);
                break;
            case '%':
                out_c(o, '%');
                break;
            default:
                out_c(o, '%');
                out_c(o, *fmt);
                break;
        }
    }

    if(o->size) o->buf[o->len < o->size ? o->len : o->size - 1] = 0;
    return (int)o->len;
}

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
{
    out_t o;
    char dummy;
    o.buf = n ? buf : &dummy;
    o.size = n ? n : 1;
    o.len = 0;
    return do_printf(&o, fmt, ap);
}

int snprintf(char *buf, size_t n, const char *fmt, ...)
{
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vsnprintf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vsnprintf(buf, (size_t)INT_MAX, fmt, ap);
    va_end(ap);
    return r;
}

int vprintf(const char *fmt, va_list ap)
{
    out_t o;
    o.buf = NULL;
    o.size = 0;
    o.len = 0;
    return do_printf(&o, fmt, ap);
}

int vfprintf(FILE *f, const char *fmt, va_list ap)
{
    (void)f;
    return vprintf(fmt, ap);
}

int fprintf(FILE *f, const char *fmt, ...)
{
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vfprintf(f, fmt, ap);
    va_end(ap);
    return r;
}

int printf(const char *fmt, ...)
{
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vprintf(fmt, ap);
    va_end(ap);
    return r;
}

struct __emex64_tls_index {
    unsigned long module;
    unsigned long offset;
};

#define EMEX64_TLS_MODULES 16
static char *tls_modules[EMEX64_TLS_MODULES];

int __emex64_tls_register(unsigned long module, void *block)
{
    if(module == 0 || module >= EMEX64_TLS_MODULES) return -1;
    tls_modules[module] = block;
    return 0;
}

void *__tls_get_addr(struct __emex64_tls_index *ti)
{
    char *base;
    if(ti->module == 1)
    {
        __asm__("mov %0, tp" : "=r"(base));
    }
    else
    {
        if(ti->module >= EMEX64_TLS_MODULES || !tls_modules[ti->module]) abort();
        base = tls_modules[ti->module];
    }
    return base + ti->offset;
}
