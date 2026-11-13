#ifndef _ASSERT_H
#define _ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void __assert_fail(const char *expr, const char *file, int line) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ASSERT_H */

#undef assert
#ifdef NDEBUG
#define assert(x) ((void)0)
#else
#define assert(x) ((x) ? (void)0 : __assert_fail(#x, __FILE__, __LINE__))
#endif /* NDEBUG */
