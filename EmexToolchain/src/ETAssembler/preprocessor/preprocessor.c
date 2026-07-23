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
#include <EmexToolchain/Support/parser.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETAssembler/preprocessor/preprocessor.h>
#include <EmexToolchain/ETAssembler/ETAssemblerInvocation.h>
#include <EmexToolchain/ETAssembler/code.h>

static inline char *__assembler_preprocessor_include_directive_get_token(const char *token,
                                                                         Boolean *system_hdr)
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

Boolean assembler_preprocessor_run(ETAssemblerInvocationRef inv)
{
    /* allocating macro storage */
    assembler_macro_storage_t *storage = assembler_macro_storage_alloc();
    if(storage == NULL)
    {
        ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't allocate macro storage"));
        return false;
    }

    /* adding predefined macro definitions */
    EFIndex definitionCount = EFArrayGetCount(inv->definitions);
    for(EFIndex index = 0; index < definitionCount; index++)
    {
        assembler_macro_definition_t *definition = EFArrayGetValueAtIndex(inv->definitions, index);

        const char **inject_token = calloc(1, sizeof(char*));
        if(inject_token == NULL)
        {
            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't allocate inject_token"));
            assembler_macro_storage_dealloc(storage);
            return false;
        }

        inject_token[0] = definition->value;

        if(!assembler_macro_storage_append_macro_char(storage, definition->match, inject_token, 1))
        {
            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't append macro to macro storage"));
            free(inject_token);
            assembler_macro_storage_dealloc(storage);
            return false;
        }
    }

    /* expand macros */
    assembler_condition_state_t state;
    assembler_condition_state_init(&state);

    for(UInt64 li = 0; li < inv->line_cnt; li++)
    {
        if(inv->line[li]->token_cnt <= 0 || /* whitespaces stay this type if im not wrong UwU */
           inv->line[li]->type == kETAssemblerLineTypeIgnore)
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

            for(UInt64 ti = 0; ti < inv->line[li]->token_cnt; ti++)
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
                UInt16 macro_nest_remaining = UINT16_MAX;

            repeat:
                {
                    assembler_macro_t *found_match = assembler_macro_storage_lookup(storage, inv->line[li]->token[ti]->str);
                    if(found_match == NULL)
                    {
                        continue;
                    }

                    UInt64 old_token_cnt = inv->line[li]->token_cnt;
                    UInt64 new_tokens = found_match->inject_token_cnt;
                    UInt64 tail = old_token_cnt - ti - 1;
                    UInt64 new_token_cnt = old_token_cnt - 1 + new_tokens;
                    UInt64 column_number = inv->line[li]->token[ti]->column_num;
                    UInt64 real_len = inv->line[li]->token[ti]->real_len;

                    /* the consumed token always dies */
                    free(inv->line[li]->token[ti]->str);
                    free(inv->line[li]->token[ti]);

                    if(new_tokens > 1)
                    {
                        assembler_token_t **grown = realloc(inv->line[li]->token, new_token_cnt * sizeof(assembler_token_t*));
                        if(grown == NULL)
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory expanding macro"));
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

                    for(UInt64 k = 0; k < new_tokens; k++)
                    {
                        assembler_token_t *at = calloc(1, sizeof(assembler_token_t));
                        if(at == NULL)
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory expanding macro"));
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
                        ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[ti]), EFSTR("macro nesting limit of %d was reached"), UINT16_MAX);
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
            inv->line[li]->type = kETAssemblerLineTypeIgnore;
        }

    handle_preprocessor_directive:
        switch(inv->line[li]->type)
        {
            case kETAssemblerLineTypePreprocessorDirective:
                switch(type)
                {
                    case kAssemblerPreprocessorDirectiveTypeInclude:
                    {
                        if(!(!state.in_a_condition || state.condition_met))
                        {
                            /* ignore includation */
                            inv->line[li]->type = kETAssemblerLineTypeIgnore;
                            break;
                        }

                        /* extracting hdr path token and type of hdr path token */
                        Boolean system_hdr = false;
                        char *hdr_token = __assembler_preprocessor_include_directive_get_token(inv->line[li]->token[1]->str, &system_hdr);
                        if(hdr_token == NULL)
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[1]), EFSTR("invalid token '%s' passed after %%include%% directive"), inv->line[li]->token[1]->str);
                            goto failure;
                        }

                        /* looking for da cat in the file system ^^ */
                        EFURLRef url = EFFileGetURL(EFArrayGetValueAtIndex(inv->files, inv->line[li]->file_idx));
                        EFAUTOREL EFStringRef path = EFURLCopyPath(EFGetAllocator(url), url);
                        char *hdr_path;
                        if(system_hdr)
                        {
                            hdr_path = assembler_code_find_system_header(hdr_token, inv->includeSearchPaths);
                        }
                        else
                        {
                            hdr_path = assembler_code_find_header(hdr_token, EFStringGetCStringPtr(path, kEFStringEncodingUTF8));
                        }

                        /* did I catch this cat >:3 */
                        if(hdr_path == NULL)
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[1]), EFSTR("couldn't find header at path '%s'"), hdr_token);
                            free(hdr_token);
                            goto failure;
                        }
                        free(hdr_token);

                        /* now openup a file */
                        EFAUTOREL EFStringRef hdrPathStr = EFStringCreateWithCString(EFGetAllocator(url), hdr_path, kEFStringEncodingUTF8);
                        EFAUTOREL EFFileRef file = EFFileCreateWithPath(EFGetAllocator(url), EFFilePolicyInData, hdrPathStr);
                        if(file == NULL)
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[1]), EFSTR("couldn't open header at path '%s'"), hdr_path);
                            free(hdr_path);
                            goto failure;
                        }

                        if(!assembler_code_inject_file(inv, li, file))
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityFatal, AT_TO_DLOC(inv->line[li]->token[1]), EFSTR("couldn't inject header at path '%s' into invocation"), hdr_path);
                            free(hdr_path);
                            goto failure;
                        }
                        free(hdr_path);
                        EFAUTOTRANSFER(file);   /* transferred at injection */

                        li--; /* file was inseted at this location */
                        break;
                    }
                    case kAssemblerPreprocessorDirectiveTypeDefine:
                        if(!assembler_macro_storage_append_macro(storage, inv->line[li]->token[1]->str, &inv->line[li]->token[2], inv->line[li]->token_cnt - 2))
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("out of memory, can't append macro to macro storage"));
                            goto failure;
                        }
                        break;
                    case kAssemblerPreprocessorDirectiveTypeUndefine:
                        assembler_macro_storage_remove_macro(storage, inv->line[li]->token[1]->str);
                        break;
                    case kAssemblerPreprocessorDirectiveTypeIf:
                    {
                        Boolean parent_active = !state.in_a_condition || state.condition_met;
                        if(!assembler_condition_state_push(&state))
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityFatal, AT_TO_DLOC(inv->line[li]->token[0]), EFSTR("failed to push condition frame onto condition state"));
                            goto failure;
                        }
                        state.parent_active = parent_active;

                    express_if_directive:
                        {
                            Boolean raw;
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
                        Boolean parent_active = !state.in_a_condition || state.condition_met;
                        if(!assembler_condition_state_push(&state))
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityFatal, AT_TO_DLOC(inv->line[li]->token[0]), EFSTR("failed to push condition frame onto condition state"));
                            goto failure;
                        }
                        state.parent_active = parent_active;

                        Boolean raw = false;
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
                        Boolean parent_active = !state.in_a_condition || state.condition_met;
                        if(!assembler_condition_state_push(&state))
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityFatal, AT_TO_DLOC(inv->line[li]->token[0]), EFSTR("failed to push condition frame onto condition state"));
                            goto failure;
                        }
                        state.parent_active = parent_active;

                        Boolean raw = true;
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
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[0]), EFSTR("%%elif%% directive was defined, but no %%if%% directive was defined before."));
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
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[0]), EFSTR("%%else%% directive was defined, but no %%if%% directive was defined before."));
                            goto failure;
                        }

                        if(state.in_a_else_condition)
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[0]), EFSTR("%%else%% directive was defined inside another %%else%% directive."));
                            goto failure;
                        }

                        state.condition_met = (!state.finalized && !state.condition_met) && state.parent_active;
                        state.in_a_else_condition = true;
                        break;
                    case kAssemblerPreprocessorDirectiveTypeEndIf:
                        if(state.in_a_condition == false)
                        {
                            ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[li]->token[0]), EFSTR("%%endif%% directive was defined, but no %%if%% directive was defined before."));
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
                    inv->line[li]->type = kETAssemblerLineTypeIgnore;
                }
                break;
            default:
                break;
        }
    }

    if(state.in_a_condition)
    {
        ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(state.last_condition_line->token[0]), EFSTR("%%if%% was defined but no matching %%endif%% was found."));
    }

    Boolean in_a_condition = state.in_a_condition;
    assembler_condition_state_deinit(&state);
    assembler_macro_storage_dealloc(storage);
    return !in_a_condition;

failure:
    assembler_condition_state_deinit(&state);
    assembler_macro_storage_dealloc(storage);
    return false;
}
