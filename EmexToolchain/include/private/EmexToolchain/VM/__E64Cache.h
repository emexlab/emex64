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

#ifndef __E64CACHE_H
#define __E64CACHE_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Type.h>

#define EMEX64_CHUNK_SHIFT          8
#define EMEX64_CHUNK_SIZE           (1ULL << EMEX64_CHUNK_SHIFT)

#define EMEX64_CHUNK_CODE           (1 << 0)
#define EMEX64_CHUNK_PAGE_TABLE     (1 << 1)
#define EMEX64_CHUNK_SCANOUT        (1 << 2)
#define EMEX64_CHUNK_PTE            (1 << 3)

typedef struct emex64_snoop {
    UInt8 *flags;
    UInt32 *gen;
    UInt64 chunks;
    UInt64 tlb_epoch;
    UInt64 scan_lo;
    UInt64 scan_hi;
    Boolean ktrr_locked;
    UInt64 ktrr_base;
    UInt64 ktrr_size;
} emex64_snoop_t;

static inline void emex64_snoop_scan_written(emex64_snoop_t *snoop, UInt64 c)
{
    UInt64 lo = __atomic_load_n(&snoop->scan_lo, __ATOMIC_RELAXED);
    while(c < lo && !__atomic_compare_exchange_n(&snoop->scan_lo, &lo, c, true, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
    {
    }
    UInt64 hi = __atomic_load_n(&snoop->scan_hi, __ATOMIC_RELAXED);
    while(c + 1 > hi && !__atomic_compare_exchange_n(&snoop->scan_hi, &hi, c + 1, true, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
    {
    }
}

static inline void emex64_snoop_write(emex64_snoop_t *snoop,
                                      UInt64 paddr,
                                      UInt64 len)
{
    if(len == 0)
    {
        return;
    }

    UInt64 c = paddr >> EMEX64_CHUNK_SHIFT;
    UInt64 last = (paddr + len - 1) >> EMEX64_CHUNK_SHIFT;
    for(; c <= last; c++)
    {
        if(__builtin_expect(__atomic_load_n(&snoop->flags[c], __ATOMIC_RELAXED) != 0, 0))
        {
            UInt8 f = __atomic_exchange_n(&snoop->flags[c], 0, __ATOMIC_ACQ_REL);
            if(f & (EMEX64_CHUNK_CODE | EMEX64_CHUNK_PTE))
            {
                __atomic_fetch_add(&snoop->gen[c], 1, __ATOMIC_ACQ_REL);
            }
            if(f & EMEX64_CHUNK_PAGE_TABLE)
            {
                __atomic_fetch_add(&snoop->tlb_epoch, 1, __ATOMIC_ACQ_REL);
            }
            if(f & EMEX64_CHUNK_SCANOUT)
            {
                emex64_snoop_scan_written(snoop, c);
            }
        }
    }
}

static inline void emex64_snoop_mark(emex64_snoop_t *snoop,
                                     UInt64 paddr,
                                     UInt8 what)
{
    UInt8 *f = &snoop->flags[paddr >> EMEX64_CHUNK_SHIFT];
    if((__atomic_load_n(f, __ATOMIC_ACQUIRE) & what) != what)
    {
        __atomic_fetch_or(f, what, __ATOMIC_ACQ_REL);
    }
}

static inline UInt32 emex64_snoop_gen(const emex64_snoop_t *snoop,
                                      UInt64 paddr)
{
    return __atomic_load_n(&snoop->gen[paddr >> EMEX64_CHUNK_SHIFT], __ATOMIC_RELAXED);
}

/* did someone paw at me giving me a note saying "TLB" on it? >~< */
#define EMEX64_TLB_ENTRIES          1024
#define EMEX64_TLB_ROOTS            256
#define EMEX64_ASID_SHIFT           48
#define EMEX64_PTB_PFN_MASK         0x0000FFFFFFFFFF00ULL

typedef struct emex64_tlb_entry {
    UInt64 vpn;
    UInt64 ppage;
    UInt64 epoch;
    UInt64 pte_chunk;
    UInt32 pte_gen;
    UInt16 asid;
    UInt8 flags;
    Boolean valid;
} emex64_tlb_entry_t;

typedef struct emex64_tlb {
    emex64_tlb_entry_t entries[EMEX64_TLB_ENTRIES];
    UInt16 asid;
    struct {
        UInt64 root;
        UInt16 asid;
        Boolean valid;
    } roots[EMEX64_TLB_ROOTS];
} emex64_tlb_t;

static inline emex64_tlb_entry_t *emex64_tlb_slot(emex64_tlb_t *tlb,
                                                  UInt64 vpn,
                                                  UInt16 asid)
{
    return &tlb->entries[(vpn ^ (vpn >> 10) ^ ((UInt64)asid * 0x9E5)) & (EMEX64_TLB_ENTRIES - 1)];
}

static inline emex64_tlb_entry_t *emex64_tlb_lookup(emex64_tlb_t *tlb,
                                                    const emex64_snoop_t *snoop,
                                                    UInt64 vaddr)
{
    UInt64 vpn = vaddr >> 13;
    emex64_tlb_entry_t *e = emex64_tlb_slot(tlb, vpn, tlb->asid);
    if(e->valid && e->vpn == vpn && e->asid == tlb->asid &&
       e->epoch == __atomic_load_n(&snoop->tlb_epoch, __ATOMIC_RELAXED) &&
       e->pte_gen == __atomic_load_n(&snoop->gen[e->pte_chunk], __ATOMIC_RELAXED))
    {
        return e;
    }
    return NULL;
}

/* now things go hella faaaassstt :3c */
#define EMEX64_ICACHE_ENTRIES       8192
#define EMEX64_ICACHE_ARGS          8

typedef enum: UInt8 {
    kE64OperandReg,
    kE64OperandRegExtended,
    kE64OperandImm,
    kE64OperandRegInc,
    kE64OperandRegDec,
    kE64OperandOffset,
    kE64OperandRegFloat,
} E64OperandKind;

typedef struct emex64_operand_recipe {
    UInt8 kind;
    UInt8 reg;
    UInt8 base_kind;
    UInt8 base_reg;
    UInt8 off_kind;
    UInt8 off_reg;
    Boolean add;
    UInt8 pad;
    UInt64 imm;
    UInt64 off_imm;
} emex64_operand_recipe_t;

struct emex64_opfunc_entry;

typedef struct emex64_icache_entry {
    UInt64 paddr;
    UInt32 gen_first;
    UInt32 gen_last;
    const struct emex64_opfunc_entry *opce;
    UInt8 ilen;
    UInt8 opcode;
    UInt8 param_cnt;
    Boolean rollback;
    Boolean save_erl;
    UInt16 save_rl;
    UInt8 dyn_cnt;
    UInt8 dyn[EMEX64_ICACHE_ARGS];
    UInt64 *param[EMEX64_ICACHE_ARGS];
    E64ParameterCoding param_coding[16];
    emex64_operand_recipe_t operands[EMEX64_ICACHE_ARGS];
} emex64_icache_entry_t;

static inline UInt64 emex64_icache_index(UInt64 paddr)
{
    return (paddr ^ (paddr >> 13)) & (EMEX64_ICACHE_ENTRIES - 1);
}

#endif /* __E64CACHE_H */
