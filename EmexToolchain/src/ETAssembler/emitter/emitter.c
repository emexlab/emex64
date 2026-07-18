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
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <EmexToolchain/Support/parser.h>
#include <EmexToolchain/Support/virtual/vbitwalker.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETAssembler/label/label.h>
#include <EmexToolchain/ETAssembler/label/relocate.h>
#include <EmexToolchain/ETAssembler/emitter/emitter.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>
#include <EmexToolchain/ETAssembler/lexer.h>
#include <EmexToolchain/ETAssembler/expr.h>

void assembler_emit_end(assembler_invocation_t *inv)
{
    vbitwalker_write(inv->out_vbitwalker, kE64ParameterCodingEnd, 4);
}

static inline Boolean opcode_arg_is_branch_target(E64Opcode op,
                                                  UInt64 argno)
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

static inline Boolean assembler_emit_operand(assembler_token_t *operand)
{
    if(operand->type == kETAssemblerTokenTypeRegister)
    {
        assembler_emit_register(operand->al->inv, operand->register_identifier.v, operand->register_identifier.increment, operand->register_identifier.actuallyDecrement);
    }
    else if(operand->type == kETAssemblerTokenTypeRegisterExtended)
    {
        assembler_emit_register_extended(operand->al->inv, operand->register_identifier.v_extended);
    }
    else if(operand->type == kETAssemblerTokenTypeInteger)
    {
        assembler_emit_imm(operand->al->inv, (UInt64)operand->integer_literal.v);
    }
    else if(operand->type == kETAssemblerTokenTypeIdentifier)
    {
        Boolean local;
        char *label = NULL;
        if(operand->str[0] == '.')
        {
            local = true;
            asprintf(&label, "%s%s", operand->al->inv->label_scope, operand->str);
        }
        else
        {
            local = false;
            label = strdup(operand->str);
        }

        vbitwalker_write(operand->al->inv->out_vbitwalker, kE64ParameterCodingAddr64, 4);
        vbitwalker_align_byte(operand->al->inv->out_vbitwalker);

        if(!assembler_label_relocate_append(operand->al->inv, label, local, operand))
        {
            diagnostic_report(operand->al->inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(operand),  "out of memory, can't append relocation to relocation table");
            return false;
        }

        vbitwalker_skip(operand->al->inv->out_vbitwalker, 64);
    }
    else
    {
        diagnostic_report(operand->al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(operand), "unexpected %s '%s'", assembler_lexer_str_for_token_type(operand->type), operand->str);
        return false;
    }
    return true;
}

Boolean assembler_emit_instruction(assembler_line_t *al)
{
    if(al->token[0]->type != kETAssemblerTokenTypeInstruction)
    {
        diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(al->token[0]), "expected instruction identifier, but got %s '%s'", assembler_lexer_str_for_token_type(al->token[0]->type), al->token[0]->str);
        return false;
    }

    const E64Opcode opcode = al->token[0]->instruction_identifier.v;
    const emex64_opfunc_entry_t *entry = &kE64OpfuncTable[opcode];

    UInt64 operand_total = 0;
    ETAssemblerTokenType ptype = kETAssemblerTokenTypeInvalid;
    if(al->token_cnt > 1)
    {
        operand_total = 1;
        for(UInt64 k = 1; k < al->token_cnt; k++)
        {
            if(k != 1 && ptype == kETAssemblerTokenTypeComma && al->token[k]->type == kETAssemblerTokenTypeComma)
            {
                diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(al->token[k - 1]), "expected operand after %s '%s'", assembler_lexer_str_for_token_type(al->token[k - 1]->type), al->token[k - 1]->str);
                return false;
            }

            ptype = al->token[k]->type;
            if(al->token[k]->type == kETAssemblerTokenTypeComma)
            {
                operand_total++;
            }
        }

        if(ptype == kETAssemblerTokenTypeComma)
        {
            diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(al->token[al->token_cnt - 1]), "expected operand after %s '%s'", assembler_lexer_str_for_token_type(al->token[al->token_cnt - 1]->type), al->token[al->token_cnt - 1]->str);
            return false;
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
        diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(al->token[al->token_cnt - 1]), "too few operands for a %s instruction, expected %d operands, but got %llu operands", al->token[0]->str, entry->minargs, (unsigned long long)operand_total);
        return false;
    }

    assembler_emit_opcode(al->inv, opcode);

    UInt64 i = 1;
    UInt64 argno = 0;
    while(i < al->token_cnt)
    {
        UInt64 start = i;
        while(i < al->token_cnt && al->token[i]->type != kETAssemblerTokenTypeComma)
        {
            i++;
        }
        assembler_token_t **operand = &al->token[start];
        UInt64 operand_cnt = i - start;

        if(i < al->token_cnt)
        {
            i++;
        }

        if(operand_cnt == 0)
        {
            diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(al->token[start > 0 ? start - 1 : 0]), "empty operand");
            return false;
        }

        if(operand_cnt == 1 && operand[0]->type == kETAssemblerTokenTypeRegister)
        {
            assembler_emit_register(al->inv, operand[0]->register_identifier.v, operand[0]->register_identifier.increment, operand[0]->register_identifier.actuallyDecrement);
            argno++;
            continue;
        }

        if(operand_cnt == 1 && operand[0]->type == kETAssemblerTokenTypeRegisterExtended)
        {
            assembler_emit_register_extended(al->inv, operand[0]->register_identifier.v_extended);
            argno++;
            continue;
        }

        /* check if this is a LPack */
        if(operand[0]->type == kETAssemblerTokenTypeLPack)
        {
            if(operand_cnt > 5)
            {
                diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(operand[operand_cnt - 1]), "too many operands for a lpack statement thingy, expected 5 operands, but got %llu operands", operand_cnt);
                return false;
            }
            else if(operand_cnt < 5)
            {
                diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(operand[operand_cnt - 1]), "too few operands for a lpack statement thingy, expected 5 operands, but got %llu operands", operand_cnt);
                return false;
            }

            /* the second operand has to be a plus or a minus */
            if(operand[2]->type == kETAssemblerTokenTypePlus)
            {
                vbitwalker_write(al->inv->out_vbitwalker, kE64ParameterCodingOffsetAdd, 4);
            }
            else if(operand[2]->type == kETAssemblerTokenTypeMinus)
            {
                vbitwalker_write(al->inv->out_vbitwalker, kE64ParameterCodingOffsetSub, 4);
            }
            else
            {
                diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(operand[2]), "expected plus or minus, but got %s '%s'", assembler_lexer_str_for_token_type(operand[2]->type), operand[2]->str);
                return false;
            }

            if(!assembler_emit_operand(operand[1]) || !assembler_emit_operand(operand[3]))
            {
                return false;
            }

            argno++;
            continue;
        }

        if(opcode_arg_accepts_reg_only(entry, argno))
        {
            diagnostic_report(al->inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(operand[0]), "expected register identifier, but got %s '%s'", assembler_lexer_str_for_token_type(operand[0]->type), operand[0]->str);
            return false;
        }

        if(operand_cnt == 1 && operand[0]->type == kETAssemblerTokenTypeIdentifier)
        {
            Boolean local;
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

            vbitwalker_write(al->inv->out_vbitwalker, kE64ParameterCodingAddr64, 4);
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

        SInt64 value;
        if(!assembler_eval_const(operand, operand_cnt, &value))
        {
            return false;
        }

        if(opcode_arg_is_branch_target(opcode, argno))
        {
            assembler_emit_addr64(al->inv, (UInt64)value);
        }
        else
        {
            assembler_emit_imm(al->inv, (UInt64)value);
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

Boolean assembler_emit(assembler_invocation_t *inv)
{
    Boolean failed = false;
    UInt8 errors = 0;

    /* iterate through each line */
    for(UInt64 i = 0; i < inv->line_cnt; i++)
    {
        switch(inv->line[i]->type)
        {
            case kETAssemblerLineTypeSymbol:
            case kETAssemblerLineTypeLabel:
            case kETAssemblerLineTypeExternSymbol:
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
            case kETAssemblerLineTypeAssembly:
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
