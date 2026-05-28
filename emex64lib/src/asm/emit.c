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

#include <emex64lib/parser.h>
#include <emex64lib/fdwalker.h>

#include <emex64lib/asm/compiler.h>
#include <emex64lib/asm/diag.h>
#include <emex64lib/asm/opcode.h>
#include <emex64lib/asm/register.h>
#include <emex64lib/asm/label.h>

void la64_compiler_emit_opcode(fdwalker_t *fw,
                               uint8_t op)
{
    fdwalker_write(fw, op, 8);
}

void la64_compiler_emit_reg(fdwalker_t *fw,
                            uint8_t reg)
{
    assert(reg < LA64_REGISTER_MAX);

    fdwalker_write(fw, LA64_PARAMETER_CODING_REG, 3);
    fdwalker_write(fw, reg, 5);
}

void la64_compiler_emit_imm8(fdwalker_t *fw,
                             uint8_t imm)
{
    fdwalker_write(fw, LA64_PARAMETER_CODING_IMM8, 3);
    fdwalker_write(fw, imm, 8);
}

void la64_compiler_emit_imm16(fdwalker_t *fw,
                              uint16_t imm)
{
    fdwalker_write(fw, LA64_PARAMETER_CODING_IMM16, 3);
    fdwalker_write(fw, imm, 16);
}

void la64_compiler_emit_imm32(fdwalker_t *fw,
                              uint32_t imm)
{
    fdwalker_write(fw, LA64_PARAMETER_CODING_IMM32, 3);
    fdwalker_write(fw, imm, 32);
}

void la64_compiler_emit_imm64(fdwalker_t *fw,
                              uint64_t imm)
{
    fdwalker_write(fw, LA64_PARAMETER_CODING_IMM64, 3);
    fdwalker_write(fw, imm, 64);
}

void la64_compiler_emit_imm(fdwalker_t *fw,
                            uint64_t imm)
{
    if(imm <= 0xFF)
    {
        la64_compiler_emit_imm8(fw, (uint8_t)imm);
    }
    else if(imm <= 0xFFFF)
    {
        la64_compiler_emit_imm16(fw, (uint16_t)imm);
    }
    else if(imm <= 0xFFFFFFFF)
    {
        la64_compiler_emit_imm32(fw, (uint32_t)imm);
    }
    else if(imm <= 0xFFFFFFFFFFFFFFFF)
    {
        la64_compiler_emit_imm64(fw, (uint64_t)imm);
    }
}

void la64_compiler_emit_end(fdwalker_t *fw)
{
    fdwalker_write(fw, LA64_PARAMETER_CODING_INSTR_END, 3);
}

bool la64_compiler_emit_instr_inc(opcode_entry_t *opce,
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
        la64_compiler_emit_opcode(cl->ci->fdwalker, LA64_OPCODE_ADD);

        /* it must be a register */
        register_entry_t *reg = register_from_string(cl->token[i].str);
        if(reg == NULL)
        {
            diag_error(&(cl->token[i]), "expected register, got intermediate or label \"%s\"\n", cl->token[i].str);
            return false;
        }

        /* emit parameters */
        la64_compiler_emit_reg(cl->ci->fdwalker, reg->reg);
        la64_compiler_emit_imm8(cl->ci->fdwalker, 1);
        la64_compiler_emit_end(cl->ci->fdwalker);
        fdwalker_align_byte(cl->ci->fdwalker);
    }

    return true;
}

bool la64_compiler_emit_instr_dec(opcode_entry_t *opce,
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
        la64_compiler_emit_opcode(cl->ci->fdwalker, LA64_OPCODE_SUB);

        /* it must be a register */
        register_entry_t *reg = register_from_string(cl->token[i].str);

        if(reg == NULL)
        {
            diag_error(&(cl->token[i]), "expected register, got intermediate or label \"%s\"\n", cl->token[i].str);
            return false;
        }

        /* emit parameters */
        la64_compiler_emit_reg(cl->ci->fdwalker, reg->reg);
        la64_compiler_emit_imm8(cl->ci->fdwalker, 1);
        la64_compiler_emit_end(cl->ci->fdwalker);
        fdwalker_align_byte(cl->ci->fdwalker);
    }

    return true;
}

bool la64_compiler_emit_instr_clr(opcode_entry_t *opce,
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
        la64_compiler_emit_opcode(cl->ci->fdwalker, LA64_OPCODE_MOV);

        /* it must be a register */
        register_entry_t *reg = register_from_string(cl->token[i].str);

        if(reg == NULL)
        {
            diag_error(&(cl->token[i]), "expected register, got intermediate or label \"%s\"\n", cl->token[i].str);
            return false;
        }

        /* emit parameters */
        la64_compiler_emit_reg(cl->ci->fdwalker, reg->reg);
        la64_compiler_emit_imm8(cl->ci->fdwalker, 0);
        fdwalker_align_byte(cl->ci->fdwalker);
    }

    return true;
}

bool la64_compiler_emit_instr_default(const opcode_entry_t *opce,
                                      compiler_line_t *cl)
{
    /*
     * every instruction starts with a
     * opcode. so we emit one.
     */
    la64_compiler_emit_opcode(cl->ci->fdwalker, opce->opcode);

    for(uint64_t i = 1; i < cl->token_cnt; i++)
    {
        register_entry_t *reg = register_from_string(cl->token[i].str);
        if(reg != NULL)
        {
            /* registers are always allowed so far */
            la64_compiler_emit_reg(cl->ci->fdwalker, reg->reg);
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
            la64_compiler_emit_imm(cl->ci->fdwalker, pr.value);
        }
    }

    if(opce->maxargs == 32 || opce->maxargs != (cl->token_cnt - 1))
    {
        la64_compiler_emit_end(cl->ci->fdwalker);
    }

    fdwalker_align_byte(cl->ci->fdwalker);

    return true;
}

bool la64_compiler_emit(compiler_line_t *cl)
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
    if(opce->dnstr != NULL && cl->ci->warning_deprecated)
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

bool la64_compiler_emit_all(compiler_invocation_t *ci)
{
    /* iterate through each token */
    for(uint64_t i = 0; i < ci->line_cnt; i++)
    {
        /* checking for label */
        if(ci->line[i].type == ASSEMBLER_LINE_TYPE_GLOBAL_LABEL ||
           ci->line[i].type == ASSEMBLER_LINE_TYPE_LOCAL_LABEL)
        {
            /* insert into labels */
            code_token_label_append(&(ci->line[i].token[0]));
        }
        else if(ci->line[i].type == ASSEMBLER_LINE_TYPE_ASM)
        {
            if(!la64_compiler_emit(&(ci->line[i])))
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
    if(fstat(ci->fdwalker->fd, &st) == -1)
    {
        diag_error(NULL, "fatal error occured, pls report\n");
    }

    ci->label[ci->label_cnt].addr = st.st_size;
    ci->label[ci->label_cnt++].name = strdup("__la64_exec_img_end");

    /*
     * the main code emitter appended labels the code
     * requires to the relocation table, so we have to
     * look each request up in the label lookup table
     * and insert each label at the place where a label
     * shall be.
     */
    reloc_table_entry_t *rtbe = ci->rtbe;
    while(rtbe != NULL)
    {
        compiler_label_t *label = label_lookup(ci, rtbe->name);
        if(label == NULL)
        {
            diag_error(rtbe->ctlink, "label \"%s\" not found\n", rtbe->name);
            return false;
        }

        fdwalker_seek(ci->fdwalker, rtbe->byte_pos, rtbe->bit_idx);
        fdwalker_write(ci->fdwalker, label->addr, 64);

        rtbe = rtbe->next;
    }

    fsync(ci->fdwalker->fd);

    return true;
}
