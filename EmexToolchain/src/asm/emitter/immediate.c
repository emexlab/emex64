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

#include <EmexToolchain/asm/emitter/immediate.h>
#include <EmexToolchain/vm/core.h>

void assembler_emit_imm5(assembler_invocation_t *inv,
                         uint8_t imm)
{
    vbitwalker_write(inv->out_vbitwalker, kE64ParameterCodingImm5, 3);
    vbitwalker_write(inv->out_vbitwalker, imm, 5);
}

void assembler_emit_imm8(assembler_invocation_t *inv,
                         uint8_t imm)
{
    vbitwalker_write(inv->out_vbitwalker, kE64ParameterCodingImm8, 3);
    vbitwalker_write(inv->out_vbitwalker, imm, 8);
}

void assembler_emit_imm16(assembler_invocation_t *inv,
                          uint16_t imm)
{
    vbitwalker_write(inv->out_vbitwalker, kE64ParameterCodingImm16, 3);
    vbitwalker_write(inv->out_vbitwalker, imm, 16);
}

void assembler_emit_imm32(assembler_invocation_t *inv,
                          uint32_t imm)
{
    vbitwalker_write(inv->out_vbitwalker, kE64ParameterCodingImm32, 3);
    vbitwalker_write(inv->out_vbitwalker, imm, 32);
}

void assembler_emit_imm64(assembler_invocation_t *inv,
                          uint64_t imm)
{
    vbitwalker_write(inv->out_vbitwalker, kE64ParameterCodingImm64, 3);
    vbitwalker_write(inv->out_vbitwalker, imm, 64);
}

void assembler_emit_addr64(assembler_invocation_t *inv,
                           uint64_t addr)
{
    vbitwalker_write(inv->out_vbitwalker, kE64ParameterCodingAddr64, 3);
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
