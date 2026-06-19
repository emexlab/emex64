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

#ifndef EMEX64ASM_ENDIAN_H
#define EMEX64ASM_ENDIAN_H

#define BW_LITTLE_ENDIAN 0
#define BW_BIG_ENDIAN 1

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#define BW_HOST_ENDIAN  BW_BIG_ENDIAN
#else
#define BW_HOST_ENDIAN  BW_LITTLE_ENDIAN
#endif

#if BW_HOST_ENDIAN == BW_BIG_ENDIAN
#define TO_HOST16(x) __builtin_bswap16(x)
#define TO_HOST32(x) __builtin_bswap32(x)
#define TO_HOST64(x) __builtin_bswap64(x)
#else
#define TO_HOST16(x) (x)
#define TO_HOST32(x) (x)
#define TO_HOST64(x) (x)
#endif

#endif /* EMEX64ASM_ENDIAN_H */
