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
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

#include <emex64lib/support/parser.h>
#include <emex64lib/support/fdwalker.h>
#include <emex64lib/support/diag.h>

#include <emex64lib/asm/invocation.h>
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
    assert(reg <= kEmex64RegisterMAX);

    fdwalker_write(inv->fdwalker, kEmex64ParameterCodingReg, 3);
    fdwalker_write(inv->fdwalker, reg, 5);
}

void assembler_emit_imm5(assembler_invocation_t *inv,
                         uint8_t imm)
{
    fdwalker_write(inv->fdwalker, kEmex64ParameterCodingImm5, 3);
    fdwalker_write(inv->fdwalker, imm, 5);
}

void assembler_emit_imm8(assembler_invocation_t *inv,
                         uint8_t imm)
{
    fdwalker_write(inv->fdwalker, kEmex64ParameterCodingImm8, 3);
    fdwalker_write(inv->fdwalker, imm, 8);
}

void assembler_emit_imm16(assembler_invocation_t *inv,
                          uint16_t imm)
{
    fdwalker_write(inv->fdwalker, kEmex64ParameterCodingImm16, 3);
    fdwalker_write(inv->fdwalker, imm, 16);
}

void assembler_emit_imm32(assembler_invocation_t *inv,
                          uint32_t imm)
{
    fdwalker_write(inv->fdwalker, kEmex64ParameterCodingImm32, 3);
    fdwalker_write(inv->fdwalker, imm, 32);
}

void assembler_emit_imm64(assembler_invocation_t *inv,
                          uint64_t imm)
{
    fdwalker_write(inv->fdwalker, kEmex64ParameterCodingImm64, 3);
    fdwalker_write(inv->fdwalker, imm, 64);
}

void assembler_emit_addr64(assembler_invocation_t *inv,
                           uint64_t addr)
{
    fdwalker_write(inv->fdwalker, kEmex64ParameterCodingAddr64, 3);
    fdwalker_align_byte(inv->fdwalker);
    fdwalker_write(inv->fdwalker, addr, 64);
}

void assembler_emit_imm(assembler_invocation_t *inv,
                        uint64_t imm)
{
    if(imm <= 0x1F)
    {
        assembler_emit_imm5(inv, (uint8_t)imm);
    }
    else if(imm <= 0xFF)
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
    fdwalker_write(inv->fdwalker, kEmex64ParameterCodingEnd, 3);
}

bool assembler_emit_instruction(assembler_line_t *al)
{
    const opcode_entry_t *opce = opcode_from_string(al->token[0]->str);
    if(opce == NULL)
    {
        diag_error(al->token[0], "illegal opcode \"%s\"\n", al->token[0]->str);
        return false;
    }

    /* sanity checking all parameter count related things */
    if(al->token_cnt <= 0)
    {
        diag_error(al->token[0], "insufficient operands\n");
        return false;
    }
    else if(al->token_cnt > EMEX64_MAX_ARGS)
    {
        diag_error(al->token[0], "holy smokes, why soo many operands, maximum is %d operands in emex64\n", EMEX64_MAX_ARGS);
        return false;
    }
    if((al->token_cnt - 1) > opce->maxargs)
    {
        diag_error(al->token[al->token_cnt - 1], "too many operands for opcode \"%s\", expected %d operands, but got %d operands\n", al->token[0]->str, opce->maxargs, al->token_cnt - 1);
        return false;
    }
    else if((al->token_cnt - 1) < opce->minargs)
    {
        diag_error(al->token[al->token_cnt - 1], "too few operands for opcode \"%s\", expected %d operands, but got %d operands\n", al->token[0]->str, opce->minargs, al->token_cnt - 1);
        return false;
    }

    /* emitting the instruction */
    assembler_emit_opcode(al->inv, opce->opcode);

    for(uint64_t i = 1; i < al->token_cnt; i++)
    {
        register_entry_t *reg = register_from_string(al->token[i]->str);
        if(reg != NULL)
        {
            /* registers are always allowed so far */
            assembler_emit_register(al->inv, reg->reg);
            continue;
        }

        /* checking if allowed to be something else than a register */
        if(opcode_arg_accepts_reg_only(opce,  i - 1))
        {
            diag_error(al->token[i], "expected register, got intermediate or label \"%s\"\n", al->token[i]->str);
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
        parser_return_t pr = parse_value_from_string(al->token[i]->str);
        if(pr.type == emexParserValueTypeOverflow)
        {
            diag_error(al->token[i], "integer literal '%s' overflows 64bit lenght\n", al->token[i]->str);
            return false;
        }
        else if(pr.type == emexParserValueTypeString)
        {
            /* the label is either local or global */
            char *label = NULL;
            if(al->token[i]->str[0] == '.')
            {
                asprintf(&label, "%s%s", al->inv->label_scope, al->token[i]->str);
            }
            else
            {
                label = strdup(al->token[i]->str);
            }

            fdwalker_write(al->inv->fdwalker, kEmex64ParameterCodingAddr64, 3);
            fdwalker_align_byte(al->inv->fdwalker);

            /*
             * append label callsite to relocation
             * table.
             */
            reloc_table_entry_t *rtbe = calloc(1, sizeof(reloc_table_entry_t));
            if(al->inv->rtbe != NULL)
            {
                rtbe->next = al->inv->rtbe;
            }
            al->inv->rtbe = rtbe;
            rtbe->name = label;
            rtbe->byte_pos = al->inv->fdwalker->byte_pos;
            rtbe->at_link = al->token[i];

            /*
             * skip the 64bit the label occupies
             * as we added it to the relocation table
             * already. the relocation table later will
             * fill this space with the address.
             */
            fdwalker_skip(al->inv->fdwalker, 64);
        }
        else
        {
            /* branches work different, they have offset branching */
            if((i == 1 && (opce->opcode == kEmex64OpcodeB   || opce->opcode == kEmex64OpcodeBE  || opce->opcode == kEmex64OpcodeBNE   ||
                           opce->opcode == kEmex64OpcodeBLE || opce->opcode == kEmex64OpcodeBGE || opce->opcode == kEmex64OpcodeBLT   ||
                           opce->opcode == kEmex64OpcodeBGT || opce->opcode == kEmex64OpcodeBLW || opce->opcode == kEmex64OpcodeBLE)) ||
               (i == 2 && (opce->opcode == kEmex64OpcodeBZ  || opce->opcode == kEmex64OpcodeBNZ)))
            {
                assembler_emit_addr64(al->inv, pr.value);
            }
            else
            {
                /* its a intermediate */
                assembler_emit_imm(al->inv, pr.value);
            }
        }
    }

    if(opce->maxargs != (al->token_cnt - 1))
    {
        assembler_emit_end(al->inv);
    }

    fdwalker_align_byte(al->inv->fdwalker);

    return true;
}

bool assembler_emit(assembler_invocation_t *inv)
{
    /* iterate through each token */
    for(uint64_t i = 0; i < inv->line_cnt; i++)
    {
        /* checking for label */
        if(inv->line[i]->type == kAssemblerLineTypeGlobalLabel ||
           inv->line[i]->type == kAssemblerLineTypeLocalLabel)
        {
            /* insert into labels */
            if(!assembler_label_append(inv->line[i]->token[0]))
            {
                return false;
            }
        }
        else if(inv->line[i]->type == kAssemblerLineTypeAssembly)
        {
            if(!assembler_emit_instruction(inv->line[i]))
            {
                return false;
            }
        }
    }

    /* TODO: fix this so static vs exported symbols work */
    /*reloc_table_entry_t *rtbe = inv->rtbe;
    while(rtbe != NULL)
    {
        assembler_label_t *label = assembler_label_lookup(inv, rtbe->name);
        if(label == NULL)
        {
            rtbe = rtbe->next;
            continue;
        }

        fdwalker_seek(inv->fdwalker, rtbe->byte_pos, 0);
        fdwalker_write(inv->fdwalker, label->addr, 64);

        rtbe = rtbe->next;
    }*/

    fsync(inv->fdwalker->fd);

    return true;
}
