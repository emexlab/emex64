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
#include <EmexToolchain/VM/E64Memory.h>
#include <EmexToolchain/VM/__E64Core.h>

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define EMEX64_FASTMEM 1
#else
#define EMEX64_FASTMEM 0
#endif

static inline Boolean emex64_fast_paddr(__E64Core core,
                                        UInt64 addr,
                                        UInt64 size,
                                        UInt8 action,
                                        UInt64 *paddr)
{
#if EMEX64_FASTMEM
    if(unlikely(core->cr_state.crexc.exception != kE64ExceptionNone) || unlikely(addr >> 53))
    {
        return false;
    }

    UInt64 pa;
    if(core->cr_state.crptb.enabled && !core->in_interrupt)
    {
        if(unlikely((addr & 0x1FFF) + size > 0x2000))
        {
            return false;
        }

        UInt64 key = (addr & ~0x1FFFULL) | core->cr_state.crel.level;
        UInt64 map_seq = __atomic_load_n(&core->snoop->map_seq, __ATOMIC_ACQUIRE);
        emex64_fast_page_t *page = &core->pages[action == kE64MemoryActionTypeWrite][(addr >> 13) & (EMEX64_FAST_PAGES - 1)];
        if(likely(page->key == key) && likely(page->map_seq == map_seq) && likely(page->tlb_seq == core->tlb_seq))
        {
            pa = page->ppage + (addr & 0x1FFF);
        }
        else
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
            pa = e->ppage + (addr & 0x1FFF);
            page->key = key;
            page->ppage = e->ppage;
            page->map_seq = map_seq;
            page->tlb_seq = core->tlb_seq;
        }
    }
    else
    {
        if(unlikely(addr >= core->ram_size) || unlikely(size > core->ram_size - addr))
        {
            return false;
        }
        pa = addr;
    }

    if(action == kE64MemoryActionTypeWrite)
    {
        const emex64_snoop_t *snoop = core->snoop;
        if(unlikely(snoop->ktrr_size != 0) && pa < snoop->ktrr_base + snoop->ktrr_size && pa + size > snoop->ktrr_base)
        {
            return false;
        }
    }

    *paddr = pa;
    return true;
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

#endif /* __E64FASTMEM_H */
