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

#ifndef EMEX64VM_DEVICE_PCIE_H
#define EMEX64VM_DEVICE_PCIE_H

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/device/base.h>

/* root complex registers */
#define EMEX64_PCIE_SIZE            0x40
#define PCIE_RC_REG_ID              0x00
#define PCIE_RC_REG_ECAM_BASE       0x08
#define PCIE_RC_REG_ECAM_BUSES      0x10
#define PCIE_RC_REG_MMIO_BASE       0x18
#define PCIE_RC_REG_MMIO_SIZE       0x20
#define PCIE_RC_REG_IRQ_BASE        0x28
#define PCIE_RC_REG_IRQ_COUNT       0x30

#define PCIE_RC_ID_MAGIC            0x49435045ULL
#define PCIE_RC_VERSION             1

/* configuration space registers */
#define PCIE_CFG_ID                 0x00
#define PCIE_CFG_CONTROL            0x08
#define PCIE_CFG_STATUS             0x10
#define PCIE_CFG_IRQ                0x18
#define PCIE_CFG_HEADER             0x20
#define PCIE_CFG_SUBSYSTEM          0x28
#define PCIE_CFG_BAR(n)             (0x40 + (n) * 0x10)
#define PCIE_CFG_BAR_SIZE(n)        (0x48 + (n) * 0x10)

#define PCIE_CONTROL_MEM            (1ULL << 0)
#define PCIE_CONTROL_MASTER         (1ULL << 1)
#define PCIE_CONTROL_IRQ            (1ULL << 2)
#define PCIE_STATUS_IRQ             (1ULL << 0)

#define PCIE_MAX_BARS               6
#define PCIE_MAX_DEVICES            32
#define PCIE_MAX_FUNCTIONS          8

#define PCIE_VENDOR_EMEXLABS        0xE640

typedef struct emex64_pcie emex64_pcie_t;
typedef struct emex64_pcie_function emex64_pcie_function_t;
typedef struct __E64Machine *E64MachineRef;

typedef struct {
    UInt64 (*bar_read)(emex64_pcie_function_t *function, UInt8 bar, UInt64 offset);
    void (*bar_write)(emex64_pcie_function_t *function, UInt8 bar, UInt64 offset, UInt64 value);
} emex64_pcie_ops_t;

struct emex64_pcie_function {
    UInt16 vendor;
    UInt16 device;
    UInt32 class_code;
    UInt8 revision;
    UInt64 subsystem;
    UInt64 bar_size[PCIE_MAX_BARS];
    const emex64_pcie_ops_t *ops;
    void *context;
    UInt64 bar_base[PCIE_MAX_BARS];
    UInt64 control;
    UInt64 status;
    SInt32 irq;
    UInt8 bus, dev, fn;
    emex64_pcie_t *pcie;
};

struct emex64_pcie {
    E64MachineRef machine;
    emex64_pcie_function_t *functions[PCIE_MAX_DEVICES * PCIE_MAX_FUNCTIONS];
};

emex64_pcie_t *emex64_pcie_alloc(E64MachineRef machine);
void emex64_pcie_dealloc(emex64_pcie_t *pcie);
void emex64_pcie_reset(emex64_pcie_t *pcie);

Boolean emex64_pcie_attach(emex64_pcie_t *pcie, emex64_pcie_function_t *function, UInt8 dev, UInt8 fn);

void emex64_pcie_raise_irq(emex64_pcie_function_t *function);

void *emex64_pcie_dma_map(emex64_pcie_function_t *function, UInt64 paddr, UInt64 len, Boolean forWrite);

#endif /* EMEX64VM_DEVICE_PCIE_H */
