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

#ifndef EMEX64VM_DEVICE_APIC_H
#define EMEX64VM_DEVICE_APIC_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/internal/vtimer.h>

#define EMEX64_APIC_SIZE            0x80

#define APIC_REG_ID                 0x00
#define APIC_REG_VERSION            0x08
#define APIC_REG_IPI_STATUS         0x10
#define APIC_REG_ICR                0x18
#define APIC_REG_START              0x20
#define APIC_REG_ARG                0x28
#define APIC_REG_RUNNING            0x30
#define APIC_REG_TIMER              0x40
#define APIC_ICR_DEST_MASK          0xFFULL
#define APIC_DEST_OTHERS            0xFF
#define APIC_ICR_TYPE_SHIFT         8
#define APIC_IPI_FIXED              0
#define APIC_IPI_INIT               1
#define APIC_IPI_STARTUP            2

#define APIC_VERSION                1

#define APIC_PENDING_IPI            (1u << 0)
#define APIC_PENDING_TIMER          (1u << 1)

typedef struct emex64_lapic {
    emex64_vtimer_t timer;
    UInt32 pending;
    UInt32 ipi_status;
    UInt64 start;
    UInt64 arg;
} emex64_lapic_t;

typedef struct __E64Core *E64CoreRef;

void emex64_lapic_init(emex64_lapic_t *lapic);
UInt64 emex64_lapic_read(E64CoreRef core, UInt64 offset);
void emex64_lapic_write(E64CoreRef core, UInt64 offset, UInt64 value);

#endif /* EMEX64VM_DEVICE_APIC_H */
