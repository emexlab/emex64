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

#include <emex64asm/register.h>
#include <stdlib.h>
#include <string.h>

register_entry_t register_table[] = {
    /* special purpose register */
    { .name = "pc", .reg = LA64_REGISTER_PC },
    { .name = "sp", .reg = LA64_REGISTER_SP },
    { .name = "fp", .reg = LA64_REGISTER_FP },
    { .name = "cf", .reg = LA64_REGISTER_CF },

    /* general purpose register */
    { .name = "r0", .reg = LA64_REGISTER_R0 },
    { .name = "r1", .reg = LA64_REGISTER_R1 },
    { .name = "r2", .reg = LA64_REGISTER_R2 },
    { .name = "r3", .reg = LA64_REGISTER_R3 },
    { .name = "r4", .reg = LA64_REGISTER_R4 },
    { .name = "r5", .reg = LA64_REGISTER_R5 },
    { .name = "r6", .reg = LA64_REGISTER_R6 },
    { .name = "r7", .reg = LA64_REGISTER_R7 },
    { .name = "r8", .reg = LA64_REGISTER_R8 },
    { .name = "r9", .reg = LA64_REGISTER_R9 },
    { .name = "r10", .reg = LA64_REGISTER_R10 },
    { .name = "r11", .reg = LA64_REGISTER_R11 },
    { .name = "r12", .reg = LA64_REGISTER_R12 },
    { .name = "r13", .reg = LA64_REGISTER_R13 },
    { .name = "r14", .reg = LA64_REGISTER_R14 },
    { .name = "r15", .reg = LA64_REGISTER_R15 },
    { .name = "r16", .reg = LA64_REGISTER_R16 },
    { .name = "rr",  .reg = LA64_REGISTER_RR },
    { .name = "cr0", .reg = LA64_REGISTER_CR0 },
    { .name = "cr1", .reg = LA64_REGISTER_CR1 },
    { .name = "cr2", .reg = LA64_REGISTER_CR2 },
    { .name = "cr3", .reg = LA64_REGISTER_CR3 },
    { .name = "cr4", .reg = LA64_REGISTER_CR4 },
    { .name = "cr5", .reg = LA64_REGISTER_CR5 },
    { .name = "cr6", .reg = LA64_REGISTER_CR6 },
    { .name = "cr7", .reg = LA64_REGISTER_CR7 },
    { .name = "cr8", .reg = LA64_REGISTER_CR8 },
    { .name = "cr9", .reg = LA64_REGISTER_CR9 },

    /* register nicknames */
    { .name = "r17",  .reg = LA64_REGISTER_RR },
    { .name = "crel", .reg = LA64_REGISTER_CR0 },
    { .name = "crksp", .reg = LA64_REGISTER_CR1 },
    { .name = "crexc", .reg = LA64_REGISTER_CR2 },
    { .name = "crvec", .reg = LA64_REGISTER_CR3 },
    { .name = "crptb", .reg = LA64_REGISTER_CR4 },
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