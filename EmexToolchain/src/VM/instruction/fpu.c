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

#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/instruction/fpu.h>

#pragma STDC FENV_ACCESS    ON

static const int kHostRounding[4] = {
    FE_TONEAREST,
    FE_TOWARDZERO,
    FE_DOWNWARD,
    FE_UPWARD
};

static inline unsigned fpu_begin(__E64Core core)
{
    unsigned rm = (unsigned)(core->rl[kE64RegisterFPC] & E64_FPC_RM_MASK);
    if(rm != E64_FPC_RM_NEAREST)
    {
        fesetround(kHostRounding[rm]);
    }
    feclearexcept(FE_ALL_EXCEPT);
    return rm;
}

static inline void fpu_end(__E64Core core,
                           unsigned rm,
                           UInt64 extra)
{
    int ex = fetestexcept(FE_ALL_EXCEPT);
    if(rm != E64_FPC_RM_NEAREST)
    {
        fesetround(FE_TONEAREST);
    }
    UInt64 flags = extra;
    if(ex & FE_INVALID) flags |= E64_FPC_NV;
    if(ex & FE_DIVBYZERO) flags |= E64_FPC_DZ;
    if(ex & FE_OVERFLOW) flags |= E64_FPC_OF;
    if(ex & FE_UNDERFLOW) flags |= E64_FPC_UF;
    if(ex & FE_INEXACT) flags |= E64_FPC_NX;
    core->rl[kE64RegisterFPC] |= flags;
}

static inline double get_d(const UInt64 *p)
{
    union { UInt64 u; double d; } x = { .u = *p };
    return x.d;
}

static inline float get_s(const UInt64 *p)
{
    union { UInt32 u; float f; } x = { .u = (UInt32)*p };
    return x.f;
}

static inline void put_d(UInt64 *p, double d)
{
    union { UInt64 u; double d; } x = { .d = d };
    *p = x.u;
}

static inline void put_s(UInt64 *p, float f)
{
    union { UInt32 u; float f; } x = { .f = f };
    *p = x.u;
}

#define DST         (core->op.param[0])
#define SRC_A       (core->op.param[core->op.param_cnt - 2])
#define SRC_B       (core->op.param[core->op.param_cnt - 1])
#define SRC_LAST    (core->op.param[core->op.param_cnt - 1])

#define FPU_BINARY(name, T, get, put, expr)                                 \
void emex64_op_##name(__E64Core core)                                       \
{                                                                           \
    unsigned rm = fpu_begin(core);                                          \
    volatile T a = get(SRC_A);                                              \
    volatile T b = get(SRC_B);                                              \
    volatile T r = (expr);                                                  \
    fpu_end(core, rm, 0);                                                   \
    put(DST, r);                                                            \
}

FPU_BINARY(fadd, double, get_d, put_d, a + b)
FPU_BINARY(fsub, double, get_d, put_d, a - b)
FPU_BINARY(fmul, double, get_d, put_d, a * b)
FPU_BINARY(fdiv, double, get_d, put_d, a / b)
FPU_BINARY(fmin, double, get_d, put_d, fmin(a, b))
FPU_BINARY(fmax, double, get_d, put_d, fmax(a, b))
FPU_BINARY(fadds, float, get_s, put_s, a + b)
FPU_BINARY(fsubs, float, get_s, put_s, a - b)
FPU_BINARY(fmuls, float, get_s, put_s, a * b)
FPU_BINARY(fdivs, float, get_s, put_s, a / b)
FPU_BINARY(fmins, float, get_s, put_s, fminf(a, b))
FPU_BINARY(fmaxs, float, get_s, put_s, fmaxf(a, b))

void emex64_op_fsqrt(__E64Core core)
{
    unsigned rm = fpu_begin(core);
    volatile double a = get_d(SRC_LAST);
    volatile double r = sqrt(a);
    fpu_end(core, rm, 0);
    put_d(DST, r);
}

void emex64_op_fsqrts(__E64Core core)
{
    unsigned rm = fpu_begin(core);
    volatile float a = get_s(SRC_LAST);
    volatile float r = sqrtf(a);
    fpu_end(core, rm, 0);
    put_s(DST, r);
}

void emex64_op_fmadd(__E64Core core)
{
    unsigned rm = fpu_begin(core);
    volatile double a = get_d(core->op.param[1]);
    volatile double b = get_d(core->op.param[2]);
    volatile double c = get_d(core->op.param[3]);
    volatile double r = fma(a, b, c);
    fpu_end(core, rm, 0);
    put_d(DST, r);
}

void emex64_op_fmadds(__E64Core core)
{
    unsigned rm = fpu_begin(core);
    volatile float a = get_s(core->op.param[1]);
    volatile float b = get_s(core->op.param[2]);
    volatile float c = get_s(core->op.param[3]);
    volatile float r = fmaf(a, b, c);
    fpu_end(core, rm, 0);
    put_s(DST, r);
}

void emex64_op_fneg(__E64Core core)
{
    *DST = *SRC_LAST ^ (1ULL << 63);
}

void emex64_op_fabs(__E64Core core)
{
    *DST = *SRC_LAST & ~(1ULL << 63);
}

void emex64_op_fnegs(__E64Core core)
{
    *DST = (UInt32)(*SRC_LAST ^ (1ULL << 31));
}

void emex64_op_fabss(__E64Core core)
{
    *DST = (UInt32)(*SRC_LAST & ~(1ULL << 31));
}

#define FPU_COMPARE(name, T, get)                                           \
void emex64_op_##name(__E64Core core)                                       \
{                                                                           \
    unsigned rm = fpu_begin(core);                                          \
    volatile T a = get(core->op.param[0]);                                  \
    volatile T b = get(core->op.param[1]);                                  \
    UInt64 cf;                                                              \
    if(isunordered(a, b))                                                   \
    {                                                                       \
        cf = kE64CompareFlagU;                                              \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        cf = (a == b) * kE64CompareFlagZ | isless(a, b) * kE64CompareFlagL | isgreater(a, b) * kE64CompareFlagG; \
    }                                                                       \
    fpu_end(core, rm, 0);                                                   \
    core->rl[kE64RegisterCF] = cf;                                          \
}

FPU_COMPARE(fcmp, double, get_d)
FPU_COMPARE(fcmps, float, get_s)

void emex64_op_fcvtds(__E64Core core)
{
    unsigned rm = fpu_begin(core);
    volatile float a = get_s(SRC_LAST);
    volatile double r = (double)a;
    fpu_end(core, rm, 0);
    put_d(DST, r);
}

void emex64_op_fcvtsd(__E64Core core)
{
    unsigned rm = fpu_begin(core);
    volatile double a = get_d(SRC_LAST);
    volatile float r = (float)a;
    fpu_end(core, rm, 0);
    put_s(DST, r);
}

#define FPU_FROM_INT(name, IT, T, put)                                      \
void emex64_op_##name(__E64Core core)                                       \
{                                                                           \
    unsigned rm = fpu_begin(core);                                          \
    volatile IT a = (IT)*SRC_LAST;                                          \
    volatile T r = (T)a;                                                    \
    fpu_end(core, rm, 0);                                                   \
    put(DST, r);                                                            \
}

FPU_FROM_INT(fcvtdi, SInt64, double, put_d)
FPU_FROM_INT(fcvtdu, UInt64, double, put_d)
FPU_FROM_INT(fcvtsi, SInt64, float, put_s)
FPU_FROM_INT(fcvtsu, UInt64, float, put_s)

#define FPU_TO_SIGNED(name, T, get)                                         \
void emex64_op_##name(__E64Core core)                                       \
{                                                                           \
    unsigned rm = fpu_begin(core);                                          \
    volatile T a = get(SRC_LAST);                                           \
    SInt64 r;                                                               \
    UInt64 extra = 0;                                                       \
    if(isnan(a))                                                            \
    {                                                                       \
        r = 0;                                                              \
        extra = E64_FPC_NV;                                                 \
    }                                                                       \
    else if(a >= (T)9223372036854775808.0)                                  \
    {                                                                       \
        r = INT64_MAX;                                                      \
        extra = E64_FPC_NV;                                                 \
    }                                                                       \
    else if(a < (T)-9223372036854775808.0)                                  \
    {                                                                       \
        r = INT64_MIN;                                                      \
        extra = E64_FPC_NV;                                                 \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        r = (SInt64)a;                                                      \
    }                                                                       \
    fpu_end(core, rm, extra);                                               \
    *DST = (UInt64)r;                                                       \
}

#define FPU_TO_UNSIGNED(name, T, get)                                       \
void emex64_op_##name(__E64Core core)                                       \
{                                                                           \
    unsigned rm = fpu_begin(core);                                          \
    volatile T a = get(SRC_LAST);                                           \
    UInt64 r;                                                               \
    UInt64 extra = 0;                                                       \
    if(isnan(a))                                                            \
    {                                                                       \
        r = 0;                                                              \
        extra = E64_FPC_NV;                                                 \
    }                                                                       \
    else if(a >= (T)18446744073709551616.0)                                 \
    {                                                                       \
        r = UINT64_MAX;                                                     \
        extra = E64_FPC_NV;                                                 \
    }                                                                       \
    else if(a <= (T)-1.0)                                                   \
    {                                                                       \
        r = 0;                                                              \
        extra = E64_FPC_NV;                                                 \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        r = (UInt64)a;                                                      \
    }                                                                       \
    fpu_end(core, rm, extra);                                               \
    *DST = r;                                                               \
}

FPU_TO_SIGNED(fcvtid, double, get_d)
FPU_TO_UNSIGNED(fcvtud, double, get_d)
FPU_TO_SIGNED(fcvtis, float, get_s)
FPU_TO_UNSIGNED(fcvtus, float, get_s)
