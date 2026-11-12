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

#include <pthread.h>
#include <string.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Core.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/E64Memory.h>
#include <EmexToolchain/VM/instruction/ctrl.h>
#include <EmexToolchain/VM/device/internal/controller/E64IC.h>

static UInt64 __E64ICMMIORead(E64CoreRef core,
                              void *device,
                              UInt64 offset,
                              EFSize size)
{
    __E64IC ic = (__E64IC)device;

    switch(offset)
    {
        case EMEX64_INTC_REG_PENDING:
            return __atomic_load_n(&ic->pending, __ATOMIC_RELAXED);
        case EMEX64_INTC_REG_ENABLED:
            return ic->enabled;
        case EMEX64_INTC_REG_CTRL:
            return ic->ctrl;
        case EMEX64_INTC_REG_VECTOR:
            return ic->vector_base;
        case EMEX64_INTC_REG_CURRENT:
            return (UInt64)ic->current_irq;
        default:
            return 0;
    }
}

static void __E64ICMMIOWrite(E64CoreRef core,
                             void *device,
                             UInt64 offset,
                             UInt64 value,
                             EFSize size)
{
    __E64IC ic = (__E64IC)device;

    switch(offset)
    {
        case EMEX64_INTC_REG_PENDING:
            __atomic_fetch_and(&ic->pending, ~value, __ATOMIC_ACQ_REL);
            break;
        case EMEX64_INTC_REG_ENABLED:
            ic->enabled = value;
            break;
        case EMEX64_INTC_REG_CTRL:
            ic->ctrl = value;
            break;
        case EMEX64_INTC_REG_VECTOR:
            ic->vector_base = value;
            break;
        case EMEX64_INTC_REG_ACK:
            if((SInt64)value == ic->current_irq)
            {
                ic->current_irq = -1;
            }
            break;
        default:
            break;
    }
}

static EFIndex __E64ICFindPendingInterrupt(__E64IC ic)
{
    UInt64 active = __atomic_load_n(&ic->pending, __ATOMIC_ACQUIRE) & ic->enabled & ic->core_lines[0];
    if(active == 0)
    {
        return -1;
    }
    return __builtin_ctzll(active);
}

static UInt64 __E64IOAPICMMIORead(E64CoreRef core,
                                  void *device,
                                  UInt64 offset,
                                  EFSize size)
{
    __E64IC ic = (__E64IC)device;
    if(offset == EMEX64_IOAPIC_REG_VERSION)
    {
        return 1 | (UInt64)(EMEX64_IRQ_MAX + 1) << 8;
    }
    if(offset >= EMEX64_IOAPIC_REG_ROUTE && offset < EMEX64_IOAPIC_REG_ROUTE + 8 * (EMEX64_IRQ_MAX + 1))
    {
        return ic->route[(offset - EMEX64_IOAPIC_REG_ROUTE) / 8];
    }
    return 0;
}

static void __E64IOAPICMMIOWrite(E64CoreRef core,
                                 void *device,
                                 UInt64 offset,
                                 UInt64 value,
                                 EFSize size)
{
    __E64IC ic = (__E64IC)device;
    if(offset < EMEX64_IOAPIC_REG_ROUTE || offset >= EMEX64_IOAPIC_REG_ROUTE + 8 * (EMEX64_IRQ_MAX + 1))
    {
        return;
    }
    UInt64 line = (offset - EMEX64_IOAPIC_REG_ROUTE) / 8;
    UInt8 to = (UInt8)(value & 0xFF);
    if(to >= E64IC_MAX_CORES)
    {
        return;
    }
    UInt8 from = ic->route[line];
    ic->route[line] = to;
    __atomic_fetch_and(&ic->core_lines[from], ~(1ULL << line), __ATOMIC_ACQ_REL);
    __atomic_fetch_or(&ic->core_lines[to], 1ULL << line, __ATOMIC_ACQ_REL);
}

static EFClassDefinitionV4 E64ICClass = {
    .header = {
        .version = 4,
        .typeID = kEFTypeIDNone,
        .name = EFSTR_FILESCOPE("E64IC"),
    },
    .init = NULL,
    .deinit = NULL,
    .equal = NULL,
    .hash = NULL,
    .copyDescription = NULL,
    .copyDebugDescription = NULL,
};

static void E64ICRegisterClass(void)
{
    EFClassRegister(&E64ICClass);
}

EFTypeID E64ICGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, E64ICRegisterClass);
    return E64ICClass.header.typeID;
}

E64ICRef E64ICCreate(EFAllocatorRef allocatorRef)
{
    __E64IC ic = (__E64IC)EFObjectCreate(allocatorRef, E64ICGetTypeID(), (EFIndex)sizeof(struct __E64IC));
    if(ic == NULL)
    {
        return NULL;
    }

    ic->current_irq = -1;
    ic->enabled = 0;
    ic->ctrl = 0;
    ic->vector_base = 0;
    ic->wake = NULL;
    ic->wake_context = NULL;

    memset(ic->route, 0, sizeof(ic->route));
    memset(ic->core_lines, 0, sizeof(ic->core_lines));
    ic->core_lines[0] = ~0ULL;

    return (E64ICRef)ic;
}

void __E64ICReset(E64ICRef icRef)
{
    __E64IC ic = (__E64IC)icRef;
    if(ic == NULL)
    {
        return;
    }
    __atomic_store_n(&ic->pending, 0, __ATOMIC_RELEASE);
    ic->current_irq = -1;
    ic->enabled = 0;
    ic->ctrl = 0;
    ic->vector_base = 0;
    memset(ic->route, 0, sizeof(ic->route));
    memset(ic->core_lines, 0, sizeof(ic->core_lines));
    ic->core_lines[0] = ~0ULL;
}

E64MMIORegionRef E64ICCopyMMIORegion(EFAllocatorRef allocatorRef,
                                     E64ICRef icRef)
{
    if(icRef == NULL)
    {
        return NULL;
    }

    if(allocatorRef == NULL)
    {
        allocatorRef = EFGetAllocator(icRef);
    }

    return E64MMIORegionCreate(allocatorRef, EMEX64_IC_BASE, EMEX64_INTC_SIZE, icRef, __E64ICMMIORead, __E64ICMMIOWrite);
}

Boolean E64ICRegisterOnMMIOBus(E64ICRef icRef,
                               E64MMIOBusRef MMIOBusRef)
{
    if(icRef == NULL || MMIOBusRef == NULL)
    {
        return false;
    }

    E64MMIORegionRef MMIORegionRef = E64ICCopyMMIORegion(NULL, icRef);
    if(MMIORegionRef == NULL)
    {
        return false;
    }

    Boolean success = E64MMIOBusRegisterRegion(MMIOBusRef, MMIORegionRef);
    EFRelease(MMIORegionRef);
    if(!success)
    {
        return false;
    }

    E64MMIORegionRef IOAPICRegionRef = E64MMIORegionCreate(EFGetAllocator(icRef), EMEX64_IOAPIC_BASE, EMEX64_IOAPIC_SIZE, icRef, __E64IOAPICMMIORead, __E64IOAPICMMIOWrite);
    if(IOAPICRegionRef == NULL)
    {
        return false;
    }
    success = E64MMIOBusRegisterRegion(MMIOBusRef, IOAPICRegionRef);
    EFRelease(IOAPICRegionRef);
    return success;
}

void E64ICRaiseInterrupt(E64ICRef icRef,
                         EFIndex irqLine)
{
    __E64IC ic = (__E64IC)icRef;
    if(ic == NULL || irqLine < 0 || irqLine > EMEX64_IRQ_MAX)
    {
        return;
    }

    __atomic_fetch_or(&ic->pending, 1ULL << irqLine, __ATOMIC_SEQ_CST);

    if(ic->wake != NULL)
    {
        ic->wake(ic->wake_context, ic->route[irqLine]);
    }
}

void E64ICSetWakeHandler(E64ICRef icRef,
                         void (*wake)(void *context, UInt8 core),
                         void *context)
{
    __E64IC ic = (__E64IC)icRef;
    if(ic == NULL)
    {
        return;
    }
    ic->wake_context = context;
    ic->wake = wake;
}

void E64ICClearInterrupt(E64ICRef icRef,
                         EFIndex irqLine)
{
    __E64IC ic = (__E64IC)icRef;
    if(ic == NULL || irqLine < 0 || irqLine > EMEX64_IRQ_MAX)
    {
        return;
    }

    __atomic_fetch_and(&ic->pending, ~(1ULL << irqLine), __ATOMIC_ACQ_REL);
}

Boolean E64ICHasPendingInterrupt(E64ICRef icRef,
                                 UInt8 core)
{
    __E64IC ic = (__E64IC)icRef;
    return ic != NULL && (ic->ctrl & EMEX64_INTC_CTRL_ENABLE) &&
           (__atomic_load_n(&ic->pending, __ATOMIC_RELAXED) & ic->enabled & __atomic_load_n(&ic->core_lines[core], __ATOMIC_RELAXED)) != 0;
}

EFIndex E64ICTakeInterrupt(E64ICRef icRef,
                           UInt8 core)
{
    __E64IC ic = (__E64IC)icRef;
    if(ic == NULL || !(ic->ctrl & EMEX64_INTC_CTRL_ENABLE))
    {
        return -1;
    }

    UInt64 mine = ic->enabled & __atomic_load_n(&ic->core_lines[core], __ATOMIC_RELAXED);
    for(;;)
    {
        UInt64 pending = __atomic_load_n(&ic->pending, __ATOMIC_ACQUIRE);
        UInt64 active = pending & mine;
        if(active == 0)
        {
            return -1;
        }
        EFIndex irq = __builtin_ctzll(active);
        UInt64 bit = 1ULL << irq;
        if(__atomic_fetch_and(&ic->pending, ~bit, __ATOMIC_ACQ_REL) & bit)
        {
            ic->current_irq = irq;
            return irq;
        }
    }
}

Boolean __E64ServeInterruptIfNeeded(E64ICRef icRef,
                                    E64CoreRef coreRef)
{
    __E64IC ic = (__E64IC)icRef;
    __E64Core core = (__E64Core)coreRef;
    if(ic == NULL || core == NULL || !(ic->ctrl & EMEX64_INTC_CTRL_ENABLE))
    {
        return false;
    }

    __E64Machine machine = (__E64Machine)core->machine;
    if(machine == NULL || machine->memory == NULL)
    {
        return false;
    }

    if(core->in_interrupt || core->op.opcode == kE64OpcodeIRET)
    {
        return false;
    }

    /* find highest priority pending interrupt */
    EFIndex irq = __E64ICFindPendingInterrupt(ic);
    if(irq < 0)
    {
        return false;
    }

    /* mark which IRQ were servicing */
    ic->current_irq = irq;

    __atomic_fetch_and(&ic->pending, ~(1ULL << irq), __ATOMIC_ACQ_REL);

    /* read handler address from vector table */
    UInt64 vector_addr = ic->vector_base + (irq * 8);
    UInt64 handler_addr;
    if(!E64MemoryAction(machine->memory, vector_addr, 8, &handler_addr, kE64MemoryActionTypeRead))
    {
        core->machine->intc->current_irq = -1;
        return false;
    }
    handler_addr = TO_HOST64(handler_addr);

    /* jump to handler */
    UInt64 oldsp = core->rl[kE64RegisterSP];
    UInt64 oldel = core->cr_state.crel.level;

    /*
     * must be kernel, because the IC is internal
     * inside of the emex64 SoC and it will also
     * handle syscalls for efficiency reasons, that
     * decision ma change tho in the future. But it
     * would be a vulnerability to elevate to
     * SecureMonitor as that is only accessible at
     * boot time.
     */
    core->cr_state.crel.level = kE64ElevationLevelKernel;   /* set the elevation level to kernel to prevent kernel level escape */
    core->rl[kE64RegisterSP] = core->cr_state.crksp.address;

    /* creating interrupt stack frame */
    emex64_push_il(core, oldel);                        /* fp + 128 */
    emex64_push_il(core, core->rl[kE64RegisterPC]);     /* fp + 120 */
    emex64_push_il(core, oldsp);                        /* fp + 112 */
    emex64_push_il(core, core->rl[kE64RegisterFP]);     /* fp + 104 */
    emex64_push_il(core, core->rl[kE64RegisterCF]);     /* fp + 96 */
    emex64_push_il(core, core->rl[kE64RegisterFPC]);    /* fp + 88 */
    emex64_push_il(core, core->rl[kE64RegisterR0]);     /* fp + 80 */
    emex64_push_il(core, core->rl[kE64RegisterR1]);     /* fp + 72 */
    emex64_push_il(core, core->rl[kE64RegisterR2]);     /* fp + 64 */
    emex64_push_il(core, core->rl[kE64RegisterR3]);     /* fp + 56 */
    emex64_push_il(core, core->rl[kE64RegisterR4]);     /* fp + 48 */
    emex64_push_il(core, core->rl[kE64RegisterR5]);     /* fp + 40 */
    emex64_push_il(core, core->rl[kE64RegisterR6]);     /* fp + 32 */
    emex64_push_il(core, core->rl[kE64RegisterR7]);     /* fp + 24 */
    emex64_push_il(core, core->rl[kE64RegisterR8]);     /* fp + 16 */
    emex64_push_il(core, core->rl[kE64RegisterR9]);     /* fp + 8 */

    /* storing it as frame pointer  */
    core->rl[kE64RegisterFP] = core->rl[kE64RegisterSP];

    /* performing jump */
    core->rl[kE64RegisterPC] = handler_addr;
    core->op.ilen = 0;
    core->in_interrupt = true;
    core->halted = false;

    return true;
}

EF_HIDDEN Boolean E64EnterInterruptPrivate(__E64IC ic,
                                           __E64Core core,
                                           EFIndex irq,
                                           UInt64 return_pc)
{
    if(core->in_interrupt)
    {
        return false;
    }

    __E64Machine machine = (__E64Machine)core->machine;

    UInt64 vector_addr = ic->vector_base + (irq * sizeof(UInt64));
    UInt64 handler_addr;

    if(!E64MemoryAction(machine->memory, vector_addr, 8, &handler_addr, kE64MemoryActionTypeRead))
    {
        return false;
    }

    handler_addr = TO_HOST64(handler_addr);
    UInt64 oldsp = core->rl[kE64RegisterSP];
    UInt64 oldel = core->cr_state.crel.level;

    core->cr_state.crel.level = kE64ElevationLevelKernel;
    core->rl[kE64RegisterSP] = core->cr_state.crksp.address;

    emex64_push_il(core, oldel);
    emex64_push_il(core, return_pc);
    emex64_push_il(core, oldsp);
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

    core->rl[kE64RegisterFP] = core->rl[kE64RegisterSP];
    ic->current_irq = irq;

    core->rl[kE64RegisterPC] = handler_addr;
    core->op.ilen = 0;

    core->in_interrupt = true;
    core->halted = false;

    return true;
}
