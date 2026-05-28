/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <emex64vm/device/interrupt.h>
#include <emex64vm/core.h>
#include <emex64vm/machine.h>
#include <emex64vm/memory.h>
#include <emex64vm/instruction/ctrl.h>

la64_intc_t *la64_intc_alloc(la64_machine_t *machine)
{
    /* allocate interrupt controller */
    la64_intc_t *intc = malloc(sizeof(la64_intc_t));

    /* null pointer check */
    if(intc == NULL)
    {
        return NULL;
    }

    /* register interrupt controller MMIO */
    if(!la64_mmio_register(machine->mmio_bus, LA64_INTC_BASE, LA64_INTC_SIZE, intc, la64_intc_read, la64_intc_write))
    {
        free(intc);
        return NULL;
    }

    /* setup interrupt controller */
    intc->current_irq = -1;
    intc->enabled = 0;
    intc->ctrl = 0;
    intc->vector_base = 0;

    return intc;
}

void la64_intc_dealloc(la64_intc_t *intc)
{
    free(intc);
}

void la64_raise_interrupt(la64_machine_t *machine,
                          int irq_line)
{
    /* sanity checks */
    if(irq_line < 0 ||
       irq_line > LA64_IRQ_MAX)
    {
        return;
    }
    
    /* setting pending bit for intc */
    machine->intc->pending |= (1ULL << irq_line);
}

void la64_clear_interrupt(la64_machine_t *machine,
                          int irq_line)
{
    /* sanity checks */
    if(irq_line < 0 ||
       irq_line > LA64_IRQ_MAX)
    {
        return;
    }
    
    /* clear pending bit */
    machine->intc->pending &= ~(1ULL << irq_line);
}

static int find_pending_irq(la64_intc_t *intc)
{
    /* get pending and enabled interrupts */
    uint64_t active = intc->pending & intc->enabled;

    /* checking if interrupts are enabled */
    if(active == 0)
    {
        return -1;
    }
    
    /* iterating for lowest set bit */
    for(int i = 0; i <= LA64_IRQ_MAX; i++)
    {
        if(active & (1ULL << i))
        {
            return i;
        }
    }
    
    return -1;
}

bool la64_serve_interrupt_if_needed(la64_core_t *core)
{    
    /* check if interrupts are globally enabled */
    if(!(core->machine->intc->ctrl & LA64_INTC_CTRL_ENABLE))
    {
        return false;
    }
    
    /* check if were already servicing an interrupt (unless nesting allowed) */
    if(core->machine->intc->current_irq >= 0 &&
       !(core->machine->intc->ctrl & LA64_INTC_CTRL_NESTING))
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

    uint64_t vector_addr = core->machine->intc->vector_base + (irq * 8);
    
    /* read handler address from vector table */
    void *vector_ptr = la64_memory_access(core, vector_addr, 8);
    if(vector_ptr == NULL)
    {
        core->machine->intc->current_irq = -1;
        return false;
    }

    uint64_t handler_addr = *(uint64_t *)vector_ptr;

    /* jump to handler */
    uint64_t oldsp = core->rl[LA64_REGISTER_SP];
    uint64_t oldel = core->rl[LA64_REGISTER_CR0];

    core->rl[LA64_REGISTER_CR0] = LA64_ELEVATION_KERNEL;
    core->rl[LA64_REGISTER_SP] = core->rl[LA64_REGISTER_CR1];

    /* creating interrupt stack frame */
    la64_push(core, oldel);
    la64_push(core, core->rl[LA64_REGISTER_PC]);
    la64_push(core, oldsp);
    la64_push(core, core->rl[LA64_REGISTER_FP]);
    la64_push(core, core->rl[LA64_REGISTER_CF]);
    la64_push(core, core->rl[LA64_REGISTER_R0]);
    la64_push(core, core->rl[LA64_REGISTER_R1]);
    la64_push(core, core->rl[LA64_REGISTER_R2]);
    la64_push(core, core->rl[LA64_REGISTER_R3]);
    la64_push(core, core->rl[LA64_REGISTER_R4]);
    la64_push(core, core->rl[LA64_REGISTER_R5]);
    la64_push(core, core->rl[LA64_REGISTER_R6]);
    la64_push(core, core->rl[LA64_REGISTER_R7]);
    la64_push(core, core->rl[LA64_REGISTER_R8]);
    la64_push(core, core->rl[LA64_REGISTER_R9]);
    la64_push(core, core->rl[LA64_REGISTER_R10]);
    la64_push(core, core->rl[LA64_REGISTER_R11]);

    /* storing it as frame pointer  */
    core->rl[LA64_REGISTER_FP] = core->rl[LA64_REGISTER_SP];

    /* performing jump */
    core->rl[LA64_REGISTER_PC] = handler_addr;
    core->op.ilen = 0;
    core->in_interrupt = true;

    /* checking if core is at halt */
    if(!core->halted)
    {
        core->unhalted_interrupt = true;
    }
    
    return true;
}

uint64_t la64_intc_read(la64_core_t *core, void *device, uint64_t offset, int size)
{
    la64_intc_t *intc = (la64_intc_t *)device;

    switch(offset)
    {
        case LA64_INTC_REG_PENDING:
            return intc->pending; 
        case LA64_INTC_REG_ENABLED:
            return intc->enabled;
        case LA64_INTC_REG_CTRL:
            return intc->ctrl;
        case LA64_INTC_REG_VECTOR:
            return intc->vector_base;
        case LA64_INTC_REG_CURRENT:
            return (uint64_t)intc->current_irq;
        default:
            return 0;
    }
}

void la64_intc_write(la64_core_t *core, void *device, uint64_t offset, uint64_t value, int size)
{
    la64_intc_t *intc = (la64_intc_t *)device;
    
    switch (offset) {
        case LA64_INTC_REG_PENDING:
            intc->pending &= ~value;
            break;
        case LA64_INTC_REG_ENABLED:
            intc->enabled = value;
            break;
        case LA64_INTC_REG_CTRL:
            intc->ctrl = value;
            break;
        case LA64_INTC_REG_VECTOR:
            intc->vector_base = value;
            break;
        case LA64_INTC_REG_ACK:
            if((int64_t)value == intc->current_irq)
            {
                intc->current_irq = -1;
            }
            break;
        default:
            break;
    }
}
