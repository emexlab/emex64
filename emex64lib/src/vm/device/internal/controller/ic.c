/*
 * MIT License
 *
 * Copyright (c) 2026 emexlab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <emex64lib/support/likely.h>

#include <emex64lib/vm/core.h>
#include <emex64lib/vm/machine.h>
#include <emex64lib/vm/memory.h>
#include <emex64lib/vm/instruction/ctrl.h>

#include <emex64lib/vm/device/internal/controller/ic.h>

emex64_intc_t *emex64_intc_alloc(emex64_machine_t *machine)
{
    emex64_intc_t *intc = malloc(sizeof(emex64_intc_t));
    if(intc == NULL)
    {
        return NULL;
    }

    if(!emex64_mmio_register(machine->mmio_bus, EMEX64_IC_BASE, EMEX64_INTC_SIZE, intc, emex64_intc_read, emex64_intc_write))
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

void emex64_raise_interrupt(emex64_machine_t *machine,
                            int irq_line)
{
    if(irq_line < 0 || irq_line > EMEX64_IRQ_MAX)
    {
        return;
    }
    
    machine->intc->pending |= (1ULL << irq_line);
}

void emex64_clear_interrupt(emex64_machine_t *machine,
                            int irq_line)
{
    if(irq_line < 0 || irq_line > EMEX64_IRQ_MAX)
    {
        return;
    }
    
    machine->intc->pending &= ~(1ULL << irq_line);
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
    if(core->in_interrupt || core->op.opcode == kEmex64OpcodeIRET)
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
    if(unlikely(!emex64_memory_access(core, vector_addr, 8)))
    {
        core->machine->intc->current_irq = -1;
        return false;
    }
    uint64_t handler_addr = *(uint64_t*)(core->machine->memory->memory + vector_addr);

    /* jump to handler */
    uint64_t oldsp = core->rl[kEmex64RegisterSP];
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
    core->cr_state.crel.level = kEmex64ElevationLevelKernel;
    core->rl[kEmex64RegisterSP] = core->cr_state.crksp.address;

    /* creating interrupt stack frame */
    emex64_push_il(core, oldel);
    emex64_push_il(core, core->rl[kEmex64RegisterPC]);
    emex64_push_il(core, oldsp);
    emex64_push_il(core, core->rl[kEmex64RegisterFP]);
    emex64_push_il(core, core->rl[kEmex64RegisterCF]);
    emex64_push_il(core, core->rl[kEmex64RegisterFPC]);
    emex64_push_il(core, core->rl[kEmex64RegisterR0]);
    emex64_push_il(core, core->rl[kEmex64RegisterR1]);
    emex64_push_il(core, core->rl[kEmex64RegisterR2]);
    emex64_push_il(core, core->rl[kEmex64RegisterR3]);
    emex64_push_il(core, core->rl[kEmex64RegisterR4]);
    emex64_push_il(core, core->rl[kEmex64RegisterR5]);
    emex64_push_il(core, core->rl[kEmex64RegisterR6]);
    emex64_push_il(core, core->rl[kEmex64RegisterR7]);
    emex64_push_il(core, core->rl[kEmex64RegisterR8]);
    emex64_push_il(core, core->rl[kEmex64RegisterR9]);
    emex64_push_il(core, core->rl[kEmex64RegisterR10]);
    emex64_push_il(core, core->rl[kEmex64RegisterR11]);
    emex64_push_il(core, core->rl[kEmex64RegisterR12]);
    emex64_push_il(core, core->rl[kEmex64RegisterR13]);
    emex64_push_il(core, core->rl[kEmex64RegisterR14]);
    emex64_push_il(core, core->rl[kEmex64RegisterR15]);
    emex64_push_il(core, core->rl[kEmex64RegisterR16]);
    emex64_push_il(core, core->rl[kEmex64RegisterR17]);
    emex64_push_il(core, core->rl[kEmex64RegisterR18]);
    emex64_push_il(core, core->rl[kEmex64RegisterR19]);
    emex64_push_il(core, core->rl[kEmex64RegisterR20]);
    emex64_push_il(core, core->rl[kEmex64RegisterR21]);
    emex64_push_il(core, core->rl[kEmex64RegisterR22]);
    emex64_push_il(core, core->rl[kEmex64RegisterR23]);
    emex64_push_il(core, core->rl[kEmex64RegisterR24]);
    emex64_push_il(core, core->rl[kEmex64RegisterR25]);

    /* storing it as frame pointer  */
    core->rl[kEmex64RegisterFP] = core->rl[kEmex64RegisterSP];

    /* performing jump */
    core->rl[kEmex64RegisterPC] = handler_addr;
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
