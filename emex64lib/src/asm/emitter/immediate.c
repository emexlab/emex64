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

#include <emex64lib/asm/emitter/immediate.h>
#include <emex64lib/vm/core.h>

void assembler_emit_imm5(assembler_invocation_t *inv,
                         uint8_t imm)
{
    vbitwalker_write(inv->out_vbitwalker, kEmex64ParameterCodingImm5, 3);
    vbitwalker_write(inv->out_vbitwalker, imm, 5);
}

void assembler_emit_imm8(assembler_invocation_t *inv,
                         uint8_t imm)
{
    vbitwalker_write(inv->out_vbitwalker, kEmex64ParameterCodingImm8, 3);
    vbitwalker_write(inv->out_vbitwalker, imm, 8);
}

void assembler_emit_imm16(assembler_invocation_t *inv,
                          uint16_t imm)
{
    vbitwalker_write(inv->out_vbitwalker, kEmex64ParameterCodingImm16, 3);
    vbitwalker_write(inv->out_vbitwalker, imm, 16);
}

void assembler_emit_imm32(assembler_invocation_t *inv,
                          uint32_t imm)
{
    vbitwalker_write(inv->out_vbitwalker, kEmex64ParameterCodingImm32, 3);
    vbitwalker_write(inv->out_vbitwalker, imm, 32);
}

void assembler_emit_imm64(assembler_invocation_t *inv,
                          uint64_t imm)
{
    vbitwalker_write(inv->out_vbitwalker, kEmex64ParameterCodingImm64, 3);
    vbitwalker_write(inv->out_vbitwalker, imm, 64);
}

void assembler_emit_addr64(assembler_invocation_t *inv,
                           uint64_t addr)
{
    vbitwalker_write(inv->out_vbitwalker, kEmex64ParameterCodingAddr64, 3);
    vbitwalker_align_byte(inv->out_vbitwalker);
    vbitwalker_write(inv->out_vbitwalker, addr, 64);
}

void assembler_emit_imm(assembler_invocation_t *inv,
                        uint64_t imm)
{
    if(imm <= 0x1F)
    {
        assembler_emit_imm5(inv, (uint8_t)imm);
    }
    else if(imm <= 0xFF)
    {
        assembler_emit_imm8(inv, (uint8_t)imm);
    }
    else if(imm <= 0xFFFF)
    {
        assembler_emit_imm16(inv, (uint16_t)imm);
    }
    else if(imm <= 0xFFFFFFFF)
    {
        assembler_emit_imm32(inv, (uint32_t)imm);
    }
    else if(imm <= 0xFFFFFFFFFFFFFFFF)
    {
        assembler_emit_imm64(inv, (uint64_t)imm);
    }
}
