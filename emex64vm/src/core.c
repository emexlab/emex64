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

#include <emex64vm/core.h>
#include <emex64vm/memory.h>
#include <emex64vm/machine.h>

#include <emex64vm/device/interrupt.h>
#include <emex64vm/device/timer.h>

#include <emex64vm/instruction/core.h>
#include <emex64vm/instruction/data.h>
#include <emex64vm/instruction/alu.h>
#include <emex64vm/instruction/ctrl.h>

#include <emexutils/bitwalker.h>

#if defined(__APPLE__)
#include <CoreFoundation/CFRunLoop.h>
#endif /* __APPLE__ */

la64_opfunc_t opfunc_table[] = {
    /* core operations */
    [LA64_OPCODE_HLT] = la64_op_hlt,
    [LA64_OPCODE_NOP] = la64_op_nop,

    /* data operations */
    [LA64_OPCODE_MOV] = la64_op_mov,
    [LA64_OPCODE_SWP] = la64_op_swp,
    [LA64_OPCODE_SWPZ] = la64_op_swpz,
    [LA64_OPCODE_PUSH] = la64_op_push,
    [LA64_OPCODE_POP] = la64_op_pop,
    [LA64_OPCODE_LDB] = la64_op_ldb,
    [LA64_OPCODE_LDW] = la64_op_ldw,
    [LA64_OPCODE_LDD] = la64_op_ldd,
    [LA64_OPCODE_LDQ] = la64_op_ldq,
    [LA64_OPCODE_STB] = la64_op_stb,
    [LA64_OPCODE_STW] = la64_op_stw,
    [LA64_OPCODE_STD] = la64_op_std,
    [LA64_OPCODE_STQ] = la64_op_stq,

    /* arithmetic operations */
    [LA64_OPCODE_ADD] = la64_op_add,
    [LA64_OPCODE_SUB] = la64_op_sub,
    [LA64_OPCODE_MUL] = la64_op_mul,
    [LA64_OPCODE_DIV] = la64_op_div,
    [LA64_OPCODE_IDIV] = la64_op_idiv,
    [LA64_OPCODE_MOD] = la64_op_mod,
    [LA64_OPCODE_NOT] = la64_op_not,
    [LA64_OPCODE_NEG] = la64_op_neg,
    [LA64_OPCODE_AND] = la64_op_and,
    [LA64_OPCODE_OR] = la64_op_or,
    [LA64_OPCODE_XOR] = la64_op_xor,
    [LA64_OPCODE_SHR] = la64_op_shr,
    [LA64_OPCODE_SHL] = la64_op_shl,
    [LA64_OPCODE_SAR] = la64_op_sar,
    [LA64_OPCODE_ROR] = la64_op_ror,
    [LA64_OPCODE_ROL] = la64_op_rol,
    [LA64_OPCODE_PDEP] = la64_op_pdep,
    [LA64_OPCODE_PEXT] = la64_op_pext,
    [LA64_OPCODE_BSWAPW] = la64_op_bswapw,
    [LA64_OPCODE_BSWAPD] = la64_op_bswapd,
    [LA64_OPCODE_BSWAPQ] = la64_op_bswapq,

    /* control flow operations */
    [LA64_OPCODE_B] = la64_op_b,
    [LA64_OPCODE_CMP] = la64_op_cmp,
    [LA64_OPCODE_BE] = la64_op_be,
    [LA64_OPCODE_BNE] = la64_op_bne,
    [LA64_OPCODE_BLT] = la64_op_blt,
    [LA64_OPCODE_BGT] = la64_op_bgt,
    [LA64_OPCODE_BLE] = la64_op_ble,
    [LA64_OPCODE_BGE] = la64_op_bge,
    [LA64_OPCODE_BZ] = la64_op_bz,
    [LA64_OPCODE_BNZ] = la64_op_bnz,
    [LA64_OPCODE_BL] = la64_op_bl,
    [LA64_OPCODE_RET] = la64_op_ret,
    [LA64_OPCODE_IRET] = la64_op_iret
};

static const uint8_t opcode_maxargs[256] = {
    [LA64_OPCODE_HLT] = 0,
    [LA64_OPCODE_NOP] = 0,

    [LA64_OPCODE_MOV] = 2,
    [LA64_OPCODE_SWP] = 2,
    [LA64_OPCODE_SWPZ] = 2,
    [LA64_OPCODE_PUSH] = 32,
    [LA64_OPCODE_POP] = 32,
    [LA64_OPCODE_LDB] = 2,
    [LA64_OPCODE_LDW] = 2,
    [LA64_OPCODE_LDD] = 2,
    [LA64_OPCODE_LDQ] = 2,
    [LA64_OPCODE_STB] = 2,
    [LA64_OPCODE_STW] = 2,
    [LA64_OPCODE_STD] = 2,
    [LA64_OPCODE_STQ] = 2,

    [LA64_OPCODE_ADD] = 3,
    [LA64_OPCODE_SUB] = 3,
    [LA64_OPCODE_MUL] = 3,
    [LA64_OPCODE_DIV] = 3,
    [LA64_OPCODE_IDIV] = 3,
    [LA64_OPCODE_MOD] = 3,
    [LA64_OPCODE_NOT] = 32,
    [LA64_OPCODE_NEG] = 32,
    [LA64_OPCODE_AND] = 3,
    [LA64_OPCODE_OR] = 3,
    [LA64_OPCODE_XOR] = 3,
    [LA64_OPCODE_SHR] = 3,
    [LA64_OPCODE_SHL] = 3,
    [LA64_OPCODE_SAR] = 3,
    [LA64_OPCODE_ROR] = 3,
    [LA64_OPCODE_ROL] = 3,
    [LA64_OPCODE_PDEP] = 3,
    [LA64_OPCODE_PEXT] = 3,
    [LA64_OPCODE_BSWAPW] = 1,
    [LA64_OPCODE_BSWAPD] = 1,
    [LA64_OPCODE_BSWAPQ] = 1,

    [LA64_OPCODE_B] = 1,
    [LA64_OPCODE_CMP] = 2,
    [LA64_OPCODE_BE] = 1,
    [LA64_OPCODE_BNE] = 1,
    [LA64_OPCODE_BLT] = 1,
    [LA64_OPCODE_BGT] = 1,
    [LA64_OPCODE_BLE] = 1,
    [LA64_OPCODE_BGE] = 1,
    [LA64_OPCODE_BZ] = 2,
    [LA64_OPCODE_BNZ] = 2,
    [LA64_OPCODE_BL] = 32,
    [LA64_OPCODE_RET] = 0,
    [LA64_OPCODE_IRET] = 0,
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

    /* null pointer check */
    if(iptr == NULL)
    {
        core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_ACCESS;
        return;
    }

    /* preparing bitwalker */
    bitwalker_t bw;
    bitwalker_init_read(&bw, iptr, 256, BW_LITTLE_ENDIAN);

    /* getting opcode */
    core->op.op = (uint8_t)bitwalker_read(&bw, 8);

    uint8_t maxargs = opcode_maxargs[core->op.op];

    /* parsing loop */
    bool reached_end = false;
    for(uint8_t i = 0; i < maxargs && !reached_end; i++)
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
        if((core->rl[LA64_REGISTER_CR2] != LA64_EXCEPTION_NONE ||
            core->op.op > LA64_OPCODE_MAX ||
            opfunc_table[core->op.op] == NULL) &&
           !core->in_interrupt)
        {
            core->rl[LA64_REGISTER_CR2] = LA64_EXCEPTION_BAD_INSTRUCTION;
            continue;
        }

        /* executing instruction */
        opfunc_table[core->op.op](core);

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
           core->op.op == LA64_OPCODE_IRET)
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
