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

#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/VM/E64Machine.h>
#include <EmexToolchain/VM/E64Core.h>
#include <EmexToolchain/VM/device/internal/controller/apic.h>

void emex64_lapic_init(emex64_lapic_t *lapic)
{
    emex64_vtimer_init(&lapic->timer);
    lapic->pending = 0;
    lapic->ipi_status = 0;
    lapic->start = 0;
    lapic->arg = 0;
}

UInt64 emex64_lapic_read(E64CoreRef core,
                         UInt64 offset)
{
    __E64Machine machine = (__E64Machine)core->machine;
    switch(offset)
    {
        case APIC_REG_ID:
            return core->id;
        case APIC_REG_VERSION:
            return APIC_VERSION | (UInt64)machine->core_count << 8;
        case APIC_REG_IPI_STATUS:
            return __atomic_load_n(&core->lapic.ipi_status, __ATOMIC_ACQUIRE);
        case APIC_REG_START:
            return core->lapic.start;
        case APIC_REG_ARG:
            return core->lapic.arg;
        case APIC_REG_RUNNING:
        {
            UInt64 running = 0;
            for(UInt8 i = 0; i < machine->core_count; i++)
            {
                if(__atomic_load_n(&machine->cores[i]->run_state, __ATOMIC_ACQUIRE) == E64_CORE_RUNNING)
                {
                    running |= 1ULL << i;
                }
            }
            return running;
        }
        default:
            if(offset >= APIC_REG_TIMER && offset < APIC_REG_TIMER + 0x28)
            {
                return emex64_vtimer_read(&core->lapic.timer, offset - APIC_REG_TIMER);
            }
            return 0;
    }
}

void emex64_lapic_write(E64CoreRef core,
                        UInt64 offset,
                        UInt64 value)
{
    __E64Machine machine = (__E64Machine)core->machine;
    switch(offset)
    {
        case APIC_REG_IPI_STATUS:
            __atomic_fetch_and(&core->lapic.ipi_status, ~(UInt32)value, __ATOMIC_ACQ_REL);
            break;
        case APIC_REG_ICR:
        {
            UInt64 dest = value & APIC_ICR_DEST_MASK;
            UInt64 type = (value >> APIC_ICR_TYPE_SHIFT) & 0xF;
            for(UInt8 i = 0; i < machine->core_count; i++)
            {
                if(dest == APIC_DEST_OTHERS ? i != core->id : i == dest)
                {
                    __E64CoreSendIPI(core, machine->cores[i], type);
                }
            }
            break;
        }
        case APIC_REG_START:
            core->lapic.start = value;
            break;
        case APIC_REG_ARG:
            core->lapic.arg = value;
            break;
        default:
            if(offset >= APIC_REG_TIMER && offset < APIC_REG_TIMER + 0x28)
            {
                emex64_vtimer_write(&core->lapic.timer, offset - APIC_REG_TIMER, value);
            }
            break;
    }
}
