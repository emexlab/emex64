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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <EmexToolchain/support/likely.h>
#include <EmexToolchain/vm/core.h>
#include <EmexToolchain/vm/E64Machine.h>
#include <EmexToolchain/vm/E64Memory.h>
#include <EmexToolchain/vm/instruction/ctrl.h>
#include <EmexToolchain/vm/device/internal/controller/ic.h>

emex64_intc_t *emex64_intc_alloc(E64MachineRef machineRef)
{
    emex64_intc_t *intc = malloc(sizeof(emex64_intc_t));
    if(intc == NULL)
    {
        return NULL;
    }

    E64MMIORegionRef ICRegion = E64MMIORegionCreate(kEFAllocatorDefault, EMEX64_IC_BASE, EMEX64_INTC_SIZE, intc, emex64_intc_read, emex64_intc_write);
    if(ICRegion == NULL)
    {
        free(intc);
        return NULL;
    }

    bool success = E64MMIOBusRegisterRegion(machineRef->mmio_bus, ICRegion);
    EFRelease(ICRegion);
    if(!success)
    {
        free(intc);
        return NULL;
    }

    intc->current_irq = -1;
    intc->enabled = 0;
    intc->ctrl = 0;
    intc->vector_base = 0;

    return intc;
}

void emex64_intc_dealloc(emex64_intc_t *intc)
{
    free(intc);
}

void emex64_raise_interrupt(E64MachineRef machineRef,
                            int irq_line)
{
    if(irq_line < 0 || irq_line > EMEX64_IRQ_MAX)
    {
        return;
    }
    
    machineRef->intc->pending |= (1ULL << irq_line);
}

void emex64_clear_interrupt(E64MachineRef machineRef,
                            int irq_line)
{
    if(irq_line < 0 || irq_line > EMEX64_IRQ_MAX)
    {
        return;
    }
    
    machineRef->intc->pending &= ~(1ULL << irq_line);
}

static int find_pending_irq(emex64_intc_t *intc)
{
    uint64_t active = intc->pending & intc->enabled;
    if(active == 0)
    {
        return -1;
    }
    
    for(int i = 0; i <= EMEX64_IRQ_MAX; i++)
    {
        if(active & (1ULL << i))
        {
            return i;
        }
    }
    
    return -1;
}

bool emex64_serve_interrupt_if_needed(emex64_core_t *core)
{    
    /* check if interrupts are globally enabled */
    if(!(core->machine->intc->ctrl & EMEX64_INTC_CTRL_ENABLE))
    {
        return false;
    }
    
    /* check if were already servicing an interrupt (unless nesting allowed) */
    if(core->in_interrupt || core->op.opcode == kE64OpcodeIRET)
    {
        return false;
    }
    
    /* find highest priority pending interrupt */
    int irq = find_pending_irq(core->machine->intc);
    if(irq < 0)
    {
        return false;
    }
    
    /* mark which IRQ were servicing */
    core->machine->intc->current_irq = irq;
    
    /* clear pending bit (edge-triggered style) */
    core->machine->intc->pending &= ~(1ULL << irq);

    /* read handler address from vector table */
    uint64_t vector_addr = core->machine->intc->vector_base + (irq * 8);
    uint64_t handler_addr;
    if(!E64MemoryAction(core->machine->memory, vector_addr, 8, &handler_addr, kE64MemoryActionRead))
    {
        core->machine->intc->current_irq = -1;
        return false;
    }
    handler_addr = TO_HOST64(handler_addr);

    /* jump to handler */
    uint64_t oldsp = core->rl[kE64RegisterSP];
    uint64_t oldel = core->cr_state.crel.level;

    /*
     * must be kernel, because the IC is internal
     * inside of the emex64 SoC and it will also
     * handle syscalls for efficiency reasons, that
     * decision ma change tho in the future. But it
     * would be a vulnerability to elevate to
     * SecureMonitor as that is only accessible at
     * boot time.
     * 
     * todo: overhaul the register access system to
     *       distinct between read vs write access.
     *       my beautiful decoder is doomed.
     */
    core->cr_state.crel.level = kE64ElevationLevelKernel;
    core->rl[kE64RegisterSP] = core->cr_state.crksp.address;

    /* creating interrupt stack frame */
    emex64_push_il(core, oldel);
    emex64_push_il(core, core->rl[kE64RegisterPC]);
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
    emex64_push_il(core, core->rl[kE64RegisterR10]);
    emex64_push_il(core, core->rl[kE64RegisterR11]);
    emex64_push_il(core, core->rl[kE64RegisterR12]);
    emex64_push_il(core, core->rl[kE64RegisterR13]);
    emex64_push_il(core, core->rl[kE64RegisterR14]);
    emex64_push_il(core, core->rl[kE64RegisterR15]);
    emex64_push_il(core, core->rl[kE64RegisterR16]);
    emex64_push_il(core, core->rl[kE64RegisterR17]);
    emex64_push_il(core, core->rl[kE64RegisterR18]);
    emex64_push_il(core, core->rl[kE64RegisterR19]);
    emex64_push_il(core, core->rl[kE64RegisterR20]);
    emex64_push_il(core, core->rl[kE64RegisterR21]);
    emex64_push_il(core, core->rl[kE64RegisterR22]);
    emex64_push_il(core, core->rl[kE64RegisterR23]);
    emex64_push_il(core, core->rl[kE64RegisterR24]);
    emex64_push_il(core, core->rl[kE64RegisterR25]);

    /* storing it as frame pointer  */
    core->rl[kE64RegisterFP] = core->rl[kE64RegisterSP];

    /* performing jump */
    core->rl[kE64RegisterPC] = handler_addr;
    core->op.ilen = 0;
    core->in_interrupt = true;
    core->halted = false;
    
    return true;
}

uint64_t emex64_intc_read(emex64_core_t *core, void *device, uint64_t offset, int size)
{
    emex64_intc_t *intc = (emex64_intc_t *)device;

    switch(offset)
    {
        case EMEX64_INTC_REG_PENDING:
            return intc->pending; 
        case EMEX64_INTC_REG_ENABLED:
            return intc->enabled;
        case EMEX64_INTC_REG_CTRL:
            return intc->ctrl;
        case EMEX64_INTC_REG_VECTOR:
            return intc->vector_base;
        case EMEX64_INTC_REG_CURRENT:
            return (uint64_t)intc->current_irq;
        default:
            return 0;
    }
}

void emex64_intc_write(emex64_core_t *core, void *device, uint64_t offset, uint64_t value, int size)
{
    emex64_intc_t *intc = (emex64_intc_t *)device;
    
    switch (offset) {
        case EMEX64_INTC_REG_PENDING:
            intc->pending &= ~value;
            break;
        case EMEX64_INTC_REG_ENABLED:
            intc->enabled = value;
            break;
        case EMEX64_INTC_REG_CTRL:
            intc->ctrl = value;
            break;
        case EMEX64_INTC_REG_VECTOR:
            intc->vector_base = value;
            break;
        case EMEX64_INTC_REG_ACK:
            if((int64_t)value == intc->current_irq)
            {
                intc->current_irq = -1;
            }
            break;
        default:
            break;
    }
}
