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

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/instruction/ctrl.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/Support/likely.h>

void emex64_op_b(__E64Core core)
{
    core->op.ilen = 0;
    core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[0]), core->op.param_coding[0]);
}

void emex64_op_cmp(__E64Core core)
{
    SInt64 a = (SInt64)*(core->op.param[0]);
    SInt64 b = (SInt64)*(core->op.param[1]);

    core->rl[kE64RegisterCF] = (a == b) * kE64CompareFlagZ | (a <  b) * kE64CompareFlagL | (a >  b) * kE64CompareFlagG;
}

void emex64_op_be(__E64Core core)
{
    if(core->rl[kE64RegisterCF] & kE64CompareFlagZ)
    {
        emex64_op_b(core);
    }
}

void emex64_op_bne(__E64Core core)
{
    if(!(core->rl[kE64RegisterCF] & kE64CompareFlagZ))
    {
        emex64_op_b(core);
    }
}

void emex64_op_blt(__E64Core core)
{
    if(core->rl[kE64RegisterCF] & kE64CompareFlagL)
    {
        emex64_op_b(core);
    }
}

void emex64_op_bgt(__E64Core core)
{
    if(core->rl[kE64RegisterCF] & kE64CompareFlagG)
    {
        emex64_op_b(core);
    }
}

void emex64_op_ble(__E64Core core)
{
    if(core->rl[kE64RegisterCF] & (kE64CompareFlagL | kE64CompareFlagZ))
    {
        emex64_op_b(core);
    }
}

void emex64_op_bge(__E64Core core)
{
    if(core->rl[kE64RegisterCF] & (kE64CompareFlagG | kE64CompareFlagZ))
    {
        emex64_op_b(core);
    }
}

void emex64_op_bz(__E64Core core)
{
    if(*(core->op.param[0]) == 0)
    {
        core->op.ilen = 0;
        core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[1]), core->op.param_coding[1]);
    }
}

void emex64_op_bnz(__E64Core core)
{
    if(*(core->op.param[0]) != 0)
    {
        core->op.ilen = 0;
        core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[1]), core->op.param_coding[1]);
    }
}

/* call convention not needed, emex64 supports arguments directly in bl (biggest L ever, we gotta make a call convention) */
void emex64_op_blw(__E64Core core)
{
    /* backup all parameters from pointer to not malform double passed registers for example */
    UInt64 param_imm[32] = {};
    for(UInt8 i = 0; i < core->op.param_cnt; i++)
    {
        param_imm[i] = *(core->op.param[i]);
    }

    /* pushing all relevant registers onto stack */
    emex64_push_il(core, core->rl[kE64RegisterPC] + core->op.ilen);
    emex64_push_il(core, core->rl[kE64RegisterFP]);
    emex64_push_il(core, core->rl[kE64RegisterCF]);
    emex64_push_il(core, core->rl[kE64RegisterFPC]);
    emex64_push_il(core, core->rl[kE64RegisterR0]);
    emex64_push_il(core, core->rl[kE64RegisterR1]);
    emex64_push_il(core, core->rl[kE64RegisterR2]);
    emex64_push_il(core, core->rl[kE64RegisterR3]);
    emex64_push_il(core, core->rl[kE64RegisterR4]);
    emex64_push_il(core, core->rl[kE64RegisterR5]);
    emex64_push_il(core, core->rl[kE64RegisterR6]);
    emex64_push_il(core, core->rl[kE64RegisterR7]);
    emex64_push_il(core, core->rl[kE64RegisterR8]);
    emex64_push_il(core, core->rl[kE64RegisterR9]);

    /* writing parameters */
    for(UInt8 i = 1; i < core->op.param_cnt && i < (kE64RegisterR9 - 1); i++)
    {
        core->rl[(kE64RegisterR0 - 1) + i] = param_imm[i];
    }

    /* setting current frame pointer to stack pointer to point to stack frame */
    core->rl[kE64RegisterFP] = core->rl[kE64RegisterSP];

    /* initiating jump */
    core->op.ilen = 0;
    core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], param_imm[0], core->op.param_coding[0]);
}

void emex64_op_wret(__E64Core core)
{
    core->rl[kE64RegisterSP] = core->rl[kE64RegisterFP];

    core->rl[kE64RegisterR9] = emex64_pop_il(core);
    core->rl[kE64RegisterR8] = emex64_pop_il(core);
    core->rl[kE64RegisterR7] = emex64_pop_il(core);
    core->rl[kE64RegisterR6] = emex64_pop_il(core);
    core->rl[kE64RegisterR5] = emex64_pop_il(core);
    core->rl[kE64RegisterR4] = emex64_pop_il(core);
    core->rl[kE64RegisterR3] = emex64_pop_il(core);
    core->rl[kE64RegisterR2] = emex64_pop_il(core);
    core->rl[kE64RegisterR1] = emex64_pop_il(core);
    core->rl[kE64RegisterR0] = emex64_pop_il(core);
    core->rl[kE64RegisterFPC] = emex64_pop_il(core);
    core->rl[kE64RegisterCF] = emex64_pop_il(core);
    core->rl[kE64RegisterFP] = emex64_pop_il(core);
    core->rl[kE64RegisterPC] = emex64_pop_il(core);
    core->op.ilen = 0;
}

void emex64_op_iret(__E64Core core)
{
    if(!core->in_interrupt)
    {
        core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
        return;
    }

    core->rl[kE64RegisterSP] = core->rl[kE64RegisterFP];

    core->rl[kE64RegisterR9] = emex64_pop_il(core);
    core->rl[kE64RegisterR8] = emex64_pop_il(core);
    core->rl[kE64RegisterR7] = emex64_pop_il(core);
    core->rl[kE64RegisterR6] = emex64_pop_il(core);
    core->rl[kE64RegisterR5] = emex64_pop_il(core);
    core->rl[kE64RegisterR4] = emex64_pop_il(core);
    core->rl[kE64RegisterR3] = emex64_pop_il(core);
    core->rl[kE64RegisterR2] = emex64_pop_il(core);
    core->rl[kE64RegisterR1] = emex64_pop_il(core);
    core->rl[kE64RegisterR0] = emex64_pop_il(core);
    core->rl[kE64RegisterFPC] = emex64_pop_il(core);
    core->rl[kE64RegisterCF] = emex64_pop_il(core);
    core->rl[kE64RegisterFP] = emex64_pop_il(core);
    UInt64 oldsp = emex64_pop_il(core);
    core->rl[kE64RegisterPC] = emex64_pop_il(core);
    core->cr_state.crel.level = emex64_pop_il(core);
    core->op.ilen = 0;

    core->rl[kE64RegisterSP] = oldsp;

    core->machine->intc->current_irq = -1;
    core->in_interrupt = false;
    core->halted = false;
    emex64_core_context_changed(core);
}

void emex64_op_bl(__E64Core core)
{
    /* pushing all relevant registers onto stack */
    emex64_push_il(core, core->rl[kE64RegisterPC] + core->op.ilen);
    emex64_push_il(core, core->rl[kE64RegisterFP]);

    /* setting current frame pointer to stack pointer to point to stack frame */
    core->rl[kE64RegisterFP] = core->rl[kE64RegisterSP];

    /* initiating jump */
    core->op.ilen = 0;
    core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[0]), core->op.param_coding[0]);
}

void emex64_op_ret(__E64Core core)
{
    core->rl[kE64RegisterSP] = core->rl[kE64RegisterFP];
    core->rl[kE64RegisterFP] = emex64_pop_il(core);
    core->rl[kE64RegisterPC] = emex64_pop_il(core);
    core->op.ilen = 0;
}

void emex64_op_bbz(__E64Core core)
{
    if((*(core->op.param[0]) & *(core->op.param[1])) == 0)
    {
        core->op.ilen = 0;
        core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[2]), core->op.param_coding[2]);
    }
}

void emex64_op_bbnz(__E64Core core)
{
    if((*(core->op.param[0]) & *(core->op.param[1])) != 0)
    {
        core->op.ilen = 0;
        core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[2]), core->op.param_coding[2]);
    }
}

static inline UInt64 emex64_rotl(UInt64 x,
                                 unsigned int b)
{
    return (x << b) | (x >> (64 - b));
}

#define EMEX64_SIPROUND                                                         \
    do {                                                                        \
        v0 += v1; v1 = emex64_rotl(v1, 13); v1 ^= v0; v0 = emex64_rotl(v0, 32); \
        v2 += v3; v3 = emex64_rotl(v3, 16); v3 ^= v2;                           \
        v0 += v3; v3 = emex64_rotl(v3, 21); v3 ^= v0;                           \
        v2 += v1; v1 = emex64_rotl(v1, 17); v1 ^= v2; v2 = emex64_rotl(v2, 32); \
    } while(0)

static inline UInt64 emex64_pac_siphash(UInt64 key,
                                        UInt64 ptr,
                                        UInt64 modifier)
{
    UInt64 k0 = key;
    UInt64 k1 = key ^ 0x9E3779B97F4A7C15ULL;
    UInt64 v0 = k0 ^ 0x736F6D6570736575ULL;
    UInt64 v1 = k1 ^ 0x646F72616E646F6DULL;
    UInt64 v2 = k0 ^ 0x6C7967656E657261ULL;
    UInt64 v3 = k1 ^ 0x7465646279746573ULL;
    UInt64 m[3] = { ptr & ~E64_PAC_MASK, modifier, 16ULL << 56 };

    for(int i = 0; i < 3; i++)
    {
        v3 ^= m[i];
        EMEX64_SIPROUND;
        v0 ^= m[i];
    }
    v2 ^= 0xFF;
    EMEX64_SIPROUND;
    EMEX64_SIPROUND;
    EMEX64_SIPROUND;
    return (v0 ^ v1 ^ v2 ^ v3) & E64_PAC_MASK;
}

static inline UInt64 emex64_pac_signature(__E64Core core,
                                          UInt64 key,
                                          UInt64 ptr,
                                          UInt64 modifier)
{
    ptr &= ~E64_PAC_MASK;
    emex64_pac_cache_entry_t *e = &core->pac_cache[emex64_pac_cache_index(ptr, modifier)];
    if(likely(e->ptr == ptr && e->modifier == modifier && e->key == key))
    {
        return e->signature;
    }
    UInt64 signature = emex64_pac_siphash(key, ptr, modifier);
    e->key = key;
    e->ptr = ptr;
    e->modifier = modifier;
    e->signature = signature;
    return signature;
}

static inline Boolean emex64_pac_enabled(__E64Core core)
{
    return (core->cr_state.crpac & (core->cr_state.crel.level == kE64ElevationLevelUser ? E64_CRPAC_USER : E64_CRPAC_KERNEL)) != 0;
}

static inline UInt64 emex64_pac_sign(__E64Core core,
                                     UInt64 key,
                                     UInt64 ptr,
                                     UInt64 modifier)
{
    ptr &= ~E64_PAC_MASK;
    return ptr | emex64_pac_signature(core, key, ptr, modifier);
}

static Boolean emex64_pac_auth(__E64Core core,
                               UInt64 key,
                               UInt64 ptr,
                               UInt64 modifier,
                               UInt64 *out)
{
    if((ptr & E64_PAC_MASK) == emex64_pac_signature(core, key, ptr, modifier))
    {
        *out = ptr & ~E64_PAC_MASK;
        return true;
    }
    if(core->cr_state.crexc.exception == kE64ExceptionNone)
    {
        core->cr_state.crexc.exception = kE64ExceptionPAC;
    }
    if(!core->trap.fault_valid)
    {
        core->trap.fault_valid = true;
        core->trap.fault_addr = ptr;
        core->trap.fault_access = 0;
    }
    return false;
}

static inline UInt64 emex64_pac_modifier(__E64Core core)
{
    return core->op.param_cnt > 1 ? *core->op.param[1] : 0;
}

void emex64_op_pacia(__E64Core core)
{
    if(emex64_pac_enabled(core))
    {
        *core->op.param[0] = emex64_pac_sign(core, core->cr_state.criakey, *core->op.param[0], emex64_pac_modifier(core));
    }
}

void emex64_op_autia(__E64Core core)
{
    UInt64 ptr;
    if(emex64_pac_enabled(core) &&
       emex64_pac_auth(core, core->cr_state.criakey, *core->op.param[0], emex64_pac_modifier(core), &ptr))
    {
        *core->op.param[0] = ptr;
    }
}

void emex64_op_pacda(__E64Core core)
{
    if(emex64_pac_enabled(core))
    {
        *core->op.param[0] = emex64_pac_sign(core, core->cr_state.crdakey, *core->op.param[0], emex64_pac_modifier(core));
    }
}

void emex64_op_autda(__E64Core core)
{
    UInt64 ptr;
    if(emex64_pac_enabled(core) &&
       emex64_pac_auth(core, core->cr_state.crdakey, *core->op.param[0], emex64_pac_modifier(core), &ptr))
    {
        *core->op.param[0] = ptr;
    }
}

void emex64_op_xpac(__E64Core core)
{
    *core->op.param[0] = *core->op.param[core->op.param_cnt - 1] & ~E64_PAC_MASK;
}

void emex64_op_pacret(__E64Core core)
{
    if(!emex64_pac_enabled(core))
    {
        return;
    }
    UInt64 frame = core->rl[kE64RegisterFP];
    UInt64 ret = 0;
    E64MemoryCoreAction(core->machine->memory, core, frame + 16, sizeof(UInt64), &ret, kE64MemoryActionTypeRead);
    if(core->cr_state.crexc.exception != kE64ExceptionNone)
    {
        return;
    }
    ret = emex64_pac_sign(core, core->cr_state.criakey, ret, frame);
    E64MemoryCoreAction(core->machine->memory, core, frame + 16, sizeof(UInt64), &ret, kE64MemoryActionTypeWrite);
}

void emex64_op_reta(__E64Core core)
{
    UInt64 frame = core->rl[kE64RegisterFP];
    core->rl[kE64RegisterSP] = frame;
    core->rl[kE64RegisterFP] = emex64_pop_il(core);
    UInt64 ret = emex64_pop_il(core);
    if(emex64_pac_enabled(core) &&
       !emex64_pac_auth(core, core->cr_state.criakey, ret, frame, &ret))
    {
        return;
    }
    core->rl[kE64RegisterPC] = ret;
    core->op.ilen = 0;
}

void emex64_op_cblt(__E64Core core)
{
    if((SInt64)*(core->op.param[0]) < (SInt64)*(core->op.param[1]))
    {
        core->op.ilen = 0;
        core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[2]), core->op.param_coding[2]);
    }
}

static inline void emex64_branch_if(__E64Core core,
                                    Boolean taken)
{
    if(taken)
    {
        core->op.ilen = 0;
        core->rl[kE64RegisterPC] = emex64_branch_pc(core->rl[kE64RegisterPC], *(core->op.param[2]), core->op.param_coding[2]);
    }
}

void emex64_op_cbeq(__E64Core core)
{
    emex64_branch_if(core, *(core->op.param[0]) == *(core->op.param[1]));
}

void emex64_op_cbne(__E64Core core)
{
    emex64_branch_if(core, *(core->op.param[0]) != *(core->op.param[1]));
}

void emex64_op_cbge(__E64Core core)
{
    emex64_branch_if(core, (SInt64)*(core->op.param[0]) >= (SInt64)*(core->op.param[1]));
}

void emex64_op_cbltu(__E64Core core)
{
    emex64_branch_if(core, *(core->op.param[0]) < *(core->op.param[1]));
}

void emex64_op_cbgeu(__E64Core core)
{
    emex64_branch_if(core, *(core->op.param[0]) >= *(core->op.param[1]));
}

void emex64_op_wfe(__E64Core core)
{
    if(__atomic_exchange_n(&core->event, false, __ATOMIC_ACQ_REL))
    {
        return;
    }
    if(core->cr_state.crstat & E64_CRSTAT_TRAPS)
    {
        core->wfe = true;
        core->halted = true;
    }
}

void emex64_op_sev(__E64Core core)
{
    __E64Machine machine = (__E64Machine)core->machine;
    for(UInt8 i = 0; i < machine->core_count; i++)
    {
        __E64Core c = (__E64Core)machine->cores[i];
        __atomic_store_n(&c->event, true, __ATOMIC_RELEASE);
        __E64CoreKick(c);
    }
}

void emex64_op_pause(__E64Core core)
{
    /* fuck it, let's make it depend on host OS*/
#if defined(__aarch64__)
    __asm__ volatile("yield");
#elif defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#endif
}

void emex64_op_tlbi(__E64Core core)
{
    if(core->cr_state.crel.level < kE64ElevationLevelKernel)
    {
        core->cr_state.crexc.exception = kE64ExceptionPermission;
        return;
    }

    emex64_core_tlb_changed(core);
    if(core->op.param_cnt == 0)
    {
        __atomic_fetch_add(&core->snoop->tlb_epoch, 1, __ATOMIC_ACQ_REL);
        __atomic_fetch_add(&core->snoop->map_seq, 1, __ATOMIC_ACQ_REL);
        return;
    }

    UInt64 vpn = *(core->op.param[0]) >> 13;
    Boolean any_asid = core->op.param_cnt < 2;
    UInt16 asid = any_asid ? 0 : (UInt16)*(core->op.param[1]);
    emex64_tlb_t *tlb = &core->tlb;
    for(UInt64 k = 0; k < EMEX64_TLB_ENTRIES; k++)
    {
        emex64_tlb_entry_t *e = &tlb->entries[k];
        if(e->valid && e->vpn == vpn && (any_asid || e->asid == asid))
        {
            e->valid = false;
        }
    }
}

Boolean E64EnterInterruptPrivate(__E64IC ic, __E64Core core, EFIndex irq, UInt64 return_pc);

void emex64_op_svc(__E64Core core)
{
    if(core->cr_state.crstat & E64_CRSTAT_TRAPS)
    {
        core->trap.svc_pending = true;
        return;
    }

    __E64IC ic = (__E64IC)core->machine->intc;

    UInt64 return_pc = core->rl[kE64RegisterPC] + core->op.ilen;
    E64EnterInterruptPrivate(ic, core, EMEX64_IRQ_SOFTWARE, return_pc);

    core->op.ilen = 0;
}

void emex64_op_eret(__E64Core core)
{
    if(!(core->cr_state.crstat & E64_CRSTAT_TRAPS))
    {
        core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
        return;
    }
    if(core->cr_state.crel.level < kE64ElevationLevelKernel)
    {
        core->cr_state.crexc.exception = kE64ExceptionPermission;
        return;
    }

    UInt64 st = core->cr_state.crstat;
    E64ElevationLevel level = (E64ElevationLevel)((st & E64_CRSTAT_PEL_MASK) >> E64_CRSTAT_PEL_SHIFT);

    st &= ~(E64_CRSTAT_IE | E64_CRSTAT_PEL_MASK);
    if(st & E64_CRSTAT_PIE)
    {
        st |= E64_CRSTAT_IE;
    }
    st |= E64_CRSTAT_PIE;
    core->cr_state.crstat = st;

    core->cr_state.crel.level = level;
    core->rl[kE64RegisterPC] = core->cr_state.crepc;
    core->op.ilen = 0;

    emex64_core_context_changed(core);
}
