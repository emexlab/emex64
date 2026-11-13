#ifndef _STDIO_H
#define _STDIO_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define EOF (-1)
typedef struct __emex64_file FILE;
extern FILE *stdout, *stderr;
int fputc(int c, FILE *f);
int fputs(const char *s, FILE *f);
int fflush(FILE *f);
int fprintf(FILE *f, const char *fmt, ...);
int vfprintf(FILE *f, const char *fmt, va_list ap);
#define putc fputc
int putchar(int c);
int puts(const char *s);
int getchar(void);
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int sprintf(char *buf, const char *fmt, ...);
int snprintf(char *buf, size_t n, const char *fmt, ...);
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDIO_H */
