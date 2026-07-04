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
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <emex64lib/vm/core.h>
#include <emex64lib/vm/memory.h>
#include <emex64lib/vm/machine.h>
#include <emex64lib/vm/device/internal/controller/ic.h>
#include <emex64lib/vm/device/internal/timer.h>
#include <emex64lib/vm/instruction/core.h>
#include <emex64lib/vm/instruction/data.h>
#include <emex64lib/vm/instruction/alu.h>
#include <emex64lib/vm/instruction/ctrl.h>
#include <emex64lib/support/bitbolt.h>
#include <emex64lib/support/likely.h>

#if defined(__APPLE__)
#include <CoreFoundation/CFRunLoop.h>
#endif /* __APPLE__ */

const emex64_opfunc_entry_t kE64OpfuncTable[] = {
    /* core operations */
    [kE64OpcodeHLT] = { .func = emex64_op_hlt, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeNOP] = { .func = emex64_op_nop, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000 },

    /* data operations */
    [kE64OpcodeMOV] = { .func = emex64_op_mov, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeSWP] = { .func = emex64_op_swp, .minargs = 2, .maxargs = 2, .argmask = 0b11000000000000000000000000000000 },
    [kE64OpcodeSWPZ] = { .func = emex64_op_swpz, .minargs = 2, .maxargs = 2, .argmask = 0b11000000000000000000000000000000 },
    [kE64OpcodePUSH] = { .func = emex64_op_push, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodePOP] = { .func = emex64_op_pop, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111 },
    [kE64OpcodeLDB] = { .func = emex64_op_ldb, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeLDW] = { .func = emex64_op_ldw, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeLDD] = { .func = emex64_op_ldd, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeLDQ] = { .func = emex64_op_ldq, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeSTB] = { .func = emex64_op_stb, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeSTW] = { .func = emex64_op_stw, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeSTD] = { .func = emex64_op_std, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeSTQ] = { .func = emex64_op_stq, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000 },

    /* arithmetic operations */
    [kE64OpcodeADD] = { .func = emex64_op_add, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeSUB] = { .func = emex64_op_sub, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeMUL] = { .func = emex64_op_mul, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeDIV] = { .func = emex64_op_div, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeIDIV] = { .func = emex64_op_idiv, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeMOD] = { .func = emex64_op_mod, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeNOT] = { .func = emex64_op_not, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111 },
    [kE64OpcodeNEG] = { .func = emex64_op_neg, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111 },
    [kE64OpcodeAND] = { .func = emex64_op_and, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeOR] = { .func = emex64_op_or, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeXOR] = { .func = emex64_op_xor, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeSHR] = { .func = emex64_op_shr, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeSHL] = { .func = emex64_op_shl, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeSAR] = { .func = emex64_op_sar, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeROR] = { .func = emex64_op_ror, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeROL] = { .func = emex64_op_rol, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodePDEP] = { .func = emex64_op_pdep, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodePEXT] = { .func = emex64_op_pext, .minargs = 2, .maxargs = 3, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeBSWAPW] = { .func = emex64_op_bswapw, .minargs = 1, .maxargs = 1, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeBSWAPD] = { .func = emex64_op_bswapd, .minargs = 1, .maxargs = 1, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeBSWAPQ] = { .func = emex64_op_bswapq, .minargs = 1, .maxargs = 1, .argmask = 0b10000000000000000000000000000000 },
    [kE64OpcodeINC] = { .func = emex64_op_inc, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111 },
    [kE64OpcodeDEC] = { .func = emex64_op_dec, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111 },

    /* control flow operations */
    [kE64OpcodeB] = { .func = emex64_op_b, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeCMP] = { .func = emex64_op_cmp, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeBE] = { .func = emex64_op_be, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeBNE] = { .func = emex64_op_bne, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeBLT] = { .func = emex64_op_blt, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeBGT] = { .func = emex64_op_bgt, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeBLE] = { .func = emex64_op_ble, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeBGE] = { .func = emex64_op_bge, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeBZ] = { .func = emex64_op_bz, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeBNZ] = { .func = emex64_op_bnz, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeBLW] = { .func = emex64_op_blw, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeWRET] = { .func = emex64_op_wret, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeIRET] = { .func = emex64_op_iret, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeBL] = { .func = emex64_op_bl, .minargs = 1, .maxargs = 1, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeRET] = { .func = emex64_op_ret, .minargs = 0, .maxargs = 0, .argmask = 0b00000000000000000000000000000000 },

    /* data operations v2 */
    [kE64OpcodeCLR] = { .func = emex64_op_clr, .minargs = 1, .maxargs = EMEX64_MAX_ARGS, .argmask = 0b11111111111111111111111111111111 },
    [kE64OpcodeCMOV] = { .func = emex64_op_cmov, .minargs = 2, .maxargs = 2, .argmask = 0b00000000000000000000000000000000 },
    [kE64OpcodeCMOVB] = { .func = emex64_op_cmovb, .minargs = 2, .maxargs = 2, .argmask = 0b10000000000000000000000000000000 },
};

static const uint8_t kImmBits[] = {
    [kE64ParameterCodingImm5] = 5,
    [kE64ParameterCodingImm8] = 8,
    [kE64ParameterCodingImm16] = 16,
    [kE64ParameterCodingImm32] = 32,
    [kE64ParameterCodingImm64] = 64,
    [kE64ParameterCodingAddr64] = 64,
};

emex64_core_t *emex64_core_alloc()
{
    emex64_core_t *core = calloc(1, sizeof(emex64_core_t));
    if(core == NULL)
    {
        return NULL;
    }

    /*
     * setting it up with secure monitor, because
     * otherwise the core would sit in usermode
     * at start and the firmware wouldn't be able
     * to write to control registers, ultimatively
     * rendering the entire state functionless.
     */
    core->cr_state.crel.level = kE64ElevationLevelSecureMonitor;

    return core;
}

void emex64_core_dealloc(emex64_core_t *core)
{
    free(core);
}

static inline void emex64_core_execute_instruction_at_pc(emex64_core_t *core)
{
    if(unlikely(core->halted))
    {
        return;
    }

    /* TODO: KTRR check is missing */
    if(unlikely(!E64MemoryCoreCopyIn(core->machine->memory, core, core->op.inscache,  core->rl[kE64RegisterPC], EMEX64_MAX_ILEN, kE64MemoryActionExecute)))
    {
        /* callee wrote exception already */
        return;
    }

    bitbolt_t bb = { core->op.inscache, 0 };

    kE64Opcode opcode = (uint8_t)bb_read(&bb, 8);
    if(unlikely(opcode > kE64OpcodeMAX))
    {
        core->cr_state.crexc.exception = kE64ExceptionBadInstruction;
        return;
    }

    core->op.opcode = opcode;
    core->op.opce = kE64OpfuncTable[opcode];

    /*
     * parameter decoder, this decoding loop decodes
     * all parameters the instruction defines.
     */
    uint8_t maxarg = core->op.opce.maxargs;
    uint8_t i = 0;
    for(; i < maxarg; i++)
    {
        kE64ParameterCoding coding = (uint8_t)bb_read(&bb, 3);
        core->op.param_coding[i] = coding;
        switch(coding)
        {
            case kE64ParameterCodingEnd:
                /* a the weekend reference lol */
                goto escape_from_la;
            case kE64ParameterCodingReg:
            {
                core->op.param[i] = &(core->rl[(uint8_t)bb_read(&bb, 5)]);
                break;
            }
            case kE64ParameterCodingAddr64:
                /*
                 * fallthrough, because kE64ParameterCodingAddr64
                 * was invented to make relocation possible, because
                 * the decoder would align to the next byte boundary.
                 * So it is like Imm64 just with alignment.
                 */
                bb_align(&bb);
                [[fallthrough]];
            case kE64ParameterCodingImm5:
            case kE64ParameterCodingImm8:
            case kE64ParameterCodingImm16:
            case kE64ParameterCodingImm32:
            case kE64ParameterCodingImm64:
                core->op.immcache[i] = bb_read(&bb, kImmBits[coding]);
                core->op.param[i] = &(core->op.immcache[i]);
                break;
        }
    }

escape_from_la:
    /*
     * now we know all about this instruction, the
     * lenght and the amount of parameters, this is
     * very very very good.
     */
    core->op.param_cnt = i;
    core->op.ilen = (uint32_t)((bb.pos + 7u) >> 3);

    /* the part of executing the instruction */
    core->op.opce.func(core);
    core->rl[kE64RegisterPC] += core->op.ilen;   /* FIXME: IDK if it should increment or not due to interrupts */

    return;
}

static void *emex64_core_execute_thread(void *arg)
{
    assert(arg != NULL);

    /* execution loop */
    emex64_core_t *core = arg;
    for(;;)
    {
        emex64_core_execute_instruction_at_pc(core);
        emex64_serve_interrupt_if_needed(core);

        /*
         * currently exceptions happening in a interrupt
         * are ignored, this shall not be the case long
         * term.
         */
        if(!core->in_interrupt)
        {
            /* handling exception if applicable */
            if(unlikely(core->cr_state.crexc.exception != kE64ExceptionNone))
            {
                core->halted = true;
                emex64_raise_interrupt(core->machine, EMEX64_IRQ_EXCEPTION);
            }
            else if(unlikely(core->halted))
            {
                /*
                 * causing the host OS to schedule the process
                 * this way the cpu core is not burned.
                 */
                usleep(100);
            }
        }

        /*
         * tick the timer always (has to always be ticked for the interrupt controller)
         * we cannot just freeze the thread, otherwise the timer won't fire any
         * interrupts.
         */
        emex64_timer_tick(core->machine->timer, emex64_get_host_cycles());
    }

    return NULL;
}


void emex64_core_execute(emex64_core_t *core)
{
    assert(core != NULL && core->pthread == 0);

    pthread_create(&(core->pthread), NULL, emex64_core_execute_thread, (void*)core);

    #if EMEX64VM_DEVICE_DISPLAY
    #if defined(__APPLE__)
    /*
     * on Apple platforms the main thread must be in a run loop
     * in order to show the display.
     */
    CFRunLoopRun();
    #endif /* __APPLE__ */
    #endif /* #if EMEX64VM_DEVICE_DISPLAY */
    
    pthread_join(core->pthread, NULL);
}

void emex64_core_terminate(emex64_core_t *core)
{
    assert(core != NULL && core->pthread != 0);

    #if EMEX64VM_DEVICE_DISPLAY
    #if defined(__APPLE__)
    /* FIXME: this doesn't work */
    CFRunLoopStop(CFRunLoopGetMain());
    if(core->machine->display->enabled)
    {
        exit(0); /* FIXME: this is so it works anyways */
    }
    #endif /* __APPLE__ */
    #endif /* #if EMEX64VM_DEVICE_DISPLAY */
    
    if(pthread_self() == core->pthread)
    {
        pthread_exit(NULL);
    }
    
    pthread_cancel(core->pthread);
}
