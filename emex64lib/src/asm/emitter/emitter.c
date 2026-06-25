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
#include <emex64lib/support/virtual/vbitwalker.h>
#include <emex64lib/support/diagnostic/log.h>
#include <emex64lib/asm/label/label.h>
#include <emex64lib/asm/label/relocate.h>
#include <emex64lib/asm/emitter/emitter.h>
#include <emex64lib/asm/invocation.h>
#include <emex64lib/asm/lexer.h>
#include <emex64lib/asm/expr.h>

void assembler_emit_end(assembler_invocation_t *inv)
{
    vbitwalker_write(inv->out_vbitwalker, kEmex64ParameterCodingEnd, 3);
}

bool opcode_arg_is_branch_target(kEmex64Opcode op,
                                 uint64_t argno)
{
    switch(op)
    {
        case kEmex64OpcodeB:
        case kEmex64OpcodeBE:
        case kEmex64OpcodeBNE:
        case kEmex64OpcodeBLE:
        case kEmex64OpcodeBGE:
        case kEmex64OpcodeBLT:
        case kEmex64OpcodeBGT:
        case kEmex64OpcodeBLW:
            return argno == 0;
        case kEmex64OpcodeBZ:
        case kEmex64OpcodeBNZ:
            return argno == 1;
        default:
            return false;
    }
}

bool assembler_emit_instruction(assembler_line_t *al)
{
    if(al->token[0]->type != kAssemblerTokenTypeInstruction)
    {
        diag_error(al->token[0], "expected instruction identifier, but got %s '%s'\n", assembler_lexer_str_for_token_type(al->token[0]->type), al->token[0]->str);
        return false;
    }

    const kEmex64Opcode opcode = al->token[0]->instruction_identifier.v;
    const emex64_opfunc_entry_t *entry = &kEmex64OpfuncTable[opcode];

    uint64_t operand_total = 0;
    if(al->token_cnt > 1)
    {
        operand_total = 1;
        for(uint64_t k = 1; k < al->token_cnt; k++)
        {
            if(al->token[k]->type == kAssemblerTokenTypeComma)
            {
                operand_total++;
            }
        }
    }

    if(operand_total > EMEX64_MAX_ARGS)
    {
        diag_error(al->token[al->token_cnt - 1], "holy smokes, why soo many operands, maximum is %d operands in emex64\n", EMEX64_MAX_ARGS);
        return false;
    }
    if(operand_total > entry->maxargs)
    {
        diag_error(al->token[al->token_cnt - 1], "too many operands for a %s instruction, expected %d operands, but got %llu operands\n", al->token[0]->str, entry->maxargs, (unsigned long long)operand_total);
        return false;
    }
    if(operand_total < entry->minargs)
    {
        diag_error(al->token[0], "too few operands for a %s instruction, expected %d operands, but got %llu operands\n", al->token[0]->str, entry->minargs, (unsigned long long)operand_total);
        return false;
    }

    assembler_emit_opcode(al->inv, opcode);

    uint64_t i = 1;
    uint64_t argno = 0;
    while(i < al->token_cnt)
    {
        uint64_t start = i;
        while(i < al->token_cnt && al->token[i]->type != kAssemblerTokenTypeComma)
        {
            i++;
        }
        assembler_token_t **operand = &al->token[start];
        uint64_t operand_cnt = i - start;

        if(i < al->token_cnt)
        {
            i++;
        }

        if(operand_cnt == 0)
        {
            diag_error(al->token[start > 0 ? start - 1 : 0], "empty operand\n");
            return false;
        }

        if(operand_cnt == 1 && operand[0]->type == kAssemblerTokenTypeRegister)
        {
            assembler_emit_register(al->inv, operand[0]->register_identifier.v);
            argno++;
            continue;
        }

        if(opcode_arg_accepts_reg_only(entry, argno))
        {
            diag_error(operand[0], "expected register identifier, got %s '%s'\n", assembler_lexer_str_for_token_type(operand[0]->type), operand[0]->str);
            return false;
        }

        if(operand_cnt == 1 && operand[0]->type == kAssemblerTokenTypeIdentifier)
        {
            bool local;
            char *label = NULL;
            if(operand[0]->str[0] == '.')
            {
                local = true;
                asprintf(&label, "%s%s", al->inv->label_scope, operand[0]->str);
            }
            else
            {
                local = false;
                label = strdup(operand[0]->str);
            }

            vbitwalker_write(al->inv->out_vbitwalker, kEmex64ParameterCodingAddr64, 3);
            vbitwalker_align_byte(al->inv->out_vbitwalker);

            if(!assembler_label_relocate_append(al->inv, label, local, operand[0]))
            {
                diag_fatal(operand[0], "out of memory, can't append relocation to relocation table\n");
                return false;
            }

            vbitwalker_skip(al->inv->out_vbitwalker, 64);
            argno++;
            continue;
        }

        int64_t value;
        if(!assembler_eval_const(operand, operand_cnt, &value))
        {
            return false;
        }

        if(opcode_arg_is_branch_target(opcode, argno))
        {
            assembler_emit_addr64(al->inv, (uint64_t)value);
        }
        else
        {
            assembler_emit_imm(al->inv, (uint64_t)value);
        }
        argno++;
    }

    if(entry->maxargs != argno)
    {
        assembler_emit_end(al->inv);
    }

    vbitwalker_align_byte(al->inv->out_vbitwalker);
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
            failed = true;
            diag_error(reloc->at_link, "use of undeclared identifier '%s'\n", reloc->name);
            if(++errors >= 10)
            {
                diag_fatal(NULL, "too many errors emitted, stopping now\n");
                return false;
            }
        }

        /* travel down the list */
        reloc = reloc->next;
    }

    vbitwalker_sync(inv->out_vbitwalker);

    return !failed;
}
