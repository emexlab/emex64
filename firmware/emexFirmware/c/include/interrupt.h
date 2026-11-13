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

/*
 * interrupt.h - the interrupt controller in the legacy model: a vector
 * table, a frame on crksp and iret (docs/traps.md)
 */

#ifndef FW_INTERRUPT_H
#define FW_INTERRUPT_H

#include <fw.h>

#define EMEX64_IC_REG_PENDING   (EMEX64_IC_BASE + 0x00)
#define EMEX64_IC_REG_ENABLED   (EMEX64_IC_BASE + 0x08)
#define EMEX64_IC_REG_CTRL      (EMEX64_IC_BASE + 0x10)
#define EMEX64_IC_REG_VECTOR    (EMEX64_IC_BASE + 0x18)
#define EMEX64_IC_REG_ACK       (EMEX64_IC_BASE + 0x20)
#define EMEX64_IC_REG_CURRENT   (EMEX64_IC_BASE + 0x28)

#define EMEX64_IC_CTRL_ENABLE   0x1

#define EMEX64_IC_IRQ_EXCEPTION 0
#define EMEX64_IC_IRQ_TIMER     1
#define EMEX64_IC_IRQ_DISK      2
#define EMEX64_IC_IRQ_NETWORK   3
#define EMEX64_IC_IRQ_SOFTWARE  4
#define EMEX64_IC_IRQ_UART      5
#define EMEX64_IC_IRQ_8042      6

#define EMEX64_IC_IRQS          64

/*
 * what the CPU pushed on crksp before it entered the handler (pc to r9,
 * fp points below r9) and what interrupt_entry pushed below that (rr
 * first, at fp). iret takes the CPU's part back, so a handler can change
 * where it returns to through pc.
 */
struct fw_frame {
    u64 rr;
    u64 r9, r8, r7, r6, r5, r4, r3, r2, r1, r0;
    u64 fpc, cf, fp, sp, pc, level;
};

typedef void (*interrupt_handler_t)(struct fw_frame *frame);

void interrupt_init(void);
void interrupt_register_handler(u64 irq, interrupt_handler_t handler);
void interrupt_unregister_handler(u64 irq);
void interrupt_unregister_all(void);

void exception_handler(struct fw_frame *frame);
void kbd_handler(struct fw_frame *frame);

#endif /* FW_INTERRUPT_H */
