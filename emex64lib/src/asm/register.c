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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <emex64lib/support/pack.h>

#include <emex64lib/asm/register.h>

enum kEmex64Register register_from_string(const char *name, bool *success)
{
    if(name == NULL)
    {
        *success = false;
        return kEmex64RegisterPC;
    }

    *success = true;

    switch(pack_name(name))
    {
        case PACK('p','c'): return kEmex64RegisterPC;
        case PACK('s','p'): return kEmex64RegisterSP;
        case PACK('f','p'): return kEmex64RegisterFP;
        case PACK('f','p','c'): return kEmex64RegisterFPC;
        case PACK('r','0'): return kEmex64RegisterR0;
        case PACK('r','1'): return kEmex64RegisterR1;
        case PACK('r','2'): return kEmex64RegisterR2;
        case PACK('r','3'): return kEmex64RegisterR3;
        case PACK('r','4'): return kEmex64RegisterR4;
        case PACK('r','5'): return kEmex64RegisterR5;
        case PACK('r','6'): return kEmex64RegisterR6;
        case PACK('r','7'): return kEmex64RegisterR7;
        case PACK('r','8'): return kEmex64RegisterR8;
        case PACK('r','9'): return kEmex64RegisterR9;
        case PACK('r','1','0'): return kEmex64RegisterR10;
        case PACK('r','1','1'): return kEmex64RegisterR11;
        case PACK('r','1','2'): return kEmex64RegisterR12;
        case PACK('r','1','3'): return kEmex64RegisterR13;
        case PACK('r','1','4'): return kEmex64RegisterR14;
        case PACK('r','1','5'): return kEmex64RegisterR15;
        case PACK('r','1','6'): return kEmex64RegisterR16;
        case PACK('r','1','7'): return kEmex64RegisterR17;
        case PACK('r','1','8'): return kEmex64RegisterR18;
        case PACK('r','1','9'): return kEmex64RegisterR19;
        case PACK('r','2','0'): return kEmex64RegisterR20;
        case PACK('r','2','1'): return kEmex64RegisterR21;
        case PACK('r','2','2'): return kEmex64RegisterR22;
        case PACK('r','2','3'): return kEmex64RegisterR23;
        case PACK('r','2','4'): return kEmex64RegisterR24;
        case PACK('r','2','5'): return kEmex64RegisterR25;
        case PACK('r','2','6'): return kEmex64RegisterRR;
        case PACK('r','r'): return kEmex64RegisterRR;
        default:
            *success = false;
            return kEmex64RegisterPC;
    }
}
