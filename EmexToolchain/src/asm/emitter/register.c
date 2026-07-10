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

#include <assert.h>
#include <EmexToolchain/support/pack.h>
#include <EmexToolchain/asm/emitter/register.h>

E64Register register_from_string(const char *name)
{
    switch(pack_name(name))
    {
        case PACK('p','c'): return kE64RegisterPC;
        case PACK('s','p'): return kE64RegisterSP;
        case PACK('f','p'): return kE64RegisterFP;
        case PACK('f','p','c'): return kE64RegisterFPC;
        case PACK('r','0'): return kE64RegisterR0;
        case PACK('r','1'): return kE64RegisterR1;
        case PACK('r','2'): return kE64RegisterR2;
        case PACK('r','3'): return kE64RegisterR3;
        case PACK('r','4'): return kE64RegisterR4;
        case PACK('r','5'): return kE64RegisterR5;
        case PACK('r','6'): return kE64RegisterR6;
        case PACK('r','7'): return kE64RegisterR7;
        case PACK('r','8'): return kE64RegisterR8;
        case PACK('r','9'): return kE64RegisterR9;
        case PACK('r','r'): return kE64RegisterRR;
        default: return kE64RegisterInvalid;
    }
}

void assembler_emit_register(assembler_invocation_t *inv,
                             E64Register reg)
{
    assert(reg <= kE64RegisterMAX);

    vbitwalker_write(inv->out_vbitwalker, kE64ParameterCodingReg, 4);
    vbitwalker_write(inv->out_vbitwalker, reg, 4);
}
