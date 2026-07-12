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

#ifndef E64IC_H
#define E64IC_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/base.h>
#include <EmexToolchain/VM/E64MMIOBus.h>
#ifdef ET_PRIVATE
#include <EmexToolchain/VM/device/internal/controller/__E64IC.h>
#endif /* ET_PRIVATE */

#define EMEX64_INTC_SIZE      0x30

/* internal devices */
#define EMEX64_IRQ_EXCEPTION  0
#define EMEX64_IRQ_TIMER      1
#define EMEX64_IRQ_DISK       2
#define EMEX64_IRQ_NETWORK    3
#define EMEX64_IRQ_SOFTWARE   4

/* board devices */
#define EMEX64_IRQ_UART       5
#define EMEX64_IRQ_8042       6   /* emex8042 MMIO chip fires interrupt when device gets plugged in for example */

#define EMEX64_IRQ_MAX        63

#define EMEX64_INTC_REG_PENDING   0x00
#define EMEX64_INTC_REG_ENABLED   0x08
#define EMEX64_INTC_REG_CTRL      0x10
#define EMEX64_INTC_REG_VECTOR    0x18
#define EMEX64_INTC_REG_ACK       0x20
#define EMEX64_INTC_REG_CURRENT   0x28

/* control register bits */
#define EMEX64_INTC_CTRL_ENABLE   (1 << 0)

typedef struct __E64Core *E64CoreRef;
typedef struct __E64Machine *E64MachineRef;
typedef struct __E64IC *E64ICRef;

EFTypeID E64ICGetTypeID(void);

E64ICRef E64ICCreate(EFAllocatorRef allocatorRef);

E64MMIORegionRef E64ICCopyMMIORegion(EFAllocatorRef allocatorRef, E64ICRef icRef);

Boolean E64ICRegisterOnMMIOBus(E64ICRef icRef, E64MMIOBusRef MMIOBusRef);

void E64ICRaiseInterrupt(E64ICRef icRef, EFIndex irqLine);
void E64ICClearInterrupt(E64ICRef icRef, EFIndex irqLine);

#endif /* E64IC_H */
