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
#include <emex64lib/support/diagnostic/legacy.h>

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
    bool success = false;
    enum kEmex64Opcode opcode = opcode_from_string(al->token[0]->str, &success);
    if(!success)
    {
        diag_error(al->token[0], "illegal opcode '%s'\n", al->token[0]->str);
        return false;
    }

    const emex64_opfunc_entry_t *entry = &kEmex64OpfuncTable[opcode];

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
    if((al->token_cnt - 1) > entry->maxargs)
    {
        diag_error(al->token[al->token_cnt - 1], "too many operands for opcode '%s', expected %d operands, but got %d operands\n", al->token[0]->str, entry->maxargs, al->token_cnt - 1);
        return false;
    }
    else if((al->token_cnt - 1) < entry->minargs)
    {
        diag_error(al->token[al->token_cnt - 1], "too few operands for opcode '%s', expected %d operands, but got %d operands\n", al->token[0]->str, entry->minargs, al->token_cnt - 1);
        return false;
    }

    /* emitting the instruction */
    assembler_emit_opcode(al->inv, opcode);

    for(uint64_t i = 1; i < al->token_cnt; i++)
    {
        bool success = false;
        enum kEmex64Register reg = register_from_string(al->token[i]->str, &success);
        if(success)
        {
            /* registers are always allowed so far */
            assembler_emit_register(al->inv, reg);
            continue;
        }

        /* checking if allowed to be something else than a register */
        if(opcode_arg_accepts_reg_only(entry,  i - 1))
        {
            diag_error(al->token[i], "expected register, got intermediate or label '%s'\n", al->token[i]->str);
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
            bool local;
            char *label = NULL;
            if(al->token[i]->str[0] == '.')
            {
                local = true;
                asprintf(&label, "%s%s", al->inv->label_scope, al->token[i]->str);
            }
            else
            {
                local = false;
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
            rtbe->local = local;

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
            if((i == 1 && (opcode == kEmex64OpcodeB   || opcode == kEmex64OpcodeBE  || opcode == kEmex64OpcodeBNE   ||
                           opcode == kEmex64OpcodeBLE || opcode == kEmex64OpcodeBGE || opcode == kEmex64OpcodeBLT   ||
                           opcode == kEmex64OpcodeBGT || opcode == kEmex64OpcodeBLW || opcode == kEmex64OpcodeBLE)) ||
               (i == 2 && (opcode == kEmex64OpcodeBZ  || opcode == kEmex64OpcodeBNZ)))
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

    if(entry->maxargs != (al->token_cnt - 1))
    {
        assembler_emit_end(al->inv);
    }

    fdwalker_align_byte(al->inv->fdwalker);

    return true;
}

bool assembler_emit(assembler_invocation_t *inv)
{
    bool failed = false;
    uint8_t errors = 0;

    /* iterate through each line */
    for(uint64_t i = 0; i < inv->line_cnt; i++)
    {
        switch(inv->line[i]->type)
        {
            case kAssemblerLineTypeGlobalLabel:
            case kAssemblerLineTypeLocalLabel:
            case kAssemblerLineTypeExternLabel:
                if(!assembler_label_append(inv->line[i]->token[0]))
                {
                    failed = true;
                    if(++errors >= 10)
                    {
                        diag_fatal(NULL, "too many errors emitted, stopping now\n");
                        return false;
                    }
                }
                break;
            case kAssemblerLineTypeAssembly:
                if(!assembler_emit_instruction(inv->line[i]))
                {
                    failed = true;
                    if(++errors >= 10)
                    {
                        diag_fatal(NULL, "too many errors emitted, stopping now\n");
                        return false;
                    }
                }
                break;
            default:
                break;
        }
    }

    /* check if all local labels exist */
    reloc_table_entry_t *reloc = inv->rtbe;
    while(reloc != NULL)
    {
        assembler_label_t *label = assembler_label_lookup(inv, reloc->name);
        if(label == NULL)
        {
            /* local labels must be resolved at assembly time */
            diag_error(reloc->at_link, "label '%s' is undefined\n", reloc->name);
            return false;
        }

        /* travel down the list */
        reloc = reloc->next;
    }

    fdwalker_sync(inv->fdwalker);

    return !failed;
}
