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

#include <string.h>
#include <assert.h>
#include <EmexToolchain/support/parser.h>
#include <EmexToolchain/support/diagnostic/log.h>
#include <EmexToolchain/asm/preprocessor/preprocessor.h>
#include <EmexToolchain/asm/invocation.h>
#include <EmexToolchain/asm/code.h>

static inline char *__assembler_preprocessor_include_directive_get_token(const char *token,
                                                                         bool *system_hdr)
{
    assert(token != NULL && system_hdr != NULL);

    size_t len = strlen(token);
    if(len <= 2)
    {
        return NULL;
    }

    /* checking type of hdr */
    if(token[0] == '"' && token[len - 1] == '"')
    {
        *system_hdr = false;
    }
    else if(token[0] == '<' && token[len - 1] == '>')
    {
        *system_hdr = true;
    }
    else
    {
        /* unknown token */
        return NULL;
    }

    /* extract hdr path */
    char *hdr = malloc(len - 1);
    if(hdr == NULL)
    {
        return NULL;
    }

    memcpy(hdr, token + 1, len - 2);
    hdr[len - 2] = '\0';

    return hdr;
}

bool assembler_preprocessor_run(assembler_invocation_t *inv)
{
    /* allocating macro storage */
    assembler_macro_storage_t *storage = assembler_macro_storage_alloc();
    if(storage == NULL)
    {
        diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "out of memory, can't allocate macro storage");
        return false;
    }

    /* adding predefined macro definitions */
    for(uint64_t i = 0; i < inv->definition_cnt; i++)
    {
        const char **inject_token = calloc(1, sizeof(char*));
        if(inject_token == NULL)
        {
            diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "out of memory, can't allocate inject_token");
            assembler_macro_storage_dealloc(storage);
            return false;
        }

        inject_token[0] = inv->definition[i].value;

        if(!assembler_macro_storage_append_macro_char(storage, inv->definition[i].match, inject_token, 1))
        {
            diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "out of memory, can't append macro to macro storage");
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
        if(inv->line[li]->token_cnt <= 0 || /* whitespaces stay this type if im not wrong UwU */
           inv->line[li]->type == kAssemblerLineTypeIgnore)
        {
            /* whitespaces don't matter in emex64asm lol >:3 */
            continue;
        }

        kAssemblerPreprocessorDirectiveType type = assembler_directive_type_for_str(inv->line[li]->token[0]->str);
        if(!state.in_a_condition || state.condition_met || (type != kAssemblerPreprocessorDirectiveTypeUnknown && type != kAssemblerPreprocessorDirectiveTypeDefine && type != kAssemblerPreprocessorDirectiveTypeUndefine))
        {
            if(type == kAssemblerPreprocessorDirectiveTypeIfDefined ||
               type == kAssemblerPreprocessorDirectiveTypeIfNotDefined)
            {
                /* the values they cary shall be ignored as they shall not be resolved */
                goto handle_preprocessor_directive;
            }

            for(uint64_t ti = 0; ti < inv->line[li]->token_cnt; ti++)
            {
                if((ti == 1 && type == kAssemblerPreprocessorDirectiveTypeDefine) ||
                   (ti == 1 && type == kAssemblerPreprocessorDirectiveTypeUndefine))
                {
                    /*
                     * definition directives match value shall not be touched
                     * but the values they carry them selves shall be expanded
                     * do that a macro definition like this resolves:
                     * 
                     * %define% MEOW 2
                     * 
                     * %define% UWU MEOW
                     * 
                     */
                    continue;
                }

                /* matching macro */
                uint16_t macro_nest_remaining = UINT16_MAX;

            repeat:
                {
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
                    uint64_t real_len = inv->line[li]->token[ti]->real_len;

                    /* the consumed token always dies */
                    free(inv->line[li]->token[ti]->str);
                    free(inv->line[li]->token[ti]);

                    if(new_tokens > 1)
                    {
                        assembler_token_t **grown = realloc(inv->line[li]->token, new_token_cnt * sizeof(assembler_token_t*));
                        if(grown == NULL)
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "out of memory expanding macro");
                            goto failure;
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
                            diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "out of memory expanding macro");
                            goto failure;
                        }
                        at->al = inv->line[li];
                        at->str = strdup(found_match->inject_token[k]);
                        at->column_num = column_number;
                        at->real_len = real_len;
                        inv->line[li]->token[ti + k] = at;
                    }
                    
                    macro_nest_remaining--;
                    if(macro_nest_remaining == 0)
                    {
                        diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[ti]), "macro nesting limit of %d was reached", UINT16_MAX);
                        goto failure;
                    }

                    if(ti < inv->line[li]->token_cnt)
                    {
                        goto repeat;
                    }
                }
            }
        }
        else
        {
            inv->line[li]->type = kAssemblerLineTypeIgnore;
        }

    handle_preprocessor_directive:
        switch(inv->line[li]->type)
        {
            case kAssemblerLineTypePreprocessorDirective:
                switch(type)
                {
                    case kAssemblerPreprocessorDirectiveTypeInclude:
                    {
                        if(!(!state.in_a_condition || state.condition_met))
                        {
                            /* ignore includation */
                            inv->line[li]->type = kAssemblerLineTypeIgnore;
                            break;
                        }

                        /* extracting hdr path token and type of hdr path token */
                        bool system_hdr = false;
                        char *hdr_token = __assembler_preprocessor_include_directive_get_token(inv->line[li]->token[1]->str, &system_hdr);
                        if(hdr_token == NULL)
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[1]), "invalid token '%s' passed after %%include%% directive", inv->line[li]->token[1]->str);
                            goto failure;
                        }

                        /* looking for da cat in the file system ^^ */
                        char *hdr_path;
                        if(system_hdr)
                        {
                            hdr_path = assembler_code_find_system_header(hdr_token, (const char**)inv->include_dirs, inv->include_dir_cnt);
                        }
                        else
                        {
                            hdr_path = assembler_code_find_header(hdr_token, inv->file[inv->line[li]->file_idx]->path);
                        }

                        /* did I catch this cat >:3 */
                        if(hdr_path == NULL)
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[1]), "couldn't find header at path '%s'", hdr_token);
                            free(hdr_token);
                            goto failure;
                        }
                        free(hdr_token);

                        /* now openup a file */
                        emex_file_t *file = emex_file_alloc(hdr_path, in_data_file_policy);
                        if(file == NULL)
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[1]), "couldn't open header at path '%s'", hdr_path);
                            free(hdr_path);
                            goto failure;
                        }
                        free(hdr_path);

                        if(!assembler_code_inject_file(inv, li, file))
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(inv->line[li]->token[1]), "couldn't inject header at path '%s' into invocation", hdr_path);
                            emex_file_dealloc(file);
                            goto failure;
                        }

                        li--; /* file was inseted at this location */
                        break;
                    }
                    case kAssemblerPreprocessorDirectiveTypeDefine:
                        if(!assembler_macro_storage_append_macro(storage, inv->line[li]->token[1]->str, &inv->line[li]->token[2], inv->line[li]->token_cnt - 2))
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "out of memory, can't append macro to macro storage");
                            goto failure;
                        }
                        break;
                    case kAssemblerPreprocessorDirectiveTypeUndefine:
                        assembler_macro_storage_remove_macro(storage, inv->line[li]->token[1]->str);
                        break;
                    case kAssemblerPreprocessorDirectiveTypeIf:
                    {
                        bool parent_active = !state.in_a_condition || state.condition_met;
                        if(!assembler_condition_state_push(&state))
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(inv->line[li]->token[0]), "failed to push condition frame onto condition state");
                            goto failure;
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
                    case kAssemblerPreprocessorDirectiveTypeIfDefined:
                    {
                        bool parent_active = !state.in_a_condition || state.condition_met;
                        if(!assembler_condition_state_push(&state))
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(inv->line[li]->token[0]), "failed to push condition frame onto condition state");
                            goto failure;
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
                    case kAssemblerPreprocessorDirectiveTypeIfNotDefined:
                    {
                        bool parent_active = !state.in_a_condition || state.condition_met;
                        if(!assembler_condition_state_push(&state))
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(inv->line[li]->token[0]), "failed to push condition frame onto condition state");
                            goto failure;
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
                    case kAssemblerPreprocessorDirectiveTypeElseIf:
                        if(state.in_a_condition == false)
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[0]), "%%elif%% directive was defined, but no %%if%% directive was defined before.");
                            goto failure;
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
                    case kAssemblerPreprocessorDirectiveTypeElse:
                        if(!state.in_a_condition)
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[0]), "%%else%% directive was defined, but no %%if%% directive was defined before.");
                            goto failure;
                        }
                        
                        if(state.in_a_else_condition)
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[0]), "%%else%% directive was defined inside another %%else%% directive.");
                            goto failure;
                        }
                        
                        state.condition_met = (!state.finalized && !state.condition_met) && state.parent_active;
                        state.in_a_else_condition = true;
                        break;
                    case kAssemblerPreprocessorDirectiveTypeEndIf:
                        if(state.in_a_condition == false)
                        {
                            diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[0]), "%%endif%% directive was defined, but no %%if%% directive was defined before.");
                            goto failure;
                        }
                        assembler_condition_state_pop(&state);
                        break;
                    default:
                        break;
                }
                
                if(type != kAssemblerPreprocessorDirectiveTypeEndIf &&
                   type != kAssemblerPreprocessorDirectiveTypeDefine)
                {
                    state.last_condition_line = inv->line[li];
                }

                /* include doesn't exist anymore at that offset */
                if(type != kAssemblerPreprocessorDirectiveTypeInclude)
                {
                    inv->line[li]->type = kAssemblerLineTypeIgnore;
                }
                break;
            default:
                break;
        }
    }

    if(state.in_a_condition)
    {
        diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(state.last_condition_line->token[0]), "%%if%% was defined but no matching %%endif%% was found.");
    }
    
    bool in_a_condition = state.in_a_condition;
    assembler_condition_state_deinit(&state);
    assembler_macro_storage_dealloc(storage);
    return !in_a_condition;

failure:
    assembler_condition_state_deinit(&state);
    assembler_macro_storage_dealloc(storage);
    return false;
}

