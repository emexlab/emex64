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

#ifndef __E64FASTMEM_H
#define __E64FASTMEM_H

#include <string.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/likely.h>
#include <EmexToolchain/ETVirtualMachine/E64Memory.h>
#include <EmexToolchain/ETVirtualMachine/__E64Core.h>

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define EMEX64_FASTMEM 1
#else
#define EMEX64_FASTMEM 0
#endif /* defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ */

static inline UInt64 emex64_data_context(__E64Core core)
{
    return (UInt64)core->cr_state.crel.level | ((UInt64)(core->cr_state.crptb.enabled != 0) << 8) | ((UInt64)(core->in_interrupt != 0) << 9) | (1ULL << 15) | (core->tlb_seq << 16);
}

static __attribute__((noinline, unused)) Boolean emex64_fast_fill(__E64Core core,
                                                                   UInt64 addr,
                                                                   UInt64 size,
                                                                   UInt8 action,
                                                                   UInt64 ctx,
                                                                   UInt64 *paddr)
{
    if(unlikely(addr >> 53) || unlikely((addr & 0x1FFF) + size > 0x2000))
    {
        return false;
    }

    const emex64_snoop_t *snoop = core->snoop;
    UInt64 map_seq = __atomic_load_n(&snoop->map_seq, __ATOMIC_ACQUIRE);
    UInt64 ppage;
    if(core->cr_state.crptb.enabled && !core->in_interrupt)
    {
        const emex64_tlb_entry_t *e = emex64_tlb_lookup(&core->tlb, core->snoop, addr);
        if(unlikely(e == NULL))
        {
            return false;
        }
        UInt8 need = action;
        if(action == kE64MemoryActionTypeWrite)
        {
            need |= kE64MMUPTDirty;
        }
        if(core->cr_state.crel.level < kE64ElevationLevelKernel)
        {
            need |= kE64MMUPTUser;
        }
        if(unlikely((e->flags & need) != need))
        {
            return false;
        }
        ppage = e->ppage;
    }
    else
    {
        ppage = addr & ~0x1FFFULL;
    }

    /* the table only holds whole pages of RAM */
    if(unlikely(ppage >= core->ram_size) || unlikely(core->ram_size - ppage < 0x2000))
    {
        return false;
    }

    UInt64 pa = ppage + (addr & 0x1FFF);
    if(action == kE64MemoryActionTypeWrite)
    {
        UInt64 ktrr_base = snoop->ktrr_base;
        UInt64 ktrr_size = snoop->ktrr_size;
        if(unlikely(ktrr_size != 0) && ppage < ktrr_base + ktrr_size && ppage + 0x2000 > ktrr_base)
        {
            /* a page KTRR reaches into never goes into the table, each write to it is looked at */
            if(pa < ktrr_base + ktrr_size && pa + size > ktrr_base)
            {
                return false;
            }
            *paddr = pa;
            return true;
        }
    }

    emex64_fast_page_t *page = &core->pages[action == kE64MemoryActionTypeWrite][(addr >> 13) & (EMEX64_FAST_PAGES - 1)];
    page->key = addr & ~0x1FFFULL;
    page->ppage = ppage;
    page->map_seq = map_seq;
    page->ctx = ctx;

    *paddr = pa;
    return true;
}

static inline __attribute__((always_inline)) Boolean emex64_hot_paddr(__E64Core core,
                                                                      UInt64 ctx,
                                                                      UInt64 addr,
                                                                      UInt64 size,
                                                                      UInt8 action,
                                                                      UInt64 *paddr)
{
#if EMEX64_FASTMEM
    const emex64_fast_page_t *page = &core->pages[action == kE64MemoryActionTypeWrite][(addr >> 13) & (EMEX64_FAST_PAGES - 1)];
    if(likely(page->key == (addr & ~0x1FFFULL)) && likely(page->ctx == ctx) &&
       likely(page->map_seq == __atomic_load_n(&core->snoop->map_seq, __ATOMIC_ACQUIRE)) &&
       likely((addr & 0x1FFF) + size <= 0x2000))
    {
        *paddr = page->ppage + (addr & 0x1FFF);
        return true;
    }
#endif /* EMEX64_FASTMEM */
    return false;
}

static inline Boolean emex64_fast_paddr(__E64Core core,
                                        UInt64 addr,
                                        UInt64 size,
                                        UInt8 action,
                                        UInt64 *paddr)
{
#if EMEX64_FASTMEM
    if(unlikely(emex64_instruction_faulted(core)))
    {
        return false;
    }

    UInt64 ctx = emex64_data_context(core);
    if(likely(emex64_hot_paddr(core, ctx, addr, size, action, paddr)))
    {
        return true;
    }
    return emex64_fast_fill(core, addr, size, action, ctx, paddr);
#else
    return false;
#endif /* EMEX64_FASTMEM */
}

static inline __attribute__((always_inline)) Boolean emex64_fast_load(__E64Core core,
                                                                      UInt64 addr,
                                                                      UInt64 size,
                                                                      UInt64 *value)
{
    UInt64 pa;
    if(likely(emex64_fast_paddr(core, addr, size, kE64MemoryActionTypeRead, &pa)))
    {
        UInt64 v = 0;
        memcpy(&v, core->ram + pa, size);
        *value = v;
        return true;
    }
    return false;
}

static inline __attribute__((always_inline)) Boolean emex64_fast_store(__E64Core core,
                                                                       UInt64 addr,
                                                                       UInt64 size,
                                                                       UInt64 value)
{
    UInt64 pa;
    if(likely(emex64_fast_paddr(core, addr, size, kE64MemoryActionTypeWrite, &pa)))
    {
        memcpy(core->ram + pa, &value, size);
        emex64_snoop_write(core->snoop, pa, size);
        return true;
    }
    return false;
}

static inline __attribute__((always_inline)) Boolean emex64_hot_load(__E64Core core,
                                                                     UInt64 ctx,
                                                                     UInt64 addr,
                                                                     UInt64 size,
                                                                     UInt64 *value)
{
    UInt64 pa;
    if(likely(emex64_hot_paddr(core, ctx, addr, size, kE64MemoryActionTypeRead, &pa)))
    {
        UInt64 v = 0;
        memcpy(&v, core->ram + pa, size);
        *value = v;
        return true;
    }
    return false;
}

static inline __attribute__((always_inline)) Boolean emex64_hot_store(__E64Core core,
                                                                      UInt64 ctx,
                                                                      UInt64 addr,
                                                                      UInt64 size,
                                                                      UInt64 value,
                                                                      Boolean *touched)
{
    UInt64 pa;
    if(likely(emex64_hot_paddr(core, ctx, addr, size, kE64MemoryActionTypeWrite, &pa)))
    {
        memcpy(core->ram + pa, &value, size);
        *touched = emex64_snoop_write(core->snoop, pa, size);
        return true;
    }
    return false;
}

#endif /* __E64FASTMEM_H */
