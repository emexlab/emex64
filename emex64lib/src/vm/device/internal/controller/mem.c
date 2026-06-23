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
#include <emex64lib/vm/machine.h>
#include <emex64lib/vm/device/internal/controller/mem.h>

uint64_t emex64_mc_read(emex64_core_t *core,
                        void *device,
                        uint64_t offset,
                        int size)
{
    if(offset == EMEX64_MC_REG_SIZE)
    {
        return core->machine->memory->memory_size;
    }
    else if(offset == EMEX64_MC_REG_KTRR_SIZE)
    {
        return core->machine->memory->ktrr_size;
    }
    
    return core->machine->memory->ktrr_locked;
}

void emex64_mc_write(emex64_core_t *core,
                     void *device,
                     uint64_t offset,
                     uint64_t value,
                     int size)
{
    if(offset == EMEX64_MC_REG_KTRR_SIZE)
    {
        if(core->machine->memory->ktrr_locked)
        {
            core->cr_state.crexc.exception = kEmex64ExceptionKTRRViolation;
            return;
        }

        core->machine->memory->ktrr_size = value;
    }
    else if(offset == EMEX64_MC_REG_KTRR_LOCKED)
    {
        if(core->machine->memory->ktrr_locked)
        {
            core->cr_state.crexc.exception = kEmex64ExceptionKTRRViolation;
            return;
        }

        core->machine->memory->ktrr_locked = value;
    }

    return;
}
