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

#ifndef LAUTILS_PARSER_H
#define LAUTILS_PARSER_H

#include <stdint.h>

typedef enum laParserTypeValue {
    laParserValueTypeString         = 0x0,
    laParserValueTypeDecimal        = 0x1,
    laParserValueTypeHexadecimal    = 0x2,
    laParserValueTypeBinary         = 0x3,
    laParserValueTypeCharacter      = 0x4,
    laParserValueTypeBuffer         = 0x5
} parser_value_type_t;

typedef struct parsed_type_return {
    uint64_t value;
    uint64_t len;
    parser_value_type_t type;
} parser_return_t;

parser_return_t parse_value_from_string(const char *s);

#endif /* LAUTILS_PARSER_H */
