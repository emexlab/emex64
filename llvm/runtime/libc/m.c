#include <stdint.h>

typedef union { double d; uint64_t u; } dbits;
typedef union { float f; uint32_t u; } fbits;

double fabs(double x) { dbits b = { x }; b.u &= ~(1ULL << 63); return b.d; }
float fabsf(float x) { fbits b = { x }; b.u &= ~(1U << 31); return b.f; }

double copysign(double x, double y)
{
    dbits a = { x }, b = { y };
    a.u = (a.u & ~(1ULL << 63)) | (b.u & (1ULL << 63));
    return a.d;
}

float copysignf(float x, float y)
{
    fbits a = { x }, b = { y };
    a.u = (a.u & ~(1U << 31)) | (b.u & (1U << 31));
    return a.f;
}

double fmin(double x, double y)
{
    if(x != x) return y;
    if(y != y) return x;
    return x < y ? x : y;
}

double fmax(double x, double y)
{
    if(x != x) return y;
    if(y != y) return x;
    return x > y ? x : y;
}

float fminf(float x, float y) { return (float)fmin(x, y); }
float fmaxf(float x, float y) { return (float)fmax(x, y); }

double trunc(double x)
{
    dbits b = { x };
    int e = (int)((b.u >> 52) & 0x7FF) - 1023;
    if(e >= 52) return x;
    if(e < 0) b.u &= 1ULL << 63;
    else b.u &= ~((1ULL << (52 - e)) - 1);
    return b.d;
}

double floor(double x)
{
    double t = trunc(x);
    return (t > x) ? t - 1.0 : t;
}

double ceil(double x)
{
    double t = trunc(x);
    return (t < x) ? t + 1.0 : t;
}

double round(double x)
{
    double t = trunc(x);
    double d = x - t;
    if(d >= 0.5) return t + 1.0;
    if(d <= -0.5) return t - 1.0;
    return t;
}

float truncf(float x) { return (float)trunc(x); }
float floorf(float x) { return (float)floor(x); }
float ceilf(float x) { return (float)ceil(x); }
float roundf(float x) { return (float)round(x); }

#ifdef __emex64_fpu__

double sqrt(double x) { return __builtin_sqrt(x); }
float sqrtf(float x) { return __builtin_sqrtf(x); }

#else

double sqrt(double x)
{
    dbits b = { x };
    uint64_t sign = b.u >> 63;
    int e = (int)((b.u >> 52) & 0x7FF);
    uint64_t m = b.u & ((1ULL << 52) - 1);

    if(e == 0x7FF) return (m || !sign) ? x : (x - x) / (x - x);
    if(e == 0 && m == 0) return x;
    if(sign) return (x - x) / (x - x);

    if(e == 0)
    {
        while(!(m & (1ULL << 52)))
        {
            m <<= 1;
            e--;
        }
        e++;
    }
    else
    {
        m |= 1ULL << 52;
    }
    e -= 1023;
    if(e & 1) m <<= 1;
    e >>= 1;

    m <<= 1;
    uint64_t q = 0, s = 0, r = 1ULL << 53;
    while(r)
    {
        uint64_t t = s + r;
        if(t <= m)
        {
            s = t + r;
            m -= t;
            q += r;
        }
        m <<= 1;
        r >>= 1;
    }
    uint64_t round = q & 1, sticky = m != 0;
    q >>= 1;
    if(round && (sticky || (q & 1))) q++;
    b.u = ((uint64_t)(e + 1023) << 52) + (q & ((1ULL << 52) - 1));
    if(q >> 53) b.u += 1ULL << 52;
    return b.d;
}

float sqrtf(float x) { return (float)sqrt(x); }

#endif /* __emex64_fpu__ */
