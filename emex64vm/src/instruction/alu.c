/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <emex64vm/instruction/instruction.h>
#include <emex64vm/instruction/alu.h>

#if defined(__x86_64__)
#include <immintrin.h>
#endif /* __x86_64__ */

#define DEFINE_LA64_ARITHMETIC_OP(act)                                                                                                  \
    *(core->op.param[0]) = *(core->op.param[core->op.param_cnt - 2]) act *(core->op.param[core->op.param_cnt - 1]);                     \

#define DEFINE_LA64_SIGNED_ARITHMETIC_OP(act)                                                                                           \
    *(core->op.param[0]) = (int64_t)*(core->op.param[core->op.param_cnt - 2]) act (int64_t)*(core->op.param[core->op.param_cnt - 1]);   \

#define DEFINE_LA64_ARITHMETIC_OP_ZERO_BAD(act)                                                                                         \
    uint64_t *operand[2] = { core->op.param[core->op.param_cnt - 2], core->op.param[core->op.param_cnt - 1] };                          \
    if(*operand[1] == 0)                                                                                                                \
    {                                                                                                                                   \
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ARITHMETIC;                                                                    \
        return;                                                                                                                         \
    }                                                                                                                                   \
    *(core->op.param[0]) = *operand[0] act *operand[1];

#define DEFINE_LA64_SIGNED_ARITHMETIC_OP_ZERO_BAD(act)                                                                                  \
    uint64_t *operand[2] = { core->op.param[core->op.param_cnt - 2], core->op.param[core->op.param_cnt - 1] };                          \
    if(*operand[1] == 0)                                                                                                                \
    {                                                                                                                                   \
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ARITHMETIC;                                                                    \
        return;                                                                                                                         \
    }                                                                                                                                   \
    *(core->op.param[0]) = (int64_t)*operand[0] act (int64_t)*operand[1];

void la64_op_add(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_ARITHMETIC_OP(+);
}

void la64_op_sub(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_ARITHMETIC_OP(-);
}

void la64_op_mul(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_ARITHMETIC_OP(*);
}

void la64_op_div(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_ARITHMETIC_OP_ZERO_BAD(/);
}

void la64_op_idiv(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_SIGNED_ARITHMETIC_OP_ZERO_BAD(/);
}

void la64_op_mod(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_ARITHMETIC_OP_ZERO_BAD(%);
}

void la64_op_not(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt == 0);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        *(core->op.param[i]) = ~*(core->op.param[i]);
    }
}

void la64_op_neg(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt == 0);

    for(uint8_t i = 0; i < core->op.param_cnt; i++)
    {
        *(core->op.param[i]) = -*(core->op.param[i]);
    }
}

void la64_op_and(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_ARITHMETIC_OP(&);
}

void la64_op_or(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_ARITHMETIC_OP(|);
}

void la64_op_xor(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_ARITHMETIC_OP(^);
}

void la64_op_shr(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_ARITHMETIC_OP(>>);
}

void la64_op_shl(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_ARITHMETIC_OP(<<);
}

void la64_op_sar(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

    DEFINE_LA64_SIGNED_ARITHMETIC_OP(>>);
}

void la64_op_ror(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);

    uint64_t v = *core->op.param[0];
    uint64_t n = (core->op.param_cnt == 2) ? (*core->op.param[1] & 63) : 1;
    *core->op.param[0] = (v >> n) | (v << (64 - n));
}

void la64_op_rol(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);

    int64_t v = *core->op.param[0];
    uint64_t n = (core->op.param_cnt == 2) ? (*core->op.param[1] & 63) : 1;
    *core->op.param[0] = (v << n) | (v >> (64 - n));
}

#if defined(__x86_64__)
__attribute__((target("bmi2")))
#endif /*__x86_64__  */
void la64_op_pdep(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

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
void la64_op_pext(la64_core_t *core)
{
    la64_instr_termcond((unsigned)(core->op.param_cnt - 2) > 1);

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

void la64_op_bswapw(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    *core->op.param[0] = __builtin_bswap16((uint16_t)*core->op.param[0]);
}

void la64_op_bswapd(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    *core->op.param[0] = __builtin_bswap32((uint32_t)*core->op.param[0]);
}

void la64_op_bswapq(la64_core_t *core)
{
    la64_instr_termcond(core->op.param_cnt != 1);
    *core->op.param[0] = __builtin_bswap64(*core->op.param[0]);
}
