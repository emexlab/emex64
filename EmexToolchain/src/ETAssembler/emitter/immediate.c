/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Copyright (C) 2026 emexlab
 *
 * This file is part of emex64.
 *
 * emex64 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * emex64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with emex64. If not, see <https://www.gnu.org/licenses/>.
 */

#include <EmexToolchain/ETAssembler/emitter/immediate.h>
#include <EmexToolchain/VM/E64Core.h>

static void assembler_emit_imm_low(ETAssemblerInvocationRef inv,
                                   E64ParameterCoding coding,
                                   UInt64 imm,
                                   UInt8 bits)
{
    EFBitWalkerWrite(inv->out_vbitwalker, coding, 4);
    EFBitWalkerWrite(inv->out_vbitwalker, imm, bits);
}

void assembler_emit_imm5(ETAssemblerInvocationRef inv,
                         UInt8 imm)
{
    assembler_emit_imm_low(inv, kE64ParameterCodingImm4, (UInt64)imm, 4);
}

void assembler_emit_imm8(ETAssemblerInvocationRef inv,
                         UInt8 imm)
{
    assembler_emit_imm_low(inv, kE64ParameterCodingImm8, (UInt64)imm, 8);
}

void assembler_emit_imm16(ETAssemblerInvocationRef inv,
                          UInt16 imm)
{
    assembler_emit_imm_low(inv, kE64ParameterCodingImm16, (UInt64)imm, 16);
}

void assembler_emit_imm32(ETAssemblerInvocationRef inv,
                          UInt32 imm)
{
    assembler_emit_imm_low(inv, kE64ParameterCodingImm32, (UInt64)imm, 32);
}

void assembler_emit_imm64(ETAssemblerInvocationRef inv,
                          UInt64 imm)
{
    assembler_emit_imm_low(inv, kE64ParameterCodingImm64, (UInt64)imm, 64);
}

void assembler_emit_addr64(ETAssemblerInvocationRef inv,
                           UInt64 addr)
{
    EFBitWalkerWrite(inv->out_vbitwalker, kE64ParameterCodingAddr64, 4);
    EFBitWalkerAlignByte(inv->out_vbitwalker);
    EFBitWalkerWrite(inv->out_vbitwalker, addr, 64);
}

void assembler_emit_imm(ETAssemblerInvocationRef inv,
                        UInt64 imm)
{
    if(imm <= 0xF)
    {
        assembler_emit_imm5(inv, (UInt8)imm);
    }
    else if(imm <= 0xFF)
    {
        assembler_emit_imm8(inv, (UInt8)imm);
    }
    else if(imm <= 0xFFFF)
    {
        assembler_emit_imm16(inv, (UInt16)imm);
    }
    else if(imm <= 0xFFFFFFFF)
    {
        assembler_emit_imm32(inv, (UInt32)imm);
    }
    else if(imm <= 0xFFFFFFFFFFFFFFFF)
    {
        assembler_emit_imm64(inv, (UInt64)imm);
    }
}
