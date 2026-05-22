/*
 * MIT License
 *
 * Copyright (c) 2024 cr4zyengineer
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

#ifndef LA64ASM_CMPTOK_H
#define LA64ASM_CMPTOK_H

#define CMPTOK_LENGHT_MAX               2048    /* if anyone comes close to that size, bro pls fix your variable naming style O.O */

#define CMPTOK_TOKEN_MODE_NONE          0b00    /* the nothing mode */
#define CMPTOK_TOKEN_MODE_STRING        0b01    /* string mode, means it parses the next characters as a character buffer sequence */

/*
 * character mode, means it parses the next characters as
 * a character, if its not a valid character next steps in
 * compilation will fail, but thats not responsibility
 * of this parser!
 */
#define CMPTOK_TOKEN_MODE_CHAR          0b10

#include <stdint.h>

typedef struct {
    const char *token;                          /* token that gets returned (is only valid until next cmptok(1) call from the same thread as before */
    size_t pos;                                 /* column position*/
} cmptok_return_t;

cmptok_return_t cmptok(const char *token);

#endif /* LA64ASM_CMPTOK_H */
