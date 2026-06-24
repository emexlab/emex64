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

void assembler_emit_end(assembler_invocation_t *inv)
{
    vbitwalker_write(inv->out_vbitwalker, kEmex64ParameterCodingEnd, 3);
}

bool assembler_emit_instruction(assembler_line_t *al)
{
    kEmex64Opcode opcode = opcode_from_string(al->token[0]->str);
    if(opcode == kEmex64OpcodeInvalid)
    {
        diag_error(al->token[0], "use of unknown instruction '%s'\n", al->token[0]->str);
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
        diag_error(al->token[al->token_cnt - 1], "too many operands for a %s instruction, expected %d operands, but got %d operands\n", al->token[0]->str, entry->maxargs, al->token_cnt - 1);
        return false;
    }
    else if((al->token_cnt - 1) < entry->minargs)
    {
        diag_error(al->token[al->token_cnt - 1], "too few operands for a %s instruction, expected %d operands, but got %d operands\n", al->token[0]->str, entry->minargs, al->token_cnt - 1);
        return false;
    }

    /* emitting the instruction */
    assembler_emit_opcode(al->inv, opcode);

    for(uint64_t i = 1; i < al->token_cnt; i++)
    {
        if(al->token[i]->type == kAssemblerTokenTypeRegister)
        {
            /* registers are always allowed so far */
            assembler_emit_register(al->inv, al->token[i]->register_literal.v);
            continue;
        }

        /* checking if allowed to be something else than a register */
        if(opcode_arg_accepts_reg_only(entry,  i - 1))
        {
            diag_error(al->token[i], "expected register, got %s '%s'\n", assembler_lexer_str_for_token_type(al->token[i]->type),  al->token[i]->str);
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
        if(al->token[i]->type == kAssemblerTokenTypeIdentifier)
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

            vbitwalker_write(al->inv->out_vbitwalker, kEmex64ParameterCodingAddr64, 3);
            vbitwalker_align_byte(al->inv->out_vbitwalker);

            /* append label callsite to relocation table */
            if(!assembler_label_relocate_append(al->inv, label, local, al->token[i]))
            {
                diag_fatal(al->token[i], "out of memory, can't append relocation to relocation table\n", al->token[i]->str);
                return false;
            }

            /*
             * skip the 64bit the label occupies
             * as we added it to the relocation table
             * already. the relocation table later will
             * fill this space with the address.
             */
            vbitwalker_skip(al->inv->out_vbitwalker, 64);
        }
        else if(al->token[i]->type == kAssemblerTokenTypeInteger)
        {
            /* branches work different, they have offset branching */
            if((i == 1 && (opcode == kEmex64OpcodeB   || opcode == kEmex64OpcodeBE  || opcode == kEmex64OpcodeBNE   ||
                           opcode == kEmex64OpcodeBLE || opcode == kEmex64OpcodeBGE || opcode == kEmex64OpcodeBLT   ||
                           opcode == kEmex64OpcodeBGT || opcode == kEmex64OpcodeBLW)) ||
               (i == 2 && (opcode == kEmex64OpcodeBZ  || opcode == kEmex64OpcodeBNZ)))
            {
                assembler_emit_addr64(al->inv, al->token[i]->integer_literal.v);
            }
            else
            {
                /* its a immediate */
                assembler_emit_imm(al->inv, al->token[i]->integer_literal.v);
            }
        }
        else
        {
            diag_error(al->token[i], "didn't expect %s '%s' within a instruction definition\n", assembler_lexer_str_for_token_type(al->token[i]->type), al->token[i]->str);
            return false;
        }
    }

    if(entry->maxargs != (al->token_cnt - 1))
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
