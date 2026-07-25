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

#ifndef EMEX64_PACK_H
#define EMEX64_PACK_H

#include <EmexFoundation/EmexFoundation.h>

#define P7(a) ((UInt64)((a) & 0x7F))
#define PACK1(a) P7(a)
#define PACK2(a,b) (PACK1(a) | (P7(b) << 7))
#define PACK3(a,b,c) (PACK2(a,b) | (P7(c) << 14))
#define PACK4(a,b,c,d) (PACK3(a,b,c) | (P7(d) << 21))
#define PACK5(a,b,c,d,e) (PACK4(a,b,c,d) | (P7(e) << 28))
#define PACK6(a,b,c,d,e,f) (PACK5(a,b,c,d,e) | (P7(f) << 35))
#define PACK7(a,b,c,d,e,f,g) (PACK6(a,b,c,d,e,f) | (P7(g) << 42))
#define PACK8(a,b,c,d,e,f,g,h) (PACK7(a,b,c,d,e,f,g) | (P7(h) << 49))
#define PACK9(a,b,c,d,e,f,g,h,i) (PACK8(a,b,c,d,e,f,g,h) | (P7(i) << 56))

#define PACK_CAT_(a,b) a##b
#define PACK_CAT(a,b)  PACK_CAT_(a,b)
#define PACK_NARG(...) PACK_NARG_(__VA_ARGS__, e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e,e, 9,8,7,6,5,4,3,2,1,0)
#define PACK_NARG_( _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16, _17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32, N, ...) N
#define PACKe(...) ((UInt64)0 * sizeof(struct { static_assert(0, "PACK: name has more than 9 characters"); char _c; }))
#define PACK(...) PACK_CAT(PACK, PACK_NARG(__VA_ARGS__))(__VA_ARGS__)

static inline UInt64 pack_name(const char *s)
{
    if(s == NULL)
    {
        return UINT64_MAX;
    }

    UInt64 v = 0;
    SInt32 i = 0;
    for(; i < 9 && s[i]; i++)
    {
        v |= (UInt64)(s[i] & 0x7F) << (i * 7);
    }
    return s[i] ? UINT64_MAX : v;
}

static inline UInt64 pack_name_until(const char *s,
                                     char delimiter)
{
    if(s == NULL)
    {
        return UINT64_MAX;
    }

    UInt64 v = 0;
    SInt32 i = 0;
    for(; i < 9 && s[i] && s[i] != delimiter; i++)
    {
        v |= (UInt64)(s[i] & 0x7F) << (i * 7);
    }
    return (i == 9 && s[i] && s[i] != delimiter) ? UINT64_MAX : v;
}

static inline SInt32 get_packed_len(UInt64 packed)
{
    if(packed == UINT64_MAX)
    {
        return 0;
    }
    
    SInt32 len = 0;
    for(SInt32 i = 0; i < 9; i++)
    {
        if((packed >> (i * 7)) & 0x7F)
        {
            len = i + 1;
        }
    }
    return len;
}

static inline Boolean has_packed_prefix(UInt64 packed_name,
                                        UInt64 search_for)
{
    if(packed_name == UINT64_MAX || search_for == UINT64_MAX)
    {
        return false;
    }
    if(search_for == 0)
    {
        return true;
    }

    SInt32 search_len = get_packed_len(search_for);
    SInt32 bits = search_len * 7;
    UInt64 mask = ((UInt64)1 << bits) - 1;
    return (packed_name & mask) == search_for;
}

static inline Boolean has_packed_suffix(UInt64 packed_name,
                                        UInt64 search_for)
{
    if(packed_name == UINT64_MAX || search_for == UINT64_MAX)
    {
        return false;
    }
    if(search_for == 0)
    {
        return true;
    }

    SInt32 name_len = get_packed_len(packed_name);
    SInt32 search_len = get_packed_len(search_for);

    if(search_len > name_len)
    {
        return false;
    }

    SInt32 shift_bits = (name_len - search_len) * 7;
    UInt64 shifted_name = packed_name >> shift_bits;

    return shifted_name == search_for;
}

#endif /* EMEX64_PACK_H */
