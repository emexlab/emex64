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

#include <stdlib.h>
#include <string.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/E64Memory.h>
#include <EmexToolchain/VM/E64MMIORegion.h>
#include <EmexToolchain/VM/device/internal/controller/E64IC.h>
#include <EmexToolchain/VM/device/board/pcie/pcie.h>

static UInt64 pcie_rc_read(E64CoreRef core,
                           void *device,
                           UInt64 offset,
                           EFSize size)
{
    /* root complex registers */
    switch(offset)
    {
        case PCIE_RC_REG_ID:
            return PCIE_RC_ID_MAGIC | ((UInt64)PCIE_RC_VERSION << 32);
        case PCIE_RC_REG_ECAM_BASE:
            return EMEX64_PCIE_ECAM_BASE;
        case PCIE_RC_REG_ECAM_BUSES:
            return 1;
        case PCIE_RC_REG_MMIO_BASE:
            return EMEX64_PCIE_MMIO_BASE;
        case PCIE_RC_REG_MMIO_SIZE:
            return EMEX64_PCIE_MMIO_SIZE;
        case PCIE_RC_REG_IRQ_BASE:
            return EMEX64_IRQ_PCIE_BASE;
        case PCIE_RC_REG_IRQ_COUNT:
            return EMEX64_IRQ_PCIE_COUNT;
        default:
            return 0;
    }
}

static void pcie_rc_write(E64CoreRef core,
                          void *device,
                          UInt64 offset,
                          UInt64 value,
                          EFSize size)
{
    /* read only */
}

static emex64_pcie_function_t *pcie_function_for_ecam_offset(emex64_pcie_t *pcie,
                                                             UInt64 offset,
                                                             UInt64 *reg)
{
    UInt64 bus = offset >> 21;
    UInt64 dev = (offset >> 16) & 0x1F;
    UInt64 fn = (offset >> 13) & 0x7;

    *reg = offset & EMEX64_PAGE_MASK;
    if(bus != 0)
    {
        return NULL;
    }

    return pcie->functions[dev * PCIE_MAX_FUNCTIONS + fn];
}

static UInt64 pcie_cfg_read(E64CoreRef core,
                            void *device,
                            UInt64 offset,
                            EFSize size)
{
    emex64_pcie_t *pcie = device;
    UInt64 reg = 0;
    emex64_pcie_function_t *f = pcie_function_for_ecam_offset(pcie, offset, &reg);
    if(f == NULL)
    {
        return ~0ULL;
    }

    switch(reg)
    {
        case PCIE_CFG_ID:
            return (UInt64)f->vendor | ((UInt64)f->device << 16) | ((UInt64)(f->class_code & 0xFFFFFF) << 32) | ((UInt64)f->revision << 56);
        case PCIE_CFG_CONTROL:
            return f->control;
        case PCIE_CFG_STATUS:
            return f->status;
        case PCIE_CFG_IRQ:
            return f->irq < 0 ? ~0ULL : (UInt64)f->irq;
        case PCIE_CFG_HEADER:
        {
            Boolean multi = false;
            for(UInt8 i = 0; i < PCIE_MAX_FUNCTIONS; i++)
            {
                if(i != f->fn && pcie->functions[f->dev * PCIE_MAX_FUNCTIONS + i] != NULL)
                {
                    multi = true;
                }
            }
            return multi ? (1ULL << 8) : 0;
        }
        case PCIE_CFG_SUBSYSTEM:
            return f->subsystem;
        default:
            break;
    }

    if(reg >= PCIE_CFG_BAR(0) && reg < PCIE_CFG_BAR(PCIE_MAX_BARS))
    {
        UInt8 bar = (UInt8)((reg - PCIE_CFG_BAR(0)) / 0x10);
        return (reg & 0x8) ? f->bar_size[bar] : f->bar_base[bar];
    }

    return 0;
}

static void pcie_cfg_write(E64CoreRef core,
                           void *device,
                           UInt64 offset,
                           UInt64 value,
                           EFSize size)
{
    emex64_pcie_t *pcie = device;
    UInt64 reg = 0;
    emex64_pcie_function_t *f = pcie_function_for_ecam_offset(pcie, offset, &reg);
    if(f == NULL)
    {
        return;
    }

    switch(reg)
    {
        case PCIE_CFG_CONTROL:
            f->control = value & (PCIE_CONTROL_MEM | PCIE_CONTROL_MASTER | PCIE_CONTROL_IRQ);
            return;
        case PCIE_CFG_STATUS:
            f->status &= ~value;
            return;
        default:
            break;
    }

    if(reg >= PCIE_CFG_BAR(0) && reg < PCIE_CFG_BAR(PCIE_MAX_BARS) && !(reg & 0x8))
    {
        UInt8 bar = (UInt8)((reg - PCIE_CFG_BAR(0)) / 0x10);
        if(f->bar_size[bar] != 0)
        {
            f->bar_base[bar] = value & ~(f->bar_size[bar] - 1);
        }
    }
}

static emex64_pcie_function_t *pcie_function_for_address(emex64_pcie_t *pcie,
                                                         UInt64 addr,
                                                         UInt8 *bar,
                                                         UInt64 *offset)
{
    for(EFIndex i = 0; i < PCIE_MAX_DEVICES * PCIE_MAX_FUNCTIONS; i++)
    {
        emex64_pcie_function_t *f = pcie->functions[i];
        if(f == NULL || !(f->control & PCIE_CONTROL_MEM))
        {
            continue;
        }

        for(UInt8 b = 0; b < PCIE_MAX_BARS; b++)
        {
            if(f->bar_size[b] != 0 && addr >= f->bar_base[b] && addr - f->bar_base[b] < f->bar_size[b])
            {
                *bar = b;
                *offset = addr - f->bar_base[b];
                return f;
            }
        }
    }

    return NULL;
}

static UInt64 pcie_mmio_read(E64CoreRef core,
                             void *device,
                             UInt64 offset,
                             EFSize size)
{
    UInt8 bar = 0;
    UInt64 bar_offset = 0;
    emex64_pcie_function_t *f = pcie_function_for_address(device, EMEX64_PCIE_MMIO_BASE + offset, &bar, &bar_offset);
    if(f == NULL || f->ops == NULL || f->ops->bar_read == NULL)
    {
        return ~0ULL;
    }

    return f->ops->bar_read(f, bar, bar_offset);
}

static void pcie_mmio_write(E64CoreRef core,
                            void *device,
                            UInt64 offset,
                            UInt64 value,
                            EFSize size)
{
    UInt8 bar = 0;
    UInt64 bar_offset = 0;
    emex64_pcie_function_t *f = pcie_function_for_address(device, EMEX64_PCIE_MMIO_BASE + offset, &bar, &bar_offset);
    if(f == NULL || f->ops == NULL || f->ops->bar_write == NULL)
    {
        return;
    }

    f->ops->bar_write(f, bar, bar_offset, value);
}

emex64_pcie_t *emex64_pcie_alloc(E64MachineRef machine)
{
    emex64_pcie_t *pcie = calloc(1, sizeof(emex64_pcie_t));
    if(pcie == NULL)
    {
        return NULL;
    }

    pcie->machine = machine;

    EFAUTOREL E64MMIORegionRef rc = E64MMIORegionCreate(kEFAllocatorDefault, EMEX64_PCIE_BASE, EMEX64_PCIE_SIZE, pcie, pcie_rc_read, pcie_rc_write);
    EFAUTOREL E64MMIORegionRef ecam = E64MMIORegionCreate(kEFAllocatorDefault, EMEX64_PCIE_ECAM_BASE, EMEX64_PCIE_ECAM_SIZE, pcie, pcie_cfg_read, pcie_cfg_write);
    EFAUTOREL E64MMIORegionRef mmio = E64MMIORegionCreate(kEFAllocatorDefault, EMEX64_PCIE_MMIO_BASE, EMEX64_PCIE_MMIO_SIZE, pcie, pcie_mmio_read, pcie_mmio_write);
    if(rc == NULL || ecam == NULL || mmio == NULL ||
       !E64MMIOBusRegisterRegion(machine->mmio_bus, rc) ||
       !E64MMIOBusRegisterRegion(machine->mmio_bus, ecam) ||
       !E64MMIOBusRegisterRegion(machine->mmio_bus, mmio))
    {
        free(pcie);
        return NULL;
    }

    return pcie;
}

void emex64_pcie_dealloc(emex64_pcie_t *pcie)
{
    free(pcie);
}

void emex64_pcie_reset(emex64_pcie_t *pcie)
{
    if(pcie == NULL)
    {
        return;
    }
    for(UInt32 i = 0; i < PCIE_MAX_DEVICES * PCIE_MAX_FUNCTIONS; i++)
    {
        emex64_pcie_function_t *f = pcie->functions[i];
        if(f == NULL)
        {
            continue;
        }
        for(UInt8 b = 0; b < PCIE_MAX_BARS; b++)
        {
            f->bar_base[b] = 0;
        }
        f->control = 0;
        f->status = 0;
    }
}

Boolean emex64_pcie_attach(emex64_pcie_t *pcie,
                           emex64_pcie_function_t *function,
                           UInt8 dev,
                           UInt8 fn)
{
    if(pcie == NULL ||
       function == NULL ||
       dev >= PCIE_MAX_DEVICES ||
       fn >= PCIE_MAX_FUNCTIONS ||
       pcie->functions[dev * PCIE_MAX_FUNCTIONS + fn] != NULL)
    {
        return false;
    }

    for(UInt8 b = 0; b < PCIE_MAX_BARS; b++)
    {
        UInt64 sz = function->bar_size[b];
        if(sz != 0 && (sz < EMEX64_PAGE_SIZE || (sz & (sz - 1)) != 0))
        {
            return false;
        }
        function->bar_base[b] = 0;
    }

    function->pcie = pcie;
    function->bus = 0;
    function->dev = dev;
    function->fn = fn;
    function->control = 0;
    function->status = 0;
    function->irq = EMEX64_IRQ_PCIE_BASE + (dev % EMEX64_IRQ_PCIE_COUNT);
    pcie->functions[dev * PCIE_MAX_FUNCTIONS + fn] = function;
    return true;
}

void emex64_pcie_raise_irq(emex64_pcie_function_t *function)
{
    if(function == NULL || function->irq < 0)
    {
        return;
    }

    function->status |= PCIE_STATUS_IRQ;
    if(function->control & PCIE_CONTROL_IRQ)
    {
        E64ICRaiseInterrupt(function->pcie->machine->intc, function->irq);
    }
}

void *emex64_pcie_dma_map(emex64_pcie_function_t *function,
                          UInt64 paddr,
                          UInt64 len,
                          Boolean forWrite)
{
    if(function == NULL || !(function->control & PCIE_CONTROL_MASTER))
    {
        return NULL;
    }

    return E64MemoryMapPhysical(function->pcie->machine->memory, paddr, len, forWrite);
}
