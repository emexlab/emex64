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

#include <fw.h>
#include <interrupt.h>

#define INTERRUPT_STACK_SIZE    EMEX64_PAGE_SIZE

static u64 ivec[EMEX64_IC_IRQS];
static interrupt_handler_t handlers[EMEX64_IC_IRQS];
static u8 interrupt_stack[INTERRUPT_STACK_SIZE] __attribute__((aligned(16)));

void interrupt_dispatch(struct fw_frame *frame);

__attribute__((naked)) static void interrupt_entry(void)
{
    __asm__ __volatile__(
        "push rr, er0, er1, er2, er3, er4, er5, er6, er7\n\t"
        "mov r0, fp\n\t"
        "bl interrupt_dispatch\n\t"
        "pop er7, er6, er5, er4, er3, er2, er1, er0, rr\n\t"
        "iret");
}

void interrupt_dispatch(struct fw_frame *frame)
{
    u64 irq = mmio_read64(EMEX64_IC_REG_CURRENT);
    if(irq < EMEX64_IC_IRQS && handlers[irq] != NULL)
    {
        handlers[irq](frame);
    }
}

void interrupt_init(void)
{
    /* the stack the CPU switches to for a handler */
    fw_cr_write(FW_CR_CRKSP, (u64)interrupt_stack + INTERRUPT_STACK_SIZE - 8);

    /* setting up vector table onto interrupt controller */
    mmio_write64(EMEX64_IC_REG_VECTOR, (u64)ivec);
    mmio_write64(EMEX64_IC_REG_ENABLED, 0);
    mmio_write64(EMEX64_IC_REG_CTRL, EMEX64_IC_CTRL_ENABLE);

    fw_printf("[ic] initialized\n");
}

void interrupt_register_handler(u64 irq,
                                interrupt_handler_t handler)
{
    if(irq >= EMEX64_IC_IRQS)
    {
        return;
    }

    handlers[irq] = handler;
    ivec[irq] = (u64)interrupt_entry;
    mmio_write64(EMEX64_IC_REG_ENABLED, mmio_read64(EMEX64_IC_REG_ENABLED) | (1UL << irq));
}

void interrupt_unregister_handler(u64 irq)
{
    if(irq >= EMEX64_IC_IRQS)
    {
        return;
    }

    mmio_write64(EMEX64_IC_REG_ENABLED, mmio_read64(EMEX64_IC_REG_ENABLED) & ~(1UL << irq));
    ivec[irq] = 0;
    handlers[irq] = NULL;
}

void interrupt_unregister_all(void)
{
    for(u64 irq = 0; irq < EMEX64_IC_IRQS; irq++)
    {
        interrupt_unregister_handler(irq);
    }
}
