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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

#include <emex64lib/support/parser.h>
#include <emex64lib/support/fdwalker.h>

#include <emex64lib/asm/invocation.h>
#include <emex64lib/asm/diag.h>
#include <emex64lib/asm/opcode.h>
#include <emex64lib/asm/register.h>
#include <emex64lib/asm/label.h>

void assembler_emit_opcode(assembler_invocation_t *inv,
                           uint8_t op)
{
    fdwalker_write(inv->fdwalker, op, 8);
}

void assembler_emit_register(assembler_invocation_t *inv,
                             uint8_t reg)
{
    assert(reg < LA64_REGISTER_MAX);

    fdwalker_write(inv->fdwalker, LA64_PARAMETER_CODING_REG, 3);
    fdwalker_write(inv->fdwalker, reg, 5);
}

void assembler_emit_imm8(assembler_invocation_t *inv,
                         uint8_t imm)
{
    fdwalker_write(inv->fdwalker, LA64_PARAMETER_CODING_IMM8, 3);
    fdwalker_write(inv->fdwalker, imm, 8);
}

void assembler_emit_imm16(assembler_invocation_t *inv,
                          uint16_t imm)
{
    fdwalker_write(inv->fdwalker, LA64_PARAMETER_CODING_IMM16, 3);
    fdwalker_write(inv->fdwalker, imm, 16);
}

void assembler_emit_imm32(assembler_invocation_t *inv,
                          uint32_t imm)
{
    fdwalker_write(inv->fdwalker, LA64_PARAMETER_CODING_IMM32, 3);
    fdwalker_write(inv->fdwalker, imm, 32);
}

void assembler_emit_imm64(assembler_invocation_t *inv,
                          uint64_t imm)
{
    fdwalker_write(inv->fdwalker, LA64_PARAMETER_CODING_IMM64, 3);
    fdwalker_write(inv->fdwalker, imm, 64);
}

void assembler_emit_imm(assembler_invocation_t *inv,
                        uint64_t imm)
{
    if(imm <= 0xFF)
    {
        assembler_emit_imm8(inv, (uint8_t)imm);
    }
    else if(imm <= 0xFFFF)
    {
        assembler_emit_imm16(inv, (uint16_t)imm);
    }
    else if(imm <= 0xFFFFFFFF)
    {
        assembler_emit_imm32(inv, (uint32_t)imm);
    }
    else if(imm <= 0xFFFFFFFFFFFFFFFF)
    {
        assembler_emit_imm64(inv, (uint64_t)imm);
    }
}

void assembler_emit_end(assembler_invocation_t *inv)
{
    fdwalker_write(inv->fdwalker, LA64_PARAMETER_CODING_INSTR_END, 3);
}

bool assembler_emit_instruction_inc(const opcode_entry_t *opce,
                                    compiler_line_t *cl)
{
    /*
     * background is this was a native instruction, but
     * was removed, because it was redundant, many
     * la64 programms tho use inc and thats why we create
     * a emit path for it that emits increment using
     * the addition opcode, it costs the same, just
     * 1 byte more for the end marker, plus.. nobody
     * actually used the multiargument feature of it.
     */
    
    for(uint64_t i = 1; i < cl->token_cnt; i++)
    {
        /* increment means each parameter, one opcode */
        assembler_emit_opcode(cl->ci, LA64_OPCODE_ADD);

        /* it must be a register */
        register_entry_t *reg = register_from_string(cl->token[i].str);
        if(reg == NULL)
        {
            diag_error(&(cl->token[i]), "expected register, got intermediate or label \"%s\"\n", cl->token[i].str);
            return false;
        }

        /* emit parameters */
        assembler_emit_register(cl->ci, reg->reg);
        assembler_emit_imm8(cl->ci, 1);
        assembler_emit_end(cl->ci);
        fdwalker_align_byte(cl->ci->fdwalker);
    }

    return true;
}

bool assembler_emit_instruction_dec(const opcode_entry_t *opce,
                                    compiler_line_t *cl)
{
    /*
     * background is this was a native instruction, but
     * was removed, because it was redundant, many
     * la64 programms tho use dec and thats why we create
     * a emit path for it that emits decrement using
     * the subtraction opcode, it costs the same, just
     * 1 byte more for the end marker, plus.. nobody
     * actually used the multiargument feature of it.
     */
    for(uint64_t i = 1; i < cl->token_cnt; i++)
    {
        /* increment means each parameter, one opcode */
        assembler_emit_opcode(cl->ci, LA64_OPCODE_SUB);

        /* it must be a register */
        register_entry_t *reg = register_from_string(cl->token[i].str);

        if(reg == NULL)
        {
            diag_error(&(cl->token[i]), "expected register, got intermediate or label \"%s\"\n", cl->token[i].str);
            return false;
        }

        /* emit parameters */
        assembler_emit_register(cl->ci, reg->reg);
        assembler_emit_imm8(cl->ci, 1);
        assembler_emit_end(cl->ci);
        fdwalker_align_byte(cl->ci->fdwalker);
    }

    return true;
}

bool assembler_emit_instruction_clr(const opcode_entry_t *opce,
                                    compiler_line_t *cl)
{
    /*
     * people would argue to emit XOR but XOR 
     * has a end marker while MOV doesnt, so
     * using MOV with 0 Is better for code density
     * than using XOR REG, REG... basically we will
     * emit...
     *
     * MOV REG, 0
     *
     * over
     *
     * XOR REG, REG, END
     *
     */
    for(uint64_t i = 1; i < cl->token_cnt; i++)
    {
        /* increment means each parameter, one opcode */
        assembler_emit_opcode(cl->ci, LA64_OPCODE_MOV);

        /* it must be a register */
        register_entry_t *reg = register_from_string(cl->token[i].str);

        if(reg == NULL)
        {
            diag_error(&(cl->token[i]), "expected register, got intermediate or label \"%s\"\n", cl->token[i].str);
            return false;
        }

        /* emit parameters */
        assembler_emit_register(cl->ci, reg->reg);
        assembler_emit_imm8(cl->ci, 0);
        fdwalker_align_byte(cl->ci->fdwalker);
    }

    return true;
}

bool assembler_emit_instruction_generic(const opcode_entry_t *opce,
                                        compiler_line_t *cl)
{
    /*
     * every instruction starts with a
     * opcode. so we emit one.
     */
    assembler_emit_opcode(cl->ci, opce->opcode);

    for(uint64_t i = 1; i < cl->token_cnt; i++)
    {
        register_entry_t *reg = register_from_string(cl->token[i].str);
        if(reg != NULL)
        {
            /* registers are always allowed so far */
            assembler_emit_register(cl->ci, reg->reg);
            continue;
        }

        /* checking if allowed to be something else than a register */
        if(opcode_arg_accepts_reg_only(opce,  i - 1))
        {
            diag_error(&(cl->token[i]), "expected register, got intermediate or label \"%s\"\n", cl->token[i].str);
            return false;
        }

        /*
         * parsing value
         *
         * note: if its a string then it is a 64bit
         *       label. 64bit defaulted because we
         *       need to ensure early compatibility
         *       with the new object file format we
         *       are going to use later on, so the
         *       relocations work perfectly fine.
         */
        parser_return_t pr = parse_value_from_string(cl->token[i].str);

        if(pr.type == emexParserValueTypeString)
        {
            fdwalker_write(cl->ci->fdwalker, LA64_PARAMETER_CODING_IMM64, 3);

            /* the label is either local or global */
            char *label = NULL;
            if(cl->token[i].str[0] == '.')
            {
                asprintf(&label, "%s%s", cl->ci->label_scope, cl->token[i].str);
            }
            else
            {
                label = strdup(cl->token[i].str);
            }

            /*
             * append label callsite to relocation
             * table.
             */
            reloc_table_entry_t *rtbe = malloc(sizeof(reloc_table_entry_t));
            if(cl->ci->rtbe != NULL)
            {
                rtbe->next = cl->ci->rtbe;
            }
            cl->ci->rtbe = rtbe;

            rtbe->name = label;
            rtbe->byte_pos = cl->ci->fdwalker->byte_pos;
            rtbe->bit_idx = cl->ci->fdwalker->bit_idx;
            rtbe->ctlink = &(cl->token[i]);

            /*
             * skip the 64bit the label occupies
             * as we added it to the relocation table
             * already. the relocation table later will
             * fill this space with the address.
             */
            fdwalker_skip(cl->ci->fdwalker, 64);
        }
        else
        {
            /* its a intermediate */
            assembler_emit_imm(cl->ci, pr.value);
        }
    }

    if(opce->maxargs == 32 || opce->maxargs != (cl->token_cnt - 1))
    {
        assembler_emit_end(cl->ci);
    }

    fdwalker_align_byte(cl->ci->fdwalker);

    return true;
}

bool assembler_emit_line(compiler_line_t *cl)
{
    /* parameter count check */
    if(cl->token_cnt <= 0)
    {
        diag_error(&(cl->token[0]), "insufficient operands\n");
        return false;
    }
    else if(cl->token_cnt > 32)
    {
        diag_error(&(cl->token[0]), "holy smokes, why soo many operands, maximum is 32 operands in 64bit lightweight architecture\n");
        return false;
    }

    /* getting opcode entry if it exists */
    const opcode_entry_t *opce = opcode_from_string(cl->token[0].str);
    if(opce == NULL)
    {
        diag_error(&(cl->token[0]), "illegal opcode \"%s\"\n", cl->token[0].str);
        return false;
    }

    /* checking for deprecation */
    if(opce->dnstr != NULL && cl->ci->options.warning_deprecated)
    {
        diag_warn(&(cl->token[cl->token_cnt - 1]), "opcode \"%s\" is deprecated: %s\n", opce->name, opce->dnstr);
    }

    /* checking argument count */
    if((cl->token_cnt - 1) > opce->maxargs)
    {
        diag_error(&(cl->token[cl->token_cnt - 1]), "too many operands for opcode \"%s\", expected %d operands, but got %d operands\n", opce->name, opce->maxargs, cl->token_cnt - 1);
        return false;
    }
    else if((cl->token_cnt - 1) < opce->minargs)
    {
        diag_error(&(cl->token[cl->token_cnt - 1]), "too few operands for opcode \"%s\", expected %d operands, but got %d operands\n", opce->name, opce->minargs, cl->token_cnt - 1);
        return false;
    }

    assert(opce->handler != NULL);

    return opce->handler(opce, cl);
}

bool assembler_emit(assembler_invocation_t *inv)
{
    /* iterate through each token */
    for(uint64_t i = 0; i < inv->line_cnt; i++)
    {
        /* checking for label */
        if(inv->line[i].type == ASSEMBLER_LINE_TYPE_GLOBAL_LABEL ||
           inv->line[i].type == ASSEMBLER_LINE_TYPE_LOCAL_LABEL)
        {
            /* insert into labels */
            assembler_label_append(&(inv->line[i].token[0]));
        }
        else if(inv->line[i].type == ASSEMBLER_LINE_TYPE_ASM)
        {
            if(!assembler_emit_line(&(inv->line[i])))
            {
                return false;
            }
        }
    }

    /*
     * appending binary end label, which is a compiler
     * constant.
     */
    struct stat st;
    if(fstat(inv->fdwalker->fd, &st) == -1)
    {
        diag_error(NULL, "fatal error occured, pls report\n");
    }

    inv->label[inv->label_cnt].addr = st.st_size;
    inv->label[inv->label_cnt++].name = strdup("__la64_exec_img_end");

    /*
     * the main code emitter appended labels the code
     * requires to the relocation table, so we have to
     * look each request up in the label lookup table
     * and insert each label at the place where a label
     * shall be.
     */
    reloc_table_entry_t *rtbe = inv->rtbe;
    while(rtbe != NULL)
    {
        compiler_label_t *label = assembler_label_lookup(inv, rtbe->name);
        if(label == NULL)
        {
            diag_error(rtbe->ctlink, "label \"%s\" not found\n", rtbe->name);
            return false;
        }

        fdwalker_seek(inv->fdwalker, rtbe->byte_pos, rtbe->bit_idx);
        fdwalker_write(inv->fdwalker, label->addr, 64);

        rtbe = rtbe->next;
    }

    fsync(inv->fdwalker->fd);

    return true;
}
