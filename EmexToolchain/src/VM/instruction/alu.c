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

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/instruction/alu.h>

#if defined(__x86_64__)
#include <immintrin.h>
#endif /* __x86_64__ */

#define DEFINE_EMEX64_ARITHMETIC_OP(act)                                                                                                \
    *(core->op.param[0]) = *(core->op.param[core->op.param_cnt - 2]) act *(core->op.param[core->op.param_cnt - 1]);                     \

#define DEFINE_EMEX64_SIGNED_ARITHMETIC_OP(act)                                                                                         \
    *(core->op.param[0]) = (SInt64)*(core->op.param[core->op.param_cnt - 2]) act (SInt64)*(core->op.param[core->op.param_cnt - 1]);   \

#define DEFINE_EMEX64_ARITHMETIC_OP_ZERO_BAD(act)                                                                                       \
    UInt64 *operand[2] = { core->op.param[core->op.param_cnt - 2], core->op.param[core->op.param_cnt - 1] };                          \
    if(*operand[1] == 0)                                                                                                                \
    {                                                                                                                                   \
        core->cr_state.crexc.exception = kE64ExceptionBadArithmetic;                                                                 \
        return;                                                                                                                         \
    }                                                                                                                                   \
    *(core->op.param[0]) = *operand[0] act *operand[1];

#define DEFINE_EMEX64_SIGNED_ARITHMETIC_OP_ZERO_BAD(act)                                                                                \
    UInt64 *operand[2] = { core->op.param[core->op.param_cnt - 2], core->op.param[core->op.param_cnt - 1] };                          \
    if(*operand[1] == 0)                                                                                                                \
    {                                                                                                                                   \
        core->cr_state.crexc.exception = kE64ExceptionBadArithmetic;                                                                 \
        return;                                                                                                                         \
    }                                                                                                                                   \
    *(core->op.param[0]) = (SInt64)*operand[0] act (SInt64)*operand[1];

void emex64_op_add(__E64Core core)
{
    DEFINE_EMEX64_ARITHMETIC_OP(+);
}

void emex64_op_sub(__E64Core core)
{
    DEFINE_EMEX64_ARITHMETIC_OP(-);
}

void emex64_op_mul(__E64Core core)
{
    DEFINE_EMEX64_ARITHMETIC_OP(*);
}

void emex64_op_div(__E64Core core)
{
    DEFINE_EMEX64_ARITHMETIC_OP_ZERO_BAD(/);
}

void emex64_op_idiv(__E64Core core)
{
    DEFINE_EMEX64_SIGNED_ARITHMETIC_OP_ZERO_BAD(/);
}

void emex64_op_mod(__E64Core core)
{
    DEFINE_EMEX64_ARITHMETIC_OP_ZERO_BAD(%);
}

void emex64_op_not(__E64Core core)
{
    for(UInt8 i = 0; i < core->op.param_cnt; i++)
    {
        *(core->op.param[i]) = ~*(core->op.param[i]);
    }
}

void emex64_op_neg(__E64Core core)
{
    for(UInt8 i = 0; i < core->op.param_cnt; i++)
    {
        *(core->op.param[i]) = -*(core->op.param[i]);
    }
}

void emex64_op_and(__E64Core core)
{
    DEFINE_EMEX64_ARITHMETIC_OP(&);
}

void emex64_op_or(__E64Core core)
{
    DEFINE_EMEX64_ARITHMETIC_OP(|);
}

void emex64_op_xor(__E64Core core)
{
    DEFINE_EMEX64_ARITHMETIC_OP(^);
}

void emex64_op_shr(__E64Core core)
{
    DEFINE_EMEX64_ARITHMETIC_OP(>>);
}

void emex64_op_shl(__E64Core core)
{
    DEFINE_EMEX64_ARITHMETIC_OP(<<);
}

void emex64_op_sar(__E64Core core)
{
    DEFINE_EMEX64_SIGNED_ARITHMETIC_OP(>>);
}

void emex64_op_ror(__E64Core core)
{
    UInt64 v = *core->op.param[0];
    UInt64 n = (core->op.param_cnt == 2) ? (*core->op.param[1] & 63) : 1;
    *core->op.param[0] = (v >> n) | (v << (64 - n));
}

void emex64_op_rol(__E64Core core)
{
    SInt64 v = *core->op.param[0];
    UInt64 n = (core->op.param_cnt == 2) ? (*core->op.param[1] & 63) : 1;
    *core->op.param[0] = (v << n) | (v >> (64 - n));
}

#if defined(__x86_64__)
__attribute__((target("bmi2")))
#endif /*__x86_64__  */
void emex64_op_pdep(__E64Core core)
{
    UInt64 *dest = core->op.param[0];
    UInt64 src = *core->op.param[core->op.param_cnt - 2];
    UInt64 mask = *core->op.param[core->op.param_cnt - 1];

#if defined(__x86_64__)
    if(__builtin_cpu_supports("bmi2"))
    {
        *dest = _pdep_u64(src, mask);
    }
    else
    {
#endif /*__x86_64__  */
        UInt64 result = 0;
        UInt64 bit = 1;

        while(mask)
        {
            UInt64 lowest = mask & -mask;

            if(src & bit)
            {
                result |= lowest;
            }

            mask ^= lowest;
            bit <<= 1;
        }

        *dest = result;
#if defined(__x86_64__)
    }
#endif /*__x86_64__  */
}

#if defined(__x86_64__)
__attribute__((target("bmi2")))
#endif /*__x86_64__  */
void emex64_op_pext(__E64Core core)
{
    UInt64 *dest = core->op.param[0];
    UInt64 src = *core->op.param[core->op.param_cnt - 2];
    UInt64 mask = *core->op.param[core->op.param_cnt - 1];

#if defined(__x86_64__)
    if(__builtin_cpu_supports("bmi2"))
    {
        *dest = _pext_u64(src, mask);
    }
    else
    {
#endif /*__x86_64__  */
        UInt64 result = 0;
        UInt64 bit = 1;

        while(mask)
        {
            UInt64 lowest = mask & -mask;

            if(src & lowest)
            {
                result |= bit;
            }

            mask ^= lowest;
            bit <<= 1;
        }

        *dest = result;
#if defined(__x86_64__)
    }
#endif /*__x86_64__  */
}

void emex64_op_bswapw(__E64Core core)
{
    *core->op.param[0] = __builtin_bswap16((UInt16)*core->op.param[0]);
}

void emex64_op_bswapd(__E64Core core)
{
    *core->op.param[0] = __builtin_bswap32((UInt32)*core->op.param[0]);
}

void emex64_op_bswapq(__E64Core core)
{
    *core->op.param[0] = __builtin_bswap64(*core->op.param[0]);
}

void emex64_op_sext(__E64Core core)
{
    UInt64 v = *core->op.param[core->op.param_cnt - 2];
    UInt64 n = *core->op.param[core->op.param_cnt - 1] & 63;
    if(n != 0)
    {
        v = (UInt64)((SInt64)(v << (64 - n)) >> (64 - n));
    }
    *core->op.param[0] = v;
}

void emex64_op_clz(__E64Core core)
{
    UInt64 v = *core->op.param[core->op.param_cnt - 1];
    *core->op.param[0] = v ? (UInt64)__builtin_clzll(v) : 64;
}

void emex64_op_ctz(__E64Core core)
{
    UInt64 v = *core->op.param[core->op.param_cnt - 1];
    *core->op.param[0] = v ? (UInt64)__builtin_ctzll(v) : 64;
}

void emex64_op_popcnt(__E64Core core)
{
    *core->op.param[0] = (UInt64)__builtin_popcountll(*core->op.param[core->op.param_cnt - 1]);
}

void emex64_op_mulh(__E64Core core)
{
    __int128 p = (__int128)(SInt64)*core->op.param[core->op.param_cnt - 2] * (__int128)(SInt64)*core->op.param[core->op.param_cnt - 1];
    *core->op.param[0] = (UInt64)(p >> 64);
}

void emex64_op_umulh(__E64Core core)
{
    unsigned __int128 p = (unsigned __int128)*core->op.param[core->op.param_cnt - 2] * (unsigned __int128)*core->op.param[core->op.param_cnt - 1];
    *core->op.param[0] = (UInt64)(p >> 64);
}

void emex64_op_adc(__E64Core core)
{
    UInt64 a = *core->op.param[core->op.param_cnt - 2];
    UInt64 b = *core->op.param[core->op.param_cnt - 1];
    UInt64 cf = core->rl[kE64RegisterCF];
    UInt64 sum;
    Boolean carry = __builtin_add_overflow(a, b, &sum);
    carry |= __builtin_add_overflow(sum, (cf & kE64CompareFlagC) ? 1 : 0, &sum);
    core->rl[kE64RegisterCF] = (cf & ~(UInt64)kE64CompareFlagC) | (carry ? kE64CompareFlagC : 0);
    *core->op.param[0] = sum;
}

void emex64_op_sbb(__E64Core core)
{
    UInt64 a = *core->op.param[core->op.param_cnt - 2];
    UInt64 b = *core->op.param[core->op.param_cnt - 1];
    UInt64 cf = core->rl[kE64RegisterCF];
    UInt64 diff;
    Boolean borrow = __builtin_sub_overflow(a, b, &diff);
    borrow |= __builtin_sub_overflow(diff, (cf & kE64CompareFlagC) ? 1 : 0, &diff);
    core->rl[kE64RegisterCF] = (cf & ~(UInt64)kE64CompareFlagC) | (borrow ? kE64CompareFlagC : 0);
    *core->op.param[0] = diff;
}

void emex64_op_inc(__E64Core core)
{
    for(UInt8 i = 0; i < core->op.param_cnt; i++)
    {
        (*core->op.param[i])++;
    }
}

void emex64_op_dec(__E64Core core)
{
    for(UInt8 i = 0; i < core->op.param_cnt; i++)
    {
        (*core->op.param[i])--;
    }
}
