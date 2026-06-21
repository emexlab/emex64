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
#include <stdint.h>

#include <emex64lib/support/diagnostic/legacy.h>
#include <emex64lib/support/parser.h>

#include <emex64lib/asm/macro.h>
#include <emex64lib/asm/code.h>

typedef struct assembler_macro {
    const char *match;
    char **inject_token;
    uint64_t inject_token_cnt;
    struct assembler_macro *next;
} assembler_macro_t;

bool assembler_macro_expand(assembler_invocation_t *inv)
{
    /* count the amount of macros */
    uint64_t c = inv->definition_cnt;
    for(uint64_t i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->type == kAssemblerLineTypeMacroDefinition)
        {
            c++;
        }
    }

    /* allocating macro storage */
    assembler_macro_t *am = calloc(c, sizeof(assembler_macro_t));
    if(am == NULL)
    {
        diag_error(NULL, "something terrible has happened\n");
        return false;
    }

    /* adding predefined macro definitions */
    c = 0;
    for(uint64_t i = 0; i < inv->definition_cnt; i++)
    {
        am[c].match = inv->definition[i].match;
        am[c].inject_token_cnt = 1;
        am[c].inject_token = calloc(1, sizeof(char*));
        am[c].inject_token[0] = inv->definition[i].value;
        c++;
    }

    for(uint64_t i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->type == kAssemblerLineTypeMacroDefinition)
        {
            am[c].match = inv->line[i]->token[1]->str;
            am[c].inject_token_cnt = inv->line[i]->token_cnt - 2;
            am[c].inject_token = calloc(am[c].inject_token_cnt, sizeof(char*));
            for(uint64_t a = 0; a < am[c].inject_token_cnt; a++)
            {
                am[c].inject_token[a] = inv->line[i]->token[a + 2]->str;
            }
            inv->line[i]->type = kAssemblerLineTypeIgnore;
            c++;
        }
    }

    /* expand macro definitions */
    for(uint64_t li = 0; li < inv->line_cnt; li++)
    {
        for(uint64_t ti = 0; ti < inv->line[li]->token_cnt; ti++)
        {
            if(ti == 0 && strcmp(inv->line[li]->token[ti]->str, "%ifdef%") == 0)
            {
                /* NEXT LINE! */
                break;
            }

            /* matching macro */
            uint16_t macro_nest_remaining = UINT16_MAX;

        repeat:
            for(uint64_t ami = 0; ami < c; ami++)
            {
                if(inv->line[li]->token[ti]->type == kAssemblerTokenTypeStaticExpression)
                {
                    diag_error(inv->line[li]->token[ti], "static expressions aren't supported yet!\n");
                    return false;
                }
                else if(strcmp(inv->line[li]->token[ti]->str, am[ami].match) == 0)
                {
                    uint64_t old_token_cnt = inv->line[li]->token_cnt;
                    uint64_t new_tokens = (am[ami].inject_token_cnt);
                    uint64_t extra = new_tokens - 1;
                    uint64_t column_number = inv->line[li]->token[ti]->column_num;

                    if(extra > 0)
                    {
                        /* reallocating space to insert the macro */
                        inv->line[li]->token_cnt += extra;
                        inv->line[li]->token = realloc(inv->line[li]->token, inv->line[li]->token_cnt * sizeof(assembler_token_t*));

                        /* destroying affected token */
                        free(inv->line[li]->token[ti]->str);
                        free(inv->line[li]->token[ti]);

                        /* making space for macro to be inserted */
                        memmove(&inv->line[li]->token[ti + new_tokens], &inv->line[li]->token[ti + 1], (old_token_cnt - ti - 1) * sizeof(assembler_token_t));
                    }

                    for(uint64_t k = 0; k < new_tokens; k++)
                    {
                        assembler_token_t *at = calloc(1, sizeof(assembler_token_t));
                        at->al = inv->line[li];
                        at->str = strdup(am[ami].inject_token[k]);
                        at->column_num = column_number;
                        inv->line[li]->token[ti + k] = at;
                    }

                    macro_nest_remaining--;
                    if(macro_nest_remaining == 0)
                    {
                        diag_error(inv->line[li]->token[ti], "macro nesting limit of %d was reached\n", UINT16_MAX);
                        return false;
                    }

                    goto repeat;
                }
            }
        }
    }

    /* expand macro conditions */
    bool in_a_condition = false;
    bool condition_met = false;
    bool in_a_else_condition = false;
    assembler_line_t *last_condition_line = NULL;

    for(uint64_t li = 0; li < inv->line_cnt; li++)
    {
        if(inv->line[li]->type == kAssemblerLineTypeMacroDefinition)
        {
            if(in_a_condition)
            {
                diag_error(inv->line[li]->token[0], "%%define%% was used in a condition, which doesn't work and is WIP.\n");
                return false;
            }
        }
        else if(inv->line[li]->type == kAssemblerLineTypeMacroCondition)
        {
            if(strcmp(inv->line[li]->token[0]->str, "%if%") == 0)
        if_macro_condition_expand:
            {
                if(in_a_condition == true)
                {
                    diag_error(inv->line[li]->token[0], "%%if%% was defined inside another %%if%%. condition nesting is still WIP.\n");
                    return false;
                }

                if(inv->line[li]->token_cnt >= 2 && strlen(inv->line[li]->token[1]->str) > 0)
                {
                    parser_return_t pret = parse_value_from_string(inv->line[li]->token[1]->str);
                    if(pret.type == emexParserValueTypeNumber)
                    {
                        condition_met = (pret.value != 0);
                    }
                    else
                    {
                        condition_met = true;
                    }
                }
                else
                {
                    condition_met = false;
                }

                in_a_condition = true;
            }
            if(strcmp(inv->line[li]->token[0]->str, "%ifdef%") == 0)
            {
                if(in_a_condition == true)
                {
                    diag_error(inv->line[li]->token[0], "%%ifdef%% was defined inside another %%if%%. condition nesting is still WIP.\n");
                    return false;
                }

                condition_met = false;
                if(inv->line[li]->token_cnt >= 2 && strlen(inv->line[li]->token[1]->str) > 0)
                {
                    for(uint64_t ami = 0; ami < c; ami++)
                    {
                        if(strcmp(inv->line[li]->token[1]->str, am[ami].match) == 0)
                        {
                            condition_met = true;
                            break;
                        }
                    }
                }

                in_a_condition = true;
            }
            else if(strcmp(inv->line[li]->token[0]->str, "%elseif%") == 0)
            {
                if(in_a_condition == false)
                {
                    diag_error(inv->line[li]->token[0], "%%elseif%% was defined but no %%if%% was defined prior.\n");
                    return false;
                }
                goto if_macro_condition_expand;
            }
            else if(strcmp(inv->line[li]->token[0]->str, "%else%") == 0)
            {
                if(in_a_condition == false)
                {
                    diag_error(inv->line[li]->token[0], "%%else%% was defined but no %%if%% was defined prior.\n");
                    return false;
                }
                else if(in_a_else_condition)
                {
                    diag_error(inv->line[li]->token[0], "%%else%% was defined after another %%else%%.\n");
                    return false;
                }
                condition_met = !condition_met;
                in_a_else_condition = true;
            }
            else if(strcmp(inv->line[li]->token[0]->str, "%endif%") == 0)
            {
                if(in_a_condition == false)
                {
                    diag_error(inv->line[li]->token[0], "%%endif%% was defined but no %%if%% was defined prior.\n");
                    return false;
                }
                in_a_condition = false;
                condition_met = false;
                in_a_else_condition = false;
            }

            last_condition_line = inv->line[li];
            inv->line[li]->type = kAssemblerLineTypeIgnore;
        }
        else if(in_a_condition && !condition_met)
        {
            inv->line[li]->type = kAssemblerLineTypeIgnore;
        }
    }

    if(in_a_condition)
    {
        diag_error(last_condition_line->token[0], "%%if%% was defined but no matching %%endif%% was found.\n");
        return false;
    }

    free(am);

    return true;
}
