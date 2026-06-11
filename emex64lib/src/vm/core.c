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

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <emex64lib/vm/core.h>
#include <emex64lib/vm/memory.h>
#include <emex64lib/vm/mmu.h>
#include <emex64lib/vm/machine.h>

#include <emex64lib/vm/device/interrupt.h>
#include <emex64lib/vm/device/timer.h>
#include <emex64lib/vm/device/ac97.h>

#include <emex64lib/vm/instruction/core.h>
#include <emex64lib/vm/instruction/data.h>
#include <emex64lib/vm/instruction/alu.h>
#include <emex64lib/vm/instruction/ctrl.h>

#include <emex64lib/support/bitwalker.h>
#include <emex64lib/support/likely.h>

#if defined(__APPLE__)
#include <CoreFoundation/CFRunLoop.h>
#endif /* __APPLE__ */

static emex64_opfunc_entry_t kOpfuncTable[] = {
    /* core operations */
    [kEmex64OpcodeHLT] = { .func = emex64_op_hlt, .maxargs = 0 },
    [kEmex64OpcodeNOP] = { .func = emex64_op_nop, .maxargs = 0 },

    /* data operations */
    [kEmex64OpcodeMOV] = { .func = emex64_op_mov, .maxargs = 2 },
    [kEmex64OpcodeSWP] = { .func = emex64_op_swp, .maxargs = 2 },
    [kEmex64OpcodeSWPZ] = { .func = emex64_op_swpz, .maxargs = 2 },
    [kEmex64OpcodePUSH] = { .func = emex64_op_push, .maxargs = 32 },
    [kEmex64OpcodePOP] = { .func = emex64_op_pop, .maxargs = 32 },
    [kEmex64OpcodeLDB] = { .func = emex64_op_ldb, .maxargs = 2 },
    [kEmex64OpcodeLDW] = { .func = emex64_op_ldw, .maxargs = 2 },
    [kEmex64OpcodeLDD] = { .func = emex64_op_ldd, .maxargs = 2 },
    [kEmex64OpcodeLDQ] = { .func = emex64_op_ldq, .maxargs = 2 },
    [kEmex64OpcodeSTB] = { .func = emex64_op_stb, .maxargs = 2 },
    [kEmex64OpcodeSTW] = { .func = emex64_op_stw, .maxargs = 2 },
    [kEmex64OpcodeSTD] = { .func = emex64_op_std, .maxargs = 2 },
    [kEmex64OpcodeSTQ] = { .func = emex64_op_stq, .maxargs = 2 },

    /* arithmetic operations */
    [kEmex64OpcodeADD] = { .func = emex64_op_add, .maxargs = 3 },
    [kEmex64OpcodeSUB] = { .func = emex64_op_sub, .maxargs = 3 },
    [kEmex64OpcodeMUL] = { .func = emex64_op_mul, .maxargs = 3 },
    [kEmex64OpcodeDIV] = { .func = emex64_op_div, .maxargs = 3 },
    [kEmex64OpcodeIDIV] = { .func = emex64_op_idiv, .maxargs = 3 },
    [kEmex64OpcodeMOD] = { .func = emex64_op_mod, .maxargs = 3 },
    [kEmex64OpcodeNOT] = { .func = emex64_op_not, .maxargs = 32 },
    [kEmex64OpcodeNEG] = { .func = emex64_op_neg, .maxargs = 32 },
    [kEmex64OpcodeAND] = { .func = emex64_op_and, .maxargs = 3 },
    [kEmex64OpcodeOR] = { .func = emex64_op_or, .maxargs = 3 },
    [kEmex64OpcodeXOR] = { .func = emex64_op_xor, .maxargs = 3 },
    [kEmex64OpcodeSHR] = { .func = emex64_op_shr, .maxargs = 3 },
    [kEmex64OpcodeSHL] = { .func = emex64_op_shl, .maxargs = 3 },
    [kEmex64OpcodeSAR] = { .func = emex64_op_sar, .maxargs = 3 },
    [kEmex64OpcodeROR] = { .func = emex64_op_ror, .maxargs = 3 },
    [kEmex64OpcodeROL] = { .func = emex64_op_rol, .maxargs = 3 },
    [kEmex64OpcodePDEP] = { .func = emex64_op_pdep, .maxargs = 3 },
    [kEmex64OpcodePEXT] = { .func = emex64_op_pext, .maxargs = 3 },
    [kEmex64OpcodeBSWAPW] = { .func = emex64_op_bswapw, .maxargs = 1 },
    [kEmex64OpcodeBSWAPD] = { .func = emex64_op_bswapd, .maxargs = 1 },
    [kEmex64OpcodeBSWAPQ] = { .func = emex64_op_bswapq, .maxargs = 1 },
    [kEmex64OpcodeINC] = { .func = emex64_op_inc, .maxargs = 32 },
    [kEmex64OpcodeDEC] = { .func = emex64_op_dec, .maxargs = 32 },

    /* control flow operations */
    [kEmex64OpcodeB] = { .func = emex64_op_b, .maxargs = 1 },
    [kEmex64OpcodeCMP] = { .func = emex64_op_cmp, .maxargs = 2 },
    [kEmex64OpcodeBE] = { .func = emex64_op_be, .maxargs = 1 },
    [kEmex64OpcodeBNE] = { .func = emex64_op_bne, .maxargs = 1 },
    [kEmex64OpcodeBLT] = { .func = emex64_op_blt, .maxargs = 1 },
    [kEmex64OpcodeBGT] = { .func = emex64_op_bgt, .maxargs = 1 },
    [kEmex64OpcodeBLE] = { .func = emex64_op_ble, .maxargs = 1 },
    [kEmex64OpcodeBGE] = { .func = emex64_op_bge, .maxargs = 1 },
    [kEmex64OpcodeBZ] = { .func = emex64_op_bz, .maxargs = 2 },
    [kEmex64OpcodeBNZ] = { .func = emex64_op_bnz, .maxargs = 2 },
    [kEmex64OpcodeBLW] = { .func = emex64_op_blw, .maxargs = 32 },
    [kEmex64OpcodeWRET] = { .func = emex64_op_wret, .maxargs = 0 },
    [kEmex64OpcodeIRET] = { .func = emex64_op_iret, .maxargs = 0 },
    [kEmex64OpcodeBL] = { .func = emex64_op_bl, .maxargs = 1 },
    [kEmex64OpcodeRET] = { .func = emex64_op_ret, .maxargs = 0 },

    /* arithmetic operations v2 */
    [kEmex64OpcodeCLR] = { .func = emex64_op_clr, .maxargs = 32 },
};

static const uint8_t kImmBits[] = {
    [kEmex64ParameterCodingImm5] = 5,
    [kEmex64ParameterCodingImm8] = 8,
    [kEmex64ParameterCodingImm16] = 16,
    [kEmex64ParameterCodingImm32] = 32,
    [kEmex64ParameterCodingImm64] = 64,
    [kEmex64ParameterCodingAddr64] = 64,
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
     * rendering the entire state useless.
     */
    core->rl[kEmex64RegisterCR0] = kEmex64ElevationLevelSecureMonitor;

    return core;
}

void emex64_core_dealloc(emex64_core_t *core)
{
    free(core);
}

static inline bool emex64_core_decode_instruction_at_pc(emex64_core_t *core)
{
    #if !EMEX64VM_USE_INSCACHE
    uint64_t pc_addr = core->rl[kEmex64RegisterPC];
    uint64_t cr_pte = core->rl[kEmex64RegisterCR4];
    if((((cr_pte & EMEX64_MMU_MASK_FLAGS) & kEmex64MMUPTPresent) && !core->in_interrupt) &&
        unlikely(!emex64_mmu_access(core, pc_addr, kEmex64MMUAccessExec, &pc_addr)))
    {
        /* MMU wrote exception, not needed to fill in our own */
        return false;
    }

    /*
     * check KTRR if kernel or secure monitor level
     * to prevent execution from non KTRR memory.
     */
    if(core->rl[kEmex64RegisterCR0] >= kEmex64ElevationLevelKernel)
    {
        if(core->machine->memory->ktrr_locked &&
           core->machine->memory->ktrr_size < pc_addr)
        {
            /* a KTRR violation. OOB of KTRR region. */
            core->rl[kEmex64RegisterCR2] = kEmex64ExceptionKTRRViolation;
            return false;
        }
    }

    if(unlikely(!emex64_memory_access(core, pc_addr, 256)))
    {
        core->rl[kEmex64RegisterCR2] = kEmex64ExceptionBadAccess;
        return false;
    }

    bitwalker_t bw = emex64_memory_bitwalker_template;
    bw.buffer = core->machine->memory->memory + pc_addr;
    #else
    if(unlikely(!emex64_memory_cpy(core, core->op.inscache,  core->rl[kEmex64RegisterPC], 256, kEmex64MemoryActionExecute)))
    {
        /* callee wrote exception already */
        return false;
    }

    bitwalker_t bw = emex64_memory_bitwalker_template;
    bw.buffer = core->op.inscache;
    #endif /* EMEX64VM_USE_INSCACHE */

    enum kEmex64Opcode opcode = (uint8_t)bitwalker_read(&bw, 8);
    if(unlikely(opcode > kEmex64OpcodeMAX))
    {
        core->rl[kEmex64RegisterCR2] = kEmex64ExceptionBadInstruction;
        return false;
    }

    core->op.opcode = opcode;
    core->op.op = kOpfuncTable[opcode];

    /*
     * parameter decoder, this decoding loop decodes
     * all parameters the instruction defines.
     */
    uint8_t maxarg = core->op.op.maxargs;
    uint8_t i = 0;
    for(; i < maxarg; i++)
    {
        enum kEmex64ParameterCoding coding = (uint8_t)bitwalker_read(&bw, 3);
        core->op.param_coding[i] = coding;
        switch(coding)
        {
            case kEmex64ParameterCodingEnd:
                /* a the weekend reference lol */
                goto escape_from_la;
            case kEmex64ParameterCodingReg:
            {
                uint8_t rcnt = (uint8_t)bitwalker_read(&bw, 5);

                /*
                 * userspace shouldn't be able to access control
                 * registers, a attacker could defeat ASLR in the
                 * future by reading CRPTB which is the page table
                 * address.
                 * 
                 * another reason is that control registers define
                 * execution behaviour. if a userspace program can
                 * write to CREL or CRKSP, they can cause panics
                 * or spy on the user.
                 */
                if(unlikely(rcnt > kEmex64RegisterRR && core->rl[kEmex64RegisterCR0] < kEmex64ElevationLevelKernel))
                {
                    core->rl[kEmex64RegisterCR2] = kEmex64ExceptionPermission;
                    return false;
                }

                core->op.param[i] = &(core->rl[rcnt]);
                break;
            }
            case kEmex64ParameterCodingAddr64:
                bitwalker_align_byte(&bw);
                /*
                 * fallthrough, because kEmex64ParameterCodingAddr64
                 * was invented to make relocation possible, because
                 * the decoder would align to the next byte boundary.
                 */
            case kEmex64ParameterCodingImm5:
            case kEmex64ParameterCodingImm8:
            case kEmex64ParameterCodingImm16:
            case kEmex64ParameterCodingImm32:
            case kEmex64ParameterCodingImm64:
                core->op.imm[i] = bitwalker_read(&bw, kImmBits[coding]);
                core->op.param[i] = &(core->op.imm[i]);
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
    core->op.ilen = bitwalker_bytes_used(&bw);

    return true;
}

static void *emex64_core_execute_thread(void *arg)
{
    assert(arg != NULL);

    /* execution loop */
    emex64_core_t *core = arg;
    for(;;)
    {
        if(unlikely(!emex64_core_decode_instruction_at_pc(core)))
        {
            continue;
        }

        /* the part of executing the instruction */
        core->op.op.func(core);
        core->rl[kEmex64RegisterPC] += core->op.ilen;

        /*
         * currently exceptions happening in a interrupt
         * are ignored, this shall not be the case long
         * term.
         */
        if(!core->in_interrupt)
        {
            /* handling exception if applicable */
            if(unlikely(core->rl[kEmex64RegisterCR2] != kEmex64ExceptionNone))
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

        /* serve interrupt for the interrupt controller */
        emex64_serve_interrupt_if_needed(core);

        /*
         * tick the timer always (has to always be ticked for the interrupt controller)
         * we cannot just freeze the thread, otherwise the timer won't fire any
         * interrupts.
         */
        emex64_timer_tick(core->machine->timer, emex64_get_host_cycles());

        /* tick the AC97 DMA engine if the device is present */
        if(core->machine->ac97 != NULL)
            emex64_ac97_tick(core->machine->ac97, core);
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
    exit(0); /* FIXME: this is so it works anyways */
    #endif /* __APPLE__ */
    #endif /* #if EMEX64VM_DEVICE_DISPLAY */
    
    if(pthread_self() == core->pthread)
    {
        pthread_exit(NULL);
    }
    
    pthread_cancel(core->pthread);
}
