#ifndef _SETJMP_H
#define _SETJMP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef unsigned long jmp_buf[24];

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SETJMP_H */
