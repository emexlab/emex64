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

#include <emex64lib/vm/instruction/instruction.h>
#include <emex64lib/vm/instruction/alu.h>

#if defined(__x86_64__)
#include <immintrin.h>
#endif /* __x86_64__ */

#define DEFINE_EMEX64_ARITHMETIC_OP(act)                                                                                                \
    *(core->op.param[0]) = *(core->op.param[core->op.param_cnt - 2]) act *(core->op.param[core->op.param_cnt - 1]);                     \

#define DEFINE_EMEX64_SIGNED_ARITHMETIC_OP(act)                                                                                         \
    *(core->op.param[0]) = (int64_t)*(core->op.param[core->op.param_cnt - 2]) act (int64_t)*(core->op.param[core->op.param_cnt - 1]);   \

#define DEFINE_EMEX64_ARITHMETIC_OP_ZERO_BAD(act)                                                                                       \
    uint64_t *operand[2] = { core->op.param[core->op.param_cnt - 2], core->op.param[core->op.param_cnt - 1] };                          \
    if(*operand[1] == 0)                                                                                                                \
    {                                                                                                                                   \
        core->cr_state.crexc.exception = kEmex64ExceptionBadArithmetic;                                                                 \
        return;                                                                                                                         \
    }                                                                                                                                   \
    *(core->op.param[0]) = *operand[0] act *operand[1];

#define DEFINE_EMEX64_SIGNED_ARITHMETIC_OP_ZERO_BAD(act)                                                                                \
    uint64_t *operand[2] = { core->op.param[core->op.param_cnt - 2], core->op.param[core->op.param_cnt - 1] };                          \
    if(*operand[1] == 0)                                                                                                                \
    {                                                                                                                                   \
        core->cr_state.crexc.exception = kEmex64ExceptionBadArithmetic;                                                                 \
        return;                                                                                                                         \
    }                                                                                                                                   \
    *(core->op.param[0]) = (int64_t)*operand[0] act (int64_t)*operand[1];

void emex64_op_add(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_ARITHMETIC_OP(+);
}

void emex64_op_sub(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_ARITHMETIC_OP(-);
}

void emex64_op_mul(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_ARITHMETIC_OP(*);
}

void emex64_op_div(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_ARITHMETIC_OP_ZERO_BAD(/);
}

void emex64_op_idiv(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_SIGNED_ARITHMETIC_OP_ZERO_BAD(/);
}

void emex64_op_mod(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_ARITHMETIC_OP_ZERO_BAD(%);
}

void emex64_op_not(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt == 0);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        *(core->op.param[i]) = ~*(core->op.param[i]);
    }
}

void emex64_op_neg(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt == 0);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        *(core->op.param[i]) = -*(core->op.param[i]);
    }
}

void emex64_op_and(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_ARITHMETIC_OP(&);
}

void emex64_op_or(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_ARITHMETIC_OP(|);
}

void emex64_op_xor(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_ARITHMETIC_OP(^);
}

void emex64_op_shr(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_ARITHMETIC_OP(>>);
}

void emex64_op_shl(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_ARITHMETIC_OP(<<);
}

void emex64_op_sar(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_EMEX64_SIGNED_ARITHMETIC_OP(>>);
}

void emex64_op_ror(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);

    uint64_t v = *core->op.param[0];
    uint64_t n = (core->op.param_cnt == 2) ? (*core->op.param[1] & 63) : 1;
    *core->op.param[0] = (v >> n) | (v << (64 - n));
}

void emex64_op_rol(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);

    int64_t v = *core->op.param[0];
    uint64_t n = (core->op.param_cnt == 2) ? (*core->op.param[1] & 63) : 1;
    *core->op.param[0] = (v << n) | (v >> (64 - n));
}

#if defined(__x86_64__)
__attribute__((target("bmi2")))
#endif /*__x86_64__  */
void emex64_op_pdep(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    uint64_t *dest = core->op.param[0];
    uint64_t src = *core->op.param[core->op.param_cnt - 2];
    uint64_t mask = *core->op.param[core->op.param_cnt - 1];

#if defined(__x86_64__)
    if(__builtin_cpu_supports("bmi2"))
    {
        *dest = _pdep_u64(src, mask);
    }
    else
    {
#endif /*__x86_64__  */
        uint64_t result = 0;
        uint64_t bit = 1;

        while(mask)
        {
            uint64_t lowest = mask & -mask;

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
void emex64_op_pext(emex64_core_t *core)
{
    emex64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    uint64_t *dest = core->op.param[0];
    uint64_t src = *core->op.param[core->op.param_cnt - 2];
    uint64_t mask = *core->op.param[core->op.param_cnt - 1];

#if defined(__x86_64__)
    if(__builtin_cpu_supports("bmi2"))
    {
        *dest = _pext_u64(src, mask);
    }
    else
    {
#endif /*__x86_64__  */
        uint64_t result = 0;
        uint64_t bit = 1;

        while(mask)
        {
            uint64_t lowest = mask & -mask;

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

void emex64_op_bswapw(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);
    *core->op.param[0] = __builtin_bswap16((uint16_t)*core->op.param[0]);
}

void emex64_op_bswapd(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);
    *core->op.param[0] = __builtin_bswap32((uint32_t)*core->op.param[0]);
}

void emex64_op_bswapq(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt != 1);
    *core->op.param[0] = __builtin_bswap64(*core->op.param[0]);
}

void emex64_op_inc(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt < 1);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        (*core->op.param[i])++;
    }
}

void emex64_op_dec(emex64_core_t *core)
{
    emex64_instr_termcond(core->op.param_cnt < 1);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        (*core->op.param[i])--;
    }
}
