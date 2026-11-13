#ifndef _MATH_H
#define _MATH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NAN __builtin_nan("")
#define INFINITY __builtin_inf()
#define HUGE_VAL __builtin_huge_val()
#define isnan(x) __builtin_isnan(x)
#define isinf(x) __builtin_isinf(x)
#define isfinite(x) __builtin_isfinite(x)
#define signbit(x) __builtin_signbit(x)
double sqrt(double);
float sqrtf(float);
double fabs(double);
float fabsf(float);
double fmin(double, double);
double fmax(double, double);
float fminf(float, float);
float fmaxf(float, float);
double copysign(double, double);
float copysignf(float, float);
double trunc(double);
double floor(double);
double ceil(double);
double round(double);
float truncf(float);
float floorf(float);
float ceilf(float);
float roundf(float);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _MATH_H */
