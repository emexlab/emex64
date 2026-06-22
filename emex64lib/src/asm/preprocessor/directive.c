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

#include <emex64lib/support/pack.h>

#include <emex64lib/asm/preprocessor/directive.h>

kAssemblerDirectiveType assembler_directive_type_for_str(const char *str)
{
    if(str == NULL)
    {
        return kAssemblerDirectiveTypeUnknown;
    }

    switch(pack_name(str))
    {
        case PACK('%','i','n','c','l','u','d','e','%'): return kAssemblerDirectiveTypeDefine;
        case PACK('%','d','e','f','i','n','e','%'): return kAssemblerDirectiveTypeDefine;
        case PACK('%','u','n','d','e','f','%'): return kAssemblerDirectiveTypeUndefine;
        case PACK('%','i','f','%'): return kAssemblerDirectiveTypeIf;
        case PACK('%','i','f','d','e','f','%'): return kAssemblerDirectiveTypeIfDefined;
        case PACK('%','i','f','n','d','e','f','%'): return kAssemblerDirectiveTypeIfNotDefined;
        case PACK('%','e','l','i','f','%'): return kAssemblerDirectiveTypeElseIf;
        case PACK('%','e','l','s','e','%'): return kAssemblerDirectiveTypeElse;
        case PACK('%','e','n','d','i','f','%'): return kAssemblerDirectiveTypeEndIf;
        default: return kAssemblerDirectiveTypeUnknown;
    }
}
