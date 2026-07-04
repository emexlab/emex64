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
#include <EmexToolchain/vm/machine.h>
#include <EmexToolchain/vm/device/internal/controller/mem.h>

uint64_t emex64_mc_read(emex64_core_t *core,
                        void *device,
                        uint64_t offset,
                        int size)
{
    if(offset == EMEX64_MC_REG_SIZE)
    {
        return E64MemoryGetSize(core->machine->memory);
    }
    else if(offset == EMEX64_MC_REG_KTRR_SIZE)
    {
        return E64MemoryGetKTRRSize(core->machine->memory);
    }
    
    return E64MemoryIsKTRRLocked(core->machine->memory);
}

void emex64_mc_write(emex64_core_t *core,
                     void *device,
                     uint64_t offset,
                     uint64_t value,
                     int size)
{
    if(offset == EMEX64_MC_REG_KTRR_SIZE)
    {
        if(!E64MemorySetKTRRSize(core->machine->memory, value))
        {
            core->cr_state.crexc.exception = kE64ExceptionKTRRViolation;
            return;
        }
    }
    else if(offset == EMEX64_MC_REG_KTRR_LOCKED)
    {
        if(E64MemoryIsKTRRLocked(core->machine->memory))
        {
            core->cr_state.crexc.exception = kE64ExceptionKTRRViolation;
            return;
        }

        E64MemoryLockKTRR(core->machine->memory);
    }

    return;
}
