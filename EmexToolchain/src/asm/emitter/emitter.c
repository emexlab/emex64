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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <EmexToolchain/support/parser.h>
#include <EmexToolchain/support/virtual/vbitwalker.h>
#include <EmexToolchain/support/diagnostic/log.h>
#include <EmexToolchain/asm/label/label.h>
#include <EmexToolchain/asm/label/relocate.h>
#include <EmexToolchain/asm/emitter/emitter.h>
#include <EmexToolchain/asm/invocation.h>
#include <EmexToolchain/asm/lexer.h>
#include <EmexToolchain/asm/expr.h>

void assembler_emit_end(assembler_invocation_t *inv)
{
    vbitwalker_write(inv->out_vbitwalker, kE64ParameterCodingEnd, 3);
}

bool opcode_arg_is_branch_target(kE64Opcode op,
                                 uint64_t argno)
{
    switch(op)
    {
        case kE64OpcodeB:
        case kE64OpcodeBE:
        case kE64OpcodeBNE:
        case kE64OpcodeBLE:
        case kE64OpcodeBGE:
        case kE64OpcodeBLT:
        case kE64OpcodeBGT:
        case kE64OpcodeBLW:
            return argno == 0;
        case kE64OpcodeBZ:
        case kE64OpcodeBNZ:
            return argno == 1;
        default:
            return false;
    }
}

bool assembler_emit_instruction(assembler_line_t *al)
{
    if(al->token[0]->type != kAssemblerTokenTypeInstruction)
    {
        diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(al->token[0]), "expected instruction identifier, but got %s '%s'", assembler_lexer_str_for_token_type(al->token[0]->type), al->token[0]->str);
        return false;
    }

    const kE64Opcode opcode = al->token[0]->instruction_identifier.v;
    const emex64_opfunc_entry_t *entry = &kE64OpfuncTable[opcode];

    uint64_t operand_total = 0;
    kAssemblerTokenType ptype;
    if(al->token_cnt > 1)
    {
        operand_total = 1;
        for(uint64_t k = 1; k < al->token_cnt; k++)
        {
            if(k != 1 && ptype == kAssemblerTokenTypeComma && al->token[k]->type == kAssemblerTokenTypeComma)
            {
                diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(al->token[k - 1]), "expected operand after %s '%s'", assembler_lexer_str_for_token_type(al->token[k - 1]->type), al->token[k - 1]->str);
                return false;
            }

            ptype = al->token[k]->type;
            if(al->token[k]->type == kAssemblerTokenTypeComma)
            {
                operand_total++;
            }
        }
    }

    if(operand_total > EMEX64_MAX_ARGS)
    {
        diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(al->token[al->token_cnt - 1]), "holy smokes, why soo many operands, maximum is %d operands in emex64", EMEX64_MAX_ARGS);
        return false;
    }
    if(operand_total > entry->maxargs)
    {
        diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(al->token[al->token_cnt - 1]), "too many operands for a %s instruction, expected %d operands, but got %llu operands", al->token[0]->str, entry->maxargs, (unsigned long long)operand_total);
        return false;
    }
    if(operand_total < entry->minargs)
    {
        diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(al->token[al->token_cnt - 1]), "too few operands for a %s instruction, expected %d operands, but got %llu operands\n", al->token[0]->str, entry->minargs, (unsigned long long)operand_total);
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
            diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(al->token[start > 0 ? start - 1 : 0]), "empty operand");
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
            diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(operand[0]), "expected register identifier, but got %s '%s'", assembler_lexer_str_for_token_type(operand[0]->type), operand[0]->str);
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

            vbitwalker_write(al->inv->out_vbitwalker, kE64ParameterCodingAddr64, 3);
            vbitwalker_align_byte(al->inv->out_vbitwalker);

            if(!assembler_label_relocate_append(al->inv, label, local, operand[0]))
            {
                diagnostic_report(al->inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(operand[0]),  "out of memory, can't append relocation to relocation table");
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
                        diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL,  "too many errors emitted, stopping now");
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
                        diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL,  "too many errors emitted, stopping now");
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
            diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(reloc->at_link),  "use of undeclared identifier '%s'", reloc->name);
            if(++errors >= 10)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL,  "too many errors emitted, stopping now");
                return false;
            }
        }

        /* travel down the list */
        reloc = reloc->next;
    }

    vbitwalker_sync(inv->out_vbitwalker);

    return !failed;
}
