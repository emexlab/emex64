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

#include <string.h>
#include <emex64lib/asm/preprocessor/preprocessor.h>
#include <emex64lib/support/parser.h>
#include <emex64lib/support/diagnostic/legacy.h>
#include <emex64lib/asm/invocation.h>

bool assembler_preprocessor_run(assembler_invocation_t *inv)
{
    /* allocating macro storage */
    assembler_macro_storage_t *storage = assembler_macro_storage_alloc();
    if(storage == NULL)
    {
        diag_fatal(NULL, "out of memory, can't allocate macro storage\n");
        return false;
    }

    /* adding predefined macro definitions */
    for(uint64_t i = 0; i < inv->definition_cnt; i++)
    {
        const char **inject_token = calloc(1, sizeof(char*));
        if(inject_token == NULL)
        {
            diag_fatal(NULL, "out of memory, can't allocate inject_token\n");
            assembler_macro_storage_dealloc(storage);
            return false;
        }

        inject_token[0] = inv->definition[i].value;

        if(!assembler_macro_storage_append_macro_char(storage, inv->definition[i].match, inject_token, 1))
        {
            diag_fatal(NULL, "out of memory, can't append macro to macro storage\n");
            free(inject_token);
            assembler_macro_storage_dealloc(storage);
            return false;
        }
    }

    /* expand macros */
    assembler_condition_state_t state;
    assembler_condition_state_init(&state);

    for(uint64_t li = 0; li < inv->line_cnt; li++)
    {
        if(!state.in_a_condition || state.condition_met || inv->line[li]->type == kAssemblerLineTypeConditionDirective)
        {
            for(uint64_t ti = 0; ti < inv->line[li]->token_cnt; ti++)
            {
                if(ti == 0 && (strcmp(inv->line[li]->token[ti]->str, "%ifdef%") == 0 || strcmp(inv->line[li]->token[ti]->str, "%ifndef%") == 0))
                {
                    /* definition check directives shall not be touched */
                    break;
                }
                else if(ti == 1 && strcmp(inv->line[li]->token[0]->str, "%define%") == 0)
                {
                    /* definition directives match value shall not be touched */
                    continue;
                }
            
                /* matching macro */
                uint16_t macro_nest_remaining = UINT16_MAX;

            repeat:
                {
                    if(inv->line[li]->token[ti]->type == kAssemblerTokenTypeStaticExpression)
                    {
                        diag_error(inv->line[li]->token[ti], "static expressions aren't supported yet!\n");
                        assembler_condition_state_deinit(&state);
                        assembler_macro_storage_dealloc(storage);
                        return false;
                    }
                    
                    assembler_macro_t *found_match = assembler_macro_storage_lookup(storage, inv->line[li]->token[ti]->str);
                    if(found_match == NULL)
                    {
                        continue;
                    }

                    uint64_t old_token_cnt = inv->line[li]->token_cnt;
                    uint64_t new_tokens = found_match->inject_token_cnt;
                    uint64_t tail = old_token_cnt - ti - 1;
                    uint64_t new_token_cnt = old_token_cnt - 1 + new_tokens;
                    uint64_t column_number = inv->line[li]->token[ti]->column_num;

                    /* the consumed token always dies */
                    free(inv->line[li]->token[ti]->str);
                    free(inv->line[li]->token[ti]);

                    if(new_tokens > 1)
                    {
                        assembler_token_t **grown = realloc(inv->line[li]->token, new_token_cnt * sizeof(assembler_token_t*));
                        if(grown == NULL)
                        {
                            diag_fatal(NULL, "out of memory expanding macro\n");
                            assembler_condition_state_deinit(&state);
                            assembler_macro_storage_dealloc(storage);
                            return false;
                        }
                        inv->line[li]->token = grown;

                        memmove(&inv->line[li]->token[ti + new_tokens], &inv->line[li]->token[ti + 1], tail * sizeof(assembler_token_t*));
                    }
                    else if(new_tokens == 0)
                    {
                        memmove(&inv->line[li]->token[ti], &inv->line[li]->token[ti + 1], tail * sizeof(assembler_token_t*));

                        if(new_token_cnt > 0)
                        {
                            assembler_token_t **shrunk = realloc(inv->line[li]->token, new_token_cnt * sizeof(assembler_token_t*));
                            if(shrunk != NULL)
                            {
                                inv->line[li]->token = shrunk;
                            }
                        }
                    }

                    inv->line[li]->token_cnt = new_token_cnt;

                    for(uint64_t k = 0; k < new_tokens; k++)
                    {
                        assembler_token_t *at = calloc(1, sizeof(assembler_token_t));
                        if(at == NULL)
                        {
                            diag_fatal(NULL, "out of memory expanding macro\n");
                            assembler_condition_state_deinit(&state);
                            assembler_macro_storage_dealloc(storage);
                            return false;
                        }
                        at->al = inv->line[li];
                        at->str = strdup(found_match->inject_token[k]);
                        at->column_num = column_number;
                        inv->line[li]->token[ti + k] = at;
                    }
                    
                    macro_nest_remaining--;
                    if(macro_nest_remaining == 0)
                    {
                        diag_error(inv->line[li]->token[ti], "macro nesting limit of %d was reached\n", UINT16_MAX);
                        assembler_condition_state_deinit(&state);
                        assembler_macro_storage_dealloc(storage);
                        return false;
                    }
                    
                    goto repeat;
                }
            }
        }
        else
        {
            inv->line[li]->type = kAssemblerLineTypeIgnore;
        }

        switch(inv->line[li]->type)
        {
            case kAssemblerLineTypeIgnore:
                break;
            case kAssemblerLineTypeDefinitionDirective:
                if(!assembler_macro_storage_append_macro(storage, inv->line[li]->token[1]->str, &inv->line[li]->token[2], inv->line[li]->token_cnt - 2))
                {
                    diag_fatal(NULL, "out of memory, can't append macro to macro storage\n");
                    assembler_condition_state_deinit(&state);
                    assembler_macro_storage_dealloc(storage);
                    return false;
                }

                /* macros shall not interfere with the code */
                inv->line[li]->type = kAssemblerLineTypeIgnore;
                break;
            case kAssemblerLineTypeConditionDirective:
                kAssemblerDirectiveType type = assembler_directive_type_for_str(inv->line[li]->token[0]->str);
                switch(type)
                {
                    case kAssemblerDirectiveTypeIf:
                    {
                        bool parent_active = !state.in_a_condition || state.condition_met;
                        if(!assembler_condition_state_push(&state))
                        {
                            diag_fatal(inv->line[li]->token[0], "failed to push condition frame onto condition state\n");
                            assembler_condition_state_deinit(&state);
                            assembler_macro_storage_dealloc(storage);
                            return false;
                        }
                        state.parent_active = parent_active;
                        
                    express_if_directive:
                        {
                            bool raw;
                            if(inv->line[li]->token_cnt >= 2 && strlen(inv->line[li]->token[1]->str) > 0)
                            {
                                parser_return_t pret = parse_value_from_string(inv->line[li]->token[1]->str);
                                raw = pret.type == emexParserValueTypeNumber ? (pret.value != 0) : true;
                            }
                            else
                            {
                                raw = false;
                            }
                            
                            state.condition_met = raw && state.parent_active;
                            if(raw)
                            {
                                state.finalized = true;
                            }
                            
                            state.in_a_condition = true;
                        }
                        break;
                    }
                    case kAssemblerDirectiveTypeIfDefined:
                    {
                        bool parent_active = !state.in_a_condition || state.condition_met;
                        if(!assembler_condition_state_push(&state))
                        {
                            diag_fatal(inv->line[li]->token[0], "failed to push condition frame onto condition state\n");
                            assembler_condition_state_deinit(&state);
                            assembler_macro_storage_dealloc(storage);
                            return false;
                        }
                        state.parent_active = parent_active;
                        
                        bool raw = false;
                        if(inv->line[li]->token_cnt >= 2 && strlen(inv->line[li]->token[1]->str) > 0)
                        {
                            assembler_macro_t *found_match = assembler_macro_storage_lookup(storage, inv->line[li]->token[1]->str);
                            raw = found_match != NULL;
                        }
                        
                        state.condition_met = raw && state.parent_active;
                        if(raw)
                        {
                            state.finalized = true;
                        }
                        
                        state.in_a_condition = true;
                        break;
                    }
                    case kAssemblerDirectiveTypeIfNotDefined:
                    {
                        bool parent_active = !state.in_a_condition || state.condition_met;
                        if(!assembler_condition_state_push(&state))
                        {
                            diag_fatal(inv->line[li]->token[0], "failed to push condition frame onto condition state\n");
                            assembler_condition_state_deinit(&state);
                            assembler_macro_storage_dealloc(storage);
                            return false;
                        }
                        state.parent_active = parent_active;
                        
                        bool raw = true;
                        if(inv->line[li]->token_cnt >= 2 && strlen(inv->line[li]->token[1]->str) > 0)
                        {
                            assembler_macro_t *found_match = assembler_macro_storage_lookup(storage, inv->line[li]->token[1]->str);
                            raw = found_match == NULL;
                        }
                        
                        state.condition_met = raw && state.parent_active;
                        if(raw)
                        {
                            state.finalized = true;
                        }
                        
                        state.in_a_condition = true;
                        break;
                    }
                    case kAssemblerDirectiveTypeElseIf:
                        if(state.in_a_condition == false)
                        {
                            diag_error(inv->line[li]->token[0], "%%elif%% directive was defined, but no %%if%% directive was defined before.\n");
                            assembler_condition_state_deinit(&state);
                            assembler_macro_storage_dealloc(storage);
                            return false;
                        }
                        
                        if(state.finalized || state.condition_met)
                        {
                            state.finalized = true;
                            state.condition_met = false;
                        }
                        else
                        {
                            goto express_if_directive;
                        }
                        break;
                    case kAssemblerDirectiveTypeElse:
                        if(!state.in_a_condition)
                        {
                            diag_error(inv->line[li]->token[0], "%%else%% directive was defined, but no %%if%% directive was defined before.\n");
                            assembler_condition_state_deinit(&state);
                            assembler_macro_storage_dealloc(storage);
                            return false;
                        }
                        
                        if(state.in_a_else_condition)
                        {
                            diag_error(inv->line[li]->token[0], "%%else%% directive was defined inside another %%else%% directive.\n");
                            assembler_condition_state_deinit(&state);
                            assembler_macro_storage_dealloc(storage);
                            return false;
                        }
                        
                        state.condition_met = (!state.finalized && !state.condition_met) && state.parent_active;
                        state.in_a_else_condition = true;
                        break;
                    case kAssemblerDirectiveTypeEndIf:
                        if(state.in_a_condition == false)
                        {
                            diag_error(inv->line[li]->token[0], "%%end%% directive was defined but no %%if%% directive was defined before.\n");
                            assembler_condition_state_deinit(&state);
                            assembler_macro_storage_dealloc(storage);
                            return false;
                        }
                        assembler_condition_state_pop(&state);
                        break;
                    default:
                        break;
                }
                
                if(type != kAssemblerDirectiveTypeEndIf)
                {
                    state.last_condition_line = inv->line[li];
                }
                inv->line[li]->type = kAssemblerLineTypeIgnore;
                break;
            default:
                break;
        }
    }

    if(state.in_a_condition)
    {
        diag_error(state.last_condition_line->token[0], "%%if%% was defined but no matching %%endif%% was found.\n");
    }
    
    bool in_a_condition = state.in_a_condition;
    assembler_condition_state_deinit(&state);
    assembler_macro_storage_dealloc(storage);
    return !in_a_condition;
}

