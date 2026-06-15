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

#include <stdlib.h>
#include <string.h>

#include <emex64lib/asm/register.h>

register_entry_t register_table[] = {
    /* special purpose register */
    { .name = "pc", .reg = kEmex64RegisterPC },
    { .name = "sp", .reg = kEmex64RegisterSP },
    { .name = "fp", .reg = kEmex64RegisterFP },
    { .name = "cf", .reg = kEmex64RegisterCF },
    { .name = "fpc", .reg = kEmex64RegisterFPC },

    /* general purpose register */
    { .name = "r0", .reg = kEmex64RegisterR0 },
    { .name = "r1", .reg = kEmex64RegisterR1 },
    { .name = "r2", .reg = kEmex64RegisterR2 },
    { .name = "r3", .reg = kEmex64RegisterR3 },
    { .name = "r4", .reg = kEmex64RegisterR4 },
    { .name = "r5", .reg = kEmex64RegisterR5 },
    { .name = "r6", .reg = kEmex64RegisterR6 },
    { .name = "r7", .reg = kEmex64RegisterR7 },
    { .name = "r8", .reg = kEmex64RegisterR8 },
    { .name = "r9", .reg = kEmex64RegisterR9 },
    { .name = "r10", .reg = kEmex64RegisterR10 },
    { .name = "r11", .reg = kEmex64RegisterR11 },
    { .name = "r12", .reg = kEmex64RegisterR12 },
    { .name = "r13", .reg = kEmex64RegisterR13 },
    { .name = "r14", .reg = kEmex64RegisterR14 },
    { .name = "r15", .reg = kEmex64RegisterR15 },
    { .name = "r16", .reg = kEmex64RegisterR16 },
    { .name = "r17", .reg = kEmex64RegisterR17 },
    { .name = "r18", .reg = kEmex64RegisterR18 },
    { .name = "r19", .reg = kEmex64RegisterR19 },
    { .name = "r20", .reg = kEmex64RegisterR20 },
    { .name = "r21", .reg = kEmex64RegisterR21 },
    { .name = "r22", .reg = kEmex64RegisterR22 },
    { .name = "r23", .reg = kEmex64RegisterR23 },
    { .name = "r24", .reg = kEmex64RegisterR24 },
    { .name = "r25", .reg = kEmex64RegisterR25 },
    { .name = "rr",  .reg = kEmex64RegisterRR },

    /* register nicknames */
    { .name = "r26",  .reg = kEmex64RegisterRR },
};

register_entry_t *register_from_string(const char *name)
{
    /* null pointer check */
    if(name == NULL)
    {
        return NULL;
    }

    /* iterating through table */
    for(unsigned char reg = 0x00; reg < (sizeof(register_table) / sizeof(register_table[0])); reg++)
    {
        /* check if opcode name matches */
        if(strcmp(register_table[reg].name, name) == 0)
        {
            return &register_table[reg];
        }
    }

    /* shouldnt happen if code is correct */
    return NULL;
}
