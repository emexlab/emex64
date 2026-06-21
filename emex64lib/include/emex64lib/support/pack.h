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

#ifndef PACK_H
#define PACK_H

#include <stdint.h>

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

static inline uint64_t pack_name(const char *s)
{
    uint64_t v = 0;
    int i = 0;
    for(; i < 9 && s[i]; i++)
    {
        v |= (uint64_t)(s[i] & 0x7F) << (i * 7);
    }
    return s[i] ? (1ull << 63) : v;
}

#endif /* PACK_H */
