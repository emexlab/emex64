#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1
#define RAND_MAX        0x7fffffff

void exit(int code) __attribute__((noreturn));
void abort(void) __attribute__((noreturn));
int atexit(void (*fn)(void));

extern void *__emex64_boot_arg;

void *malloc(size_t n);
void *calloc(size_t n, size_t m);
void *realloc(void *p, size_t n);
void free(void *p);
int abs(int x);
long labs(long x);
long long llabs(long long x);
int atoi(const char *s);
double atof(const char *s);
double strtod(const char *s, char **end);
float strtof(const char *s, char **end);
long double strtold(const char *s, char **end);
long atol(const char *s);
long strtol(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);
int rand(void);
void srand(unsigned seed);
void qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _STDLIB_H */
