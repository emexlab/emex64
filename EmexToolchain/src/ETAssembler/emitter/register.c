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
#include <EmexToolchain/Support/pack.h>
#include <EmexToolchain/ETAssembler/emitter/register.h>

static inline E64Register __register_from_string(const char *name)
{
    switch(pack_name_until(name, '+'))
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
    }

    switch(pack_name_until(name, '-'))
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

static inline E64RegisterExtended __register_extended_from_string(const char *name)
{
    switch(pack_name(name))
    {
        case PACK('e','r','0'): return kE64RegisterExtendedER0;
        case PACK('e','r','1'): return kE64RegisterExtendedER1;
        case PACK('e','r','2'): return kE64RegisterExtendedER2;
        case PACK('e','r','3'): return kE64RegisterExtendedER3;
        case PACK('e','r','4'): return kE64RegisterExtendedER4;
        case PACK('e','r','5'): return kE64RegisterExtendedER5;
        case PACK('e','r','6'): return kE64RegisterExtendedER6;
        case PACK('e','r','7'): return kE64RegisterExtendedER7;
        case PACK('e','r','8'): return kE64RegisterExtendedER8;
        case PACK('e','r','9'): return kE64RegisterExtendedER9;
        case PACK('e','r','1','0'): return kE64RegisterExtendedER10;
        case PACK('e','r','1','1'): return kE64RegisterExtendedER11;
        case PACK('e','r','1','2'): return kE64RegisterExtendedER12;
        case PACK('e','r','1','3'): return kE64RegisterExtendedER13;
        case PACK('e','r','1','4'): return kE64RegisterExtendedER14;
        case PACK('e','r','1','5'): return kE64RegisterExtendedER15;
        default: return kE64RegisterExtendedInvalid;
    }
}

E64RegisterIdentifier register_from_string(const char *name)
{
    E64RegisterIdentifier identifier;

    E64Register reg = __register_from_string(name);
    E64RegisterExtended ereg = __register_extended_from_string(name);

    identifier.valid = true;
    if(reg != kE64RegisterInvalid)
    {
        identifier.isExtended = false;
        identifier.value.base = reg;
        UInt64 packed = pack_name(name);
        identifier.decrement = has_packed_suffix(packed, PACK('-','-'));
        identifier.increment = identifier.decrement || has_packed_suffix(packed, PACK('+','+'));
        return identifier;
    }
    else if(ereg != kE64RegisterExtendedInvalid)
    {
        identifier.isExtended = true;
        identifier.value.extended = ereg;
        identifier.decrement = false;
        identifier.increment = false;
        return identifier;
    }
    else
    {
        identifier.valid = false;
        return identifier;
    }
}

void assembler_emit_register(assembler_invocation_t *inv,
                             E64Register reg,
                             Boolean increment,
                             Boolean actuallyDecrement)
{
    assert(reg <= kE64RegisterMAX);

    if(increment && actuallyDecrement)
    {
        EFBitWalkerWrite(inv->out_vbitwalker, kE64ParameterCodingRegImmDec, 4);
    }
    else if(increment)
    {
        EFBitWalkerWrite(inv->out_vbitwalker, kE64ParameterCodingRegImmInc, 4);
    }
    else
    {
        EFBitWalkerWrite(inv->out_vbitwalker, kE64ParameterCodingReg, 4);
    }
    EFBitWalkerWrite(inv->out_vbitwalker, reg, 4);
}

void assembler_emit_register_extended(assembler_invocation_t *inv,
                                      E64RegisterExtended reg)
{
    assert(reg <= kE64RegisterExtendedMAX);

    EFBitWalkerWrite(inv->out_vbitwalker, kE64ParameterCodingRegExtended, 4);
    EFBitWalkerWrite(inv->out_vbitwalker, reg, 4);
}
