/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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
#include <unistd.h>

#include <emex64lib/vm/core.h>
#include <emex64lib/vm/memory.h>
#include <emex64lib/vm/machine.h>

#include <emex64lib/vm/device/interrupt.h>
#include <emex64lib/vm/device/timer.h>

#include <emex64lib/vm/instruction/core.h>
#include <emex64lib/vm/instruction/data.h>
#include <emex64lib/vm/instruction/alu.h>
#include <emex64lib/vm/instruction/ctrl.h>

#include <emex64lib/bitwalker.h>

#if defined(__APPLE__)
#include <CoreFoundation/CFRunLoop.h>
#endif /* __APPLE__ */

emex64_opfunc_entry_t opfunc_table[] = {
    /* core operations */
    [LA64_OPCODE_HLT] = { .func = la64_op_hlt, .maxargs = 0 },
    [LA64_OPCODE_NOP] = { .func = la64_op_nop, .maxargs = 0 },

    /* data operations */
    [LA64_OPCODE_MOV] = { .func = la64_op_mov, .maxargs = 2 },
    [LA64_OPCODE_SWP] = { .func = la64_op_swp, .maxargs = 2 },
    [LA64_OPCODE_SWPZ] = { .func = la64_op_swpz, .maxargs = 2 },
    [LA64_OPCODE_PUSH] = { .func = la64_op_push, .maxargs = 32 },
    [LA64_OPCODE_POP] = { .func = la64_op_pop, .maxargs = 32 },
    [LA64_OPCODE_LDB] = { .func = la64_op_ldb, .maxargs = 2 },
    [LA64_OPCODE_LDW] = { .func = la64_op_ldw, .maxargs = 2 },
    [LA64_OPCODE_LDD] = { .func = la64_op_ldd, .maxargs = 2 },
    [LA64_OPCODE_LDQ] = { .func = la64_op_ldq, .maxargs = 2 },
    [LA64_OPCODE_STB] = { .func = la64_op_stb, .maxargs = 2 },
    [LA64_OPCODE_STW] = { .func = la64_op_stw, .maxargs = 2 },
    [LA64_OPCODE_STD] = { .func = la64_op_std, .maxargs = 2 },
    [LA64_OPCODE_STQ] = { .func = la64_op_stq, .maxargs = 2 },

    /* arithmetic operations */
    [LA64_OPCODE_ADD] = { .func = la64_op_add, .maxargs = 3 },
    [LA64_OPCODE_SUB] = { .func = la64_op_sub, .maxargs = 3 },
    [LA64_OPCODE_MUL] = { .func = la64_op_mul, .maxargs = 3 },
    [LA64_OPCODE_DIV] = { .func = la64_op_div, .maxargs = 3 },
    [LA64_OPCODE_IDIV] = { .func = la64_op_idiv, .maxargs = 3 },
    [LA64_OPCODE_MOD] = { .func = la64_op_mod, .maxargs = 3 },
    [LA64_OPCODE_NOT] = { .func = la64_op_not, .maxargs = 32 },
    [LA64_OPCODE_NEG] = { .func = la64_op_neg, .maxargs = 32 },
    [LA64_OPCODE_AND] = { .func = la64_op_and, .maxargs = 3 },
    [LA64_OPCODE_OR] = { .func = la64_op_or, .maxargs = 3 },
    [LA64_OPCODE_XOR] = { .func = la64_op_xor, .maxargs = 3 },
    [LA64_OPCODE_SHR] = { .func = la64_op_shr, .maxargs = 3 },
    [LA64_OPCODE_SHL] = { .func = la64_op_shl, .maxargs = 3 },
    [LA64_OPCODE_SAR] = { .func = la64_op_sar, .maxargs = 3 },
    [LA64_OPCODE_ROR] = { .func = la64_op_ror, .maxargs = 3 },
    [LA64_OPCODE_ROL] = { .func = la64_op_rol, .maxargs = 3 },
    [LA64_OPCODE_PDEP] = { .func = la64_op_pdep, .maxargs = 3 },
    [LA64_OPCODE_PEXT] = { .func = la64_op_pext, .maxargs = 3 },
    [LA64_OPCODE_BSWAPW] = { .func = la64_op_bswapw, .maxargs = 1 },
    [LA64_OPCODE_BSWAPD] = { .func = la64_op_bswapd, .maxargs = 1 },
    [LA64_OPCODE_BSWAPQ] = { .func = la64_op_bswapq, .maxargs = 1 },

    /* control flow operations */
    [LA64_OPCODE_B] = { .func = la64_op_b, .maxargs = 1 },
    [LA64_OPCODE_CMP] = { .func = la64_op_cmp, .maxargs = 2 },
    [LA64_OPCODE_BE] = { .func = la64_op_be, .maxargs = 1 },
    [LA64_OPCODE_BNE] = { .func = la64_op_bne, .maxargs = 1 },
    [LA64_OPCODE_BLT] = { .func = la64_op_blt, .maxargs = 1 },
    [LA64_OPCODE_BGT] = { .func = la64_op_bgt, .maxargs = 1 },
    [LA64_OPCODE_BLE] = { .func = la64_op_ble, .maxargs = 1 },
    [LA64_OPCODE_BGE] = { .func = la64_op_bge, .maxargs = 1 },
    [LA64_OPCODE_BZ] = { .func = la64_op_bz, .maxargs = 2 },
    [LA64_OPCODE_BNZ] = { .func = la64_op_bnz, .maxargs = 2 },
    [LA64_OPCODE_BL] = { .func = la64_op_bl, .maxargs = 32 },
    [LA64_OPCODE_RET] = { .func = la64_op_ret, .maxargs = 0 },
    [LA64_OPCODE_IRET] = { .func = la64_op_iret, .maxargs = 0 },
};

la64_core_t *la64_core_alloc()
{
    /* allocate a brand new core */
    la64_core_t *core = malloc(sizeof(la64_core_t));

    if(core == NULL)
    {
        return NULL;
    }

    bzero(core, sizeof(la64_core_t));

    return core;
}

void la64_core_dealloc(la64_core_t *core)
{
    /* release core */
    free(core);
}

static void la64_core_decode_instruction_at_pc(la64_core_t *core)
{
    /* reset operation structure */
    memset(&(core->op), 0, sizeof(la64_operation_t));

    /* accessing memory */
    void *iptr = la64_memory_access(core, core->rl[LA64_REGISTER_PC], 100);
    if(iptr == NULL)
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }

    /* preparing bitwalker */
    bitwalker_t bw;
    bitwalker_init_read(&bw, iptr, 256, BW_LITTLE_ENDIAN);

    /* getting opcode */
    core->op.opcode = (uint8_t)bitwalker_read(&bw, 8);
    if(core->op.opcode > LA64_OPCODE_MAX)
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }

    core->op.op = opfunc_table[core->op.opcode];

    /* parsing loop */
    bool reached_end = false;
    for(uint8_t i = 0; i < core->op.op.maxargs && !reached_end; i++)
    {
        /* next mode */
        uint8_t mode = (uint8_t)bitwalker_read(&bw, 3);

        /* switch through modes */
        switch(mode)
        {
            case LA64_PARAMETER_CODING_INSTR_END:
                reached_end = true;
                break;
            case LA64_PARAMETER_CODING_REG:
            {
                uint8_t rcnt = (uint8_t)bitwalker_read(&bw, 5);

                if(rcnt > LA64_REGISTER_RR &&
                   core->rl[LA64_REGISTER_CR0] < LA64_ELEVATION_KERNEL)
                {
                    core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_INSTRUCTION;
                    return;
                }

                core->op.param[core->op.param_cnt] = &(core->rl[rcnt]);
                core->op.param_cnt++;

                break;
            }
            case LA64_PARAMETER_CODING_IMM8:
            case LA64_PARAMETER_CODING_IMM16:
            case LA64_PARAMETER_CODING_IMM32:
            case LA64_PARAMETER_CODING_IMM64:
            {
                uint8_t bits = 1u << (((mode - LA64_PARAMETER_CODING_IMM8) + 1) + 2);
                core->op.imm[core->op.param_cnt] = (uint64_t)bitwalker_read(&bw, bits);
                core->op.param[core->op.param_cnt] = &(core->op.imm[core->op.param_cnt]);
                core->op.param_cnt++;
                break;
            }
            default:
                core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_INSTRUCTION;
                reached_end = true;
                return;
        }
    }

    /* finding out how many steps the the program counter has to jump */
    core->op.ilen = bitwalker_bytes_used(&bw);

    return;
}

static void *la64_core_execute_thread(void *arg)
{
    /* null pointer check */
    if(arg == NULL)
    {
        return NULL;
    }

    /* cast argument to core */
    la64_core_t *core = arg;

    /* going into da execution loop */
    while(1)
    {
        if(!core->in_interrupt)
        {
            /* checking if exception is non-NONE */
            if(core->rl[LA64_REGISTER_CR2] != LA64_EXCEPTION_NONE)
            {
                core->halted = true;
                la64_raise_interrupt(core->machine, LA64_IRQ_EXCEPTION);
            }
            
             /* checking if core is halted */
            if(core->halted)
            {
                /* yield cpu to not burn it */
                usleep(100);
                goto skip_execution;
            }
        }

        /* decoding instruction */
        la64_core_decode_instruction_at_pc(core);

        /* sanity check */
        if((core->rl[LA64_REGISTER_CR2] != LA64_EXCEPTION_NONE) &&
           !core->in_interrupt)
        {
            core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_INSTRUCTION;
            continue;
        }

        /* executing instruction */
        core->op.op.func(core);

        /* incrementing program counter by instruction size */
        core->rl[LA64_REGISTER_PC] += core->op.ilen;

        /*
         * if we are in a interrupt then there is no reason
         * to check if we shall serve another interrupt.
         * if we dont check if the instruction executes was
         * the return from interrupt controller then there is
         * a potential for a hardware occuring TOCTOU vulnerability,
         * because we would just immediately interrupt into another
         * interrupt handler in the interrupt vector table.
         */
        if(core->in_interrupt ||
           core->op.opcode == LA64_OPCODE_IRET)
        {
            goto tick_timer;
        }

        /* interrupt controller checking routine starts here */
skip_execution:

        /* serve interrupt for the interrupt controller */
        la64_serve_interrupt_if_needed(core);

        /* tick the timer always */
    tick_timer:
        {
            la64_timer_tick(core->machine->timer, la64_get_host_cycles());
        }
    }

    return NULL;
}


void la64_core_execute(la64_core_t *core)
{
    /* sanity check */
    if(core == NULL ||
       core->pthread != 0)
    {
        return;
    }

    /* invoking execution */
    pthread_create(&(core->pthread), NULL, la64_core_execute_thread, (void*)core);

#if defined(__APPLE__)
    CFRunLoopRun();
#endif /* __APPLE__ */
    
    pthread_join(core->pthread, NULL);
}

void la64_core_terminate(la64_core_t *core)
{
    /* sanity check */
    if(core == NULL ||
       core->pthread == 0)
    {
        return;
    }
    
    if(pthread_self() == core->pthread)
    {
        pthread_exit(NULL);
    }
    
    pthread_cancel(core->pthread);
}
