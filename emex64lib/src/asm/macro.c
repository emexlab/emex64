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
#include <stdbool.h>

#include <emex64lib/support/diagnostic/legacy.h>
#include <emex64lib/support/parser.h>
#include <emex64lib/support/pack.h>

#include <emex64lib/asm/macro.h>
#include <emex64lib/asm/code.h>

typedef struct assembler_macro {
    const char *match;          /* borrowed */
    const char **inject_token;  /* borrowed */
    uint64_t inject_token_cnt;
    struct assembler_macro *next;
} assembler_macro_t;

typedef struct assembler_macro_storage {
    assembler_macro_t *head;
    assembler_macro_t *tail;
} assembler_macro_storage_t;

assembler_macro_t *assembler_macro_alloc(const char *match,
                                         const char **inject_token,
                                         uint64_t token_cnt)
{
    assembler_macro_t *macro = malloc(sizeof(assembler_macro_t));
    if(macro == NULL)
    {
        return NULL;
    }

    macro->inject_token = inject_token;
    macro->inject_token_cnt = token_cnt;
    macro->match = match;
    macro->next = NULL;

    return macro;
}

void assembler_macro_dealloc(assembler_macro_t *macro)
{
    free(macro);
}

assembler_macro_storage_t *assembler_macro_storage_alloc()
{
    assembler_macro_storage_t *storage = malloc(sizeof(assembler_macro_storage_t));
    if(storage == NULL)
    {
        return NULL;
    }

    storage->head = NULL;
    storage->tail = NULL;

    return storage;
}

void assembler_macro_storage_dealloc(assembler_macro_storage_t *storage)
{
    while(storage->head != NULL)
    {
        assembler_macro_t *next = storage->head->next;
        free(storage->head->inject_token);
        assembler_macro_dealloc(storage->head);
        storage->head = next;
    }

    free(storage);
}

assembler_macro_t *assembler_macro_storage_lookup(assembler_macro_storage_t *storage,
                                                  const char *match)
{
    assembler_macro_t *head = storage->head; 
    while(head != NULL)
    {
        if(strcmp(head->match, match) == 0)
        {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

bool assembler_macro_storage_append_macro_char(assembler_macro_storage_t *storage,
                                               const char *match,
                                               const char **token,
                                               uint64_t token_cnt)
{
    /* checking if it is already defined */
    assembler_macro_t *found = assembler_macro_storage_lookup(storage, match);
    if(found != NULL)
    {
        /* inject information */
        free(found->inject_token);
        found->inject_token = token;
        found->inject_token_cnt = token_cnt;
        return true;
    }

    /* need new macro */
    assembler_macro_t *macro = assembler_macro_alloc(match, token, token_cnt);
    if(macro == NULL)
    {
        return false;
    }

    /* stich the linked list ^^ */
    if(storage->head == NULL)
    {
        storage->head = macro;
    }
    else
    {
        storage->tail->next = macro;
    }
    storage->tail = macro;

    return true;
}

bool assembler_macro_storage_append_macro(assembler_macro_storage_t *storage,
                                          const char *match,
                                          assembler_token_t **token,
                                          uint64_t token_cnt)
{
    /* checking if it is already defined */
    const char **token_char = calloc(token_cnt, sizeof(const char *));
    if(token_char == NULL)
    {
        return false;
    }

    for(uint64_t i = 0; i < token_cnt; i++)
    {
        token_char[i] = token[i]->str;
    }

    bool success = assembler_macro_storage_append_macro_char(storage, match, token_char, token_cnt);
    if(!success)
    {
        free(token_char);
    }
    return success;
}

typedef struct assembler_directive_condition_frame {
    bool in_a_condition;
    bool condition_met;
    bool in_a_else_condition;
    bool finalized;
    assembler_line_t *last_condition_line;
    struct assembler_directive_condition_frame *prev;
} assembler_condition_frame_t;

static inline assembler_condition_frame_t *__assembler_condition_frame_alloc()
{
    assembler_condition_frame_t *frame = malloc(sizeof(assembler_condition_frame_t));
    if(frame == NULL)
    {
        return frame;
    }
    
    frame->prev = NULL;
    
    return frame;
}

void assembler_condition_frame_dealloc(assembler_condition_frame_t **frame)
{
    assembler_condition_frame_t *tail = *frame;
    while(tail != NULL)
    {
        assembler_condition_frame_t *prev = tail->prev;
        free(tail);
        tail = prev;
    }
}

bool assembler_condition_frame_push(assembler_condition_frame_t **frame)
{
    assembler_condition_frame_t *new = __assembler_condition_frame_alloc();
    if(new == NULL)
    {
        goto out_failure;
    }
    
    new->prev = *frame;
    *frame = new;
    
    return true;
    
out_failure:
    assembler_condition_frame_dealloc(frame);
    return false;
}

void assembler_condition_frame_pop(assembler_condition_frame_t **frame)
{
    if(*frame == NULL)
    {
        return;
    }
    
    assembler_condition_frame_t *prev = (*frame)->prev;
    free(*frame);
    *frame = prev;
}

typedef struct assembler_directive_condition_state {
    assembler_condition_frame_t *frame;
    bool in_a_condition;
    bool condition_met;
    bool in_a_else_condition;
    bool finalized;
    assembler_line_t *last_condition_line;
} assembler_condition_state_t;

void assembler_condition_state_init(assembler_condition_state_t *state)
{
    bzero(state, sizeof(assembler_condition_state_t));
}

void assembler_condition_state_deinit(assembler_condition_state_t *state)
{
    assembler_condition_frame_dealloc(&state->frame);
    bzero(state, sizeof(assembler_condition_state_t));
}

bool assembler_condition_state_push(assembler_condition_state_t *state)
{
    if(!assembler_condition_frame_push(&state->frame))
    {
        return false;
    }
    
    state->frame->condition_met = state->condition_met;
    state->frame->in_a_condition = state->in_a_condition;
    state->frame->in_a_else_condition = state->in_a_else_condition;
    state->frame->finalized = state->finalized;
    state->frame->last_condition_line = state->last_condition_line;
    
    state->condition_met = false;
    state->in_a_condition = false;
    state->in_a_else_condition = false;
    state->finalized = false;
    state->last_condition_line = NULL;
    
    return true;
}

void assembler_condition_state_pop(assembler_condition_state_t *state)
{
    state->condition_met = state->frame->condition_met;
    state->in_a_condition = state->frame->in_a_condition;
    state->in_a_else_condition = state->frame->in_a_else_condition;
    state->finalized = state->frame->finalized;
    state->last_condition_line = state->frame->last_condition_line;
    
    assembler_condition_frame_pop(&state->frame);
}

bool assembler_macro_expand(assembler_invocation_t *inv)
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
                if(!state.finalized)
                {
                    switch(pack_name(inv->line[li]->token[0]->str))
                    {
                        case PACK('%','i','f','%'):
                            if(!assembler_condition_state_push(&state))
                            {
                                diag_fatal(inv->line[li]->token[0], "failed to push condition frame onto condition state\n");
                                assembler_condition_state_deinit(&state);
                                assembler_macro_storage_dealloc(storage);
                                return false;
                            }
                            
                        if_macro_condition_expand:
                        {
                            if(inv->line[li]->token_cnt >= 2 && strlen(inv->line[li]->token[1]->str) > 0)
                            {
                                parser_return_t pret = parse_value_from_string(inv->line[li]->token[1]->str);
                                state.condition_met = pret.type == emexParserValueTypeNumber ? (pret.value != 0) : true;
                            }
                            else
                            {
                                state.condition_met = false;
                            }
                            
                            state.in_a_condition = true;
                        }
                            break;
                        case PACK('%','i','f','d','e','f','%'):
                            if(!assembler_condition_state_push(&state))
                            {
                                diag_fatal(inv->line[li]->token[0], "failed to push condition frame onto condition state\n");
                                assembler_condition_state_deinit(&state);
                                assembler_macro_storage_dealloc(storage);
                                return false;
                            }
                            
                            state.condition_met = false;
                            if(inv->line[li]->token_cnt >= 2 && strlen(inv->line[li]->token[1]->str) > 0)
                            {
                                assembler_macro_t *found_match = assembler_macro_storage_lookup(storage, inv->line[li]->token[1]->str);
                                state.condition_met = found_match != NULL;
                            }
                            
                            state.in_a_condition = true;
                            break;
                        case PACK('%','i','f','n','d','e','f','%'):
                            if(!assembler_condition_state_push(&state))
                            {
                                diag_fatal(inv->line[li]->token[0], "failed to push condition frame onto condition state\n");
                                assembler_condition_state_deinit(&state);
                                assembler_macro_storage_dealloc(storage);
                                return false;
                            }
                            
                            state.condition_met = true;
                            if(inv->line[li]->token_cnt >= 2 && strlen(inv->line[li]->token[1]->str) > 0)
                            {
                                assembler_macro_t *found_match = assembler_macro_storage_lookup(storage, inv->line[li]->token[1]->str);
                                state.condition_met = found_match == NULL;
                            }
                            
                            state.in_a_condition = true;
                            break;
                        case PACK('%','e','l','i','f','%'):
                            if(state.in_a_condition == false)
                            {
                                diag_error(inv->line[li]->token[0], "%%elif%% was defined but no %%if%% was defined prior.\n");
                                assembler_condition_state_deinit(&state);
                                assembler_macro_storage_dealloc(storage);
                                return false;
                            }
                            
                            if(!state.condition_met)
                            {
                                goto if_macro_condition_expand;
                            }
                            else
                            {
                                state.finalized = true;
                                state.condition_met = false;
                            }
                            break;
                        case PACK('%','e','l','s','e','%'):
                            if(state.in_a_condition == false)
                            {
                                diag_error(inv->line[li]->token[0], "%%elseif%% was defined but no %%if%% was defined prior.\n");
                                assembler_condition_state_deinit(&state);
                                assembler_macro_storage_dealloc(storage);
                                return false;
                            }
                            
                            if(!state.condition_met)
                            {
                                state.condition_met = true;
                            }
                            else
                            {
                                state.finalized = true;
                                state.condition_met = false;
                            }
                            break;
                        default:
                            break;
                    }
                }
                
                if(strcmp(inv->line[li]->token[0]->str, "%endif%") == 0)
                {
                    if(state.in_a_condition == false)
                    {
                        diag_error(inv->line[li]->token[0], "%%endif%% was defined but no %%if%% was defined prior.\n");
                        assembler_condition_state_deinit(&state);
                        assembler_macro_storage_dealloc(storage);
                        return false;
                    }
                    assembler_condition_state_pop(&state);
                }

                state.last_condition_line = inv->line[li];
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
