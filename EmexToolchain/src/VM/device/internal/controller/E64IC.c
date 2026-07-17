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
#include <EmexToolchain/VM/E64Core.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/E64Memory.h>
#include <EmexToolchain/VM/instruction/ctrl.h>
#include <EmexToolchain/VM/device/internal/controller/E64IC.h>

static UInt64 __E64ICMMIORead(E64CoreRef core,
                              void *device,
                              UInt64 offset,
                              int size)
{
    __E64IC ic = (__E64IC)device;

    switch(offset)
    {
        case EMEX64_INTC_REG_PENDING:
            return ic->pending;
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
                             int size)
{
    __E64IC ic = (__E64IC)device;

    switch(offset)
    {
        case EMEX64_INTC_REG_PENDING:
            ic->pending &= ~value;
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
    UInt64 active = ic->pending & ic->enabled;
    if(active == 0)
    {
        return -1;
    }

    for(EFIndex i = 0; i <= EMEX64_IRQ_MAX; i++)
    {
        if(active & (1ULL << i))
        {
            return i;
        }
    }

    return -1;
}

static EFClass E64ICClass = {
    .name = "E64IC",
    .typeID = kEFNotATypeID,
    .init = NULL,
    .deinit = NULL,
    .equal = NULL,
};

static void E64ICRegisterClass(void)
{
    EFClassRegister(&E64ICClass);
}

EFTypeID E64ICGetTypeID(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, E64ICRegisterClass);
    return E64ICClass.typeID;
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

    return (E64ICRef)ic;
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

    ic->pending |= (1ULL << irqLine);
}

void E64ICClearInterrupt(E64ICRef icRef,
                         EFIndex irqLine)
{
    __E64IC ic = (__E64IC)icRef;
    if(ic == NULL || irqLine < 0 || irqLine > EMEX64_IRQ_MAX)
    {
        return;
    }

    ic->pending &= ~(1ULL << irqLine);
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

    /* check if were already servicing an interrupt (unless nesting allowed) */
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

    /* clear pending bit (edge-triggered style) */
    ic->pending &= ~(1ULL << irq);

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
    emex64_push_il(core, oldel);    /* fp + 128 */
    emex64_push_il(core, core->rl[kE64RegisterPC]); /* fp + 120 */
    emex64_push_il(core, oldsp);    /* fp + 112 */
    emex64_push_il(core, core->rl[kE64RegisterFP]); /* fp + 104 */
    emex64_push_il(core, core->rl[kE64RegisterCF]); /* fp + 96 */
    emex64_push_il(core, core->rl[kE64RegisterFPC]);    /* fp + 88 */
    emex64_push_il(core, core->rl[kE64RegisterR0]); /* fp + 80 */
    emex64_push_il(core, core->rl[kE64RegisterR1]); /* fp + 72 */
    emex64_push_il(core, core->rl[kE64RegisterR2]); /* fp + 64 */
    emex64_push_il(core, core->rl[kE64RegisterR3]); /* fp + 56 */
    emex64_push_il(core, core->rl[kE64RegisterR4]); /* fp + 48 */
    emex64_push_il(core, core->rl[kE64RegisterR5]); /* fp + 40 */
    emex64_push_il(core, core->rl[kE64RegisterR6]); /* fp + 32 */
    emex64_push_il(core, core->rl[kE64RegisterR7]); /* fp + 24 */
    emex64_push_il(core, core->rl[kE64RegisterR8]); /* fp + 16 */
    emex64_push_il(core, core->rl[kE64RegisterR9]); /* fp + 8 */

    /* storing it as frame pointer  */
    core->rl[kE64RegisterFP] = core->rl[kE64RegisterSP];

    /* performing jump */
    core->rl[kE64RegisterPC] = handler_addr;
    core->op.ilen = 0;
    core->in_interrupt = true;
    core->halted = false;

    return true;
}
