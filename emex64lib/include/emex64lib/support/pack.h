/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
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

#ifndef EMEX64_PACK_H
#define EMEX64_PACK_H

#include <stdint.h>
#include <stddef.h>

#define P7(a) ((uint64_t)((a) & 0x7F))
#define PACK1(a) P7(a)
#define PACK2(a,b) (PACK1(a) | (P7(b) << 7))
#define PACK3(a,b,c) (PACK2(a,b) | (P7(c) << 14))
#define PACK4(a,b,c,d) (PACK3(a,b,c) | (P7(d) << 21))
#define PACK5(a,b,c,d,e) (PACK4(a,b,c,d) | (P7(e) << 28))
#define PACK6(a,b,c,d,e,f) (PACK5(a,b,c,d,e) | (P7(f) << 35))
#define PACK7(a,b,c,d,e,f,g) (PACK6(a,b,c,d,e,f) | (P7(g) << 42))
#define PACK8(a,b,c,d,e,f,g,h) (PACK7(a,b,c,d,e,f,g) | (P7(h) << 49))
#define PACK9(a,b,c,d,e,f,g,h,i) (PACK8(a,b,c,d,e,f,g,h) | (P7(i) << 56))

#define PACK_CAT_(a,b) a##b
#define PACK_CAT(a,b)  PACK_CAT_(a,b)
#define PACK_NARG(...) PACK_NARG_(__VA_ARGS__, e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e, 9,8,7,6,5,4,3,2,1,0)
#define PACK_NARG_( _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16, _17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32, N, ...) N
#define PACKe(...) ((uint64_t)0 * sizeof(struct { static_assert(0, "PACK: name has more than 9 characters"); char _c; }))
#define PACK(...) PACK_CAT(PACK, PACK_NARG(__VA_ARGS__))(__VA_ARGS__)

static inline uint64_t pack_name(const char *s)
{
    if(s == NULL)
    {
        return UINT64_MAX;
    }

    uint64_t v = 0;
    int i = 0;
    for(; i < 9 && s[i]; i++)
    {
        v |= (uint64_t)(s[i] & 0x7F) << (i * 7);
    }
    return s[i] ? UINT64_MAX : v;
}

#endif /* EMEX64_PACK_H */
