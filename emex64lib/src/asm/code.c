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
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <emex64lib/support/diagnostic/log.h>
#include <emex64lib/asm/preprocessor/directive.h>
#include <emex64lib/asm/code.h>
#include <emex64lib/asm/lexer.h>

typedef struct expand_entry {
    char *code;
    size_t len;
    size_t line_num;
} expand_entry_t;

static bool __assembler_code_fastline(emex_file_t *file,
                                      expand_entry_t **entries,
                                      size_t *cnt,
                                      size_t *cap)
{
    if(!emex_file_map(file))
    {
        diag_error(NULL, "failed to map assembly file '%s'\n", file->path);
        return false;
    }

    size_t len = file->len;
    const char *code = file->content;

    size_t start = 0;
    size_t phys_line = 0;
    for(size_t i = 0; i <= len; i++)
    {
        if(code[i] != '\n' && i != len)
        {
            continue;
        }

        phys_line++;

        size_t line_len = i - start;
        char *line = malloc(line_len + 1);
        memcpy(line, code + start, line_len);
        line[line_len] = '\0';
        start = i + 1;

        char *trimmed = line;
        while(*trimmed == ' ' || *trimmed == '\t')
        {
            trimmed++;
        }

        if(*cnt >= *cap)
        {
            *cap = (*cap) ? (*cap) * 2 : 64;
            *entries = realloc(*entries, (*cap) * sizeof(expand_entry_t));
        }
        (*entries)[*cnt].code = line;
        (*entries)[*cnt].len = line_len;
        (*entries)[*cnt].line_num = phys_line;
        (*cnt)++;
    }

    return true;
}

char *assembler_code_find_header(const char *name,
                                 const char *source_file)
{
    char dir_buf[PATH_MAX];
    snprintf(dir_buf, sizeof(dir_buf), "%s", source_file);
    char *slash = strrchr(dir_buf, '/');
    const char *source_dir = slash ? (slash[0] = '\0', dir_buf) : ".";

    char buf[PATH_MAX];
    if(source_dir)
    {
        snprintf(buf, sizeof(buf), "%s/%s", source_dir, name);
        if(access(buf, R_OK) == 0)
        {
            return strdup(buf);
        }
    }
    return NULL;
}

char *assembler_code_find_system_header(const char *name,
                                        const char **inc_dirs,
                                        size_t inc_cnt)
{
    char buf[PATH_MAX];
    for(size_t i = 0; i < inc_cnt; i++)
    {
        snprintf(buf, sizeof(buf), "%s/%s", inc_dirs[i], name);
        if(access(buf, R_OK) == 0)
        {
            return strdup(buf);
        }
    }
    return NULL;
}

static inline bool __assembler_splice_line(assembler_invocation_t *inv,
                                           uint64_t idx,
                                           size_t count)
{
    /* bounds check */
    if(idx >= inv->line_cnt)
    {
        return false;
    }

    /* realloc */
    size_t new_cnt = inv->line_cnt - 1 + count;
    if(new_cnt > inv->line_cnt)
    {
        assembler_line_t **tmp = realloc(inv->line, (new_cnt + 1) * sizeof *tmp);
        if(!tmp)
        {
            return false;
        }
        inv->line = tmp;
    }

    /* deallocating the line at the idx */
    free(inv->line[idx]->str);
    for(uint64_t i = 0; i < inv->line[idx]->token_cnt; i++)
    {
        if(inv->line[idx]->token[i]->type == kAssemblerTokenTypeString)
        {
            free(inv->line[idx]->token[i]->string_literal.buf);
        }

        free(inv->line[idx]->token[i]->str);
        free(inv->line[idx]->token[i]);
    }
    free(inv->line[idx]->token);
    free(inv->line[idx]);

    /* shift the tail */
    memmove(&inv->line[idx + count], &inv->line[idx + 1], (inv->line_cnt - idx - 1) * sizeof *inv->line);
    for(size_t i = 0; i < count; i++)
    {
        inv->line[idx + i] = NULL;
    }

    inv->line_cnt = new_cnt;
    return true;
}

bool assembler_code_inject_file(assembler_invocation_t *inv,
                                uint64_t at_line_index,
                                emex_file_t *inj_file)
{
    /* getting code */
    expand_entry_t *entries = NULL;
    size_t entry_cnt = 0, entry_cap = 0;

    if(!__assembler_code_fastline(inj_file, &entries, &entry_cnt, &entry_cap))
    {
        goto out_failure;
    }

    /* injecting file into array */
    uint64_t inj_file_idx;
    emex_file_t **newp = realloc(inv->file, (inv->file_cnt + 1) *  sizeof(emex_file_t*));
    if(newp == NULL)
    {
        goto out_failure;
    }
    inv->file = newp;
    inv->file[inv->file_cnt] = inj_file;
    inj_file_idx = inv->file_cnt++;

    /* handling tokenization and preparse */
    if(inv->line_cnt != 0)
    {
        if(!__assembler_splice_line(inv, at_line_index, entry_cnt))
        {
            goto out_failure_file_rm;
        }
    }
    else
    {
        inv->line = calloc(entry_cnt, sizeof(assembler_line_t*));
        if(inv->line == NULL)
        {
            goto out_failure_file_rm;
        }
        inv->line_cnt = entry_cnt;
    }

    /* inject additional lines */
    for(size_t i = 0; i < entry_cnt; i++)
    {
        /* FIXME: not reversable for now */
        assembler_line_t *al = calloc(1, sizeof(assembler_line_t));
        al->str = entries[i].code;
        al->line_num = entries[i].line_num;
        al->file_idx = inj_file_idx;
        al->inv = inv;
        inv->line[at_line_index + i] = al;
    }

    /* getting subtokens of each token */
    for(size_t i = 0; i < entry_cnt; i++)
    {
        /* using cmptok in first pass to get token count */
        for(lextok_token_t token = assembler_lexer_tok(inv->line[at_line_index + i]->str); token.token != NULL; token = assembler_lexer_tok(NULL))
        {
            /*
             * until this is not null i will not move
             * anywhere else than my safe space which
             * is this while loop :3
             */
            inv->line[at_line_index + i]->token_cnt++;
        }

        /* copy subtokens */
        inv->line[at_line_index + i]->token = calloc(inv->line[at_line_index + i]->token_cnt, sizeof(assembler_token_t*));
        inv->line[at_line_index + i]->token_cnt = 0;

        /*
         * again doing the same dance, over and over
         * and over again, is this a carousell or
         * why am I getting ill rn.
         */
        for(lextok_token_t token = assembler_lexer_tok(inv->line[at_line_index + i]->str); token.token != NULL; token = assembler_lexer_tok(NULL))
        {
            assembler_token_t *at = calloc(1, sizeof(assembler_token_t));
            at->str = strdup(token.token);
            at->column_num = token.column + 1;
            at->al = inv->line[at_line_index + i];
            if(token.type == kAssemblerTokenTypeInvalid)
            {
                diag_error(at, "token '%s' is not valid\n", at->str);
                inv->file[inj_file_idx] = NULL;
                return false;
            }
            else if(token.type == kAssemblerTokenTypeTooLong)
            {
                diag_error(at, "token is too long, token lenght limit is %d characters\n", LEXTOK_LENGHT_MAX);
                inv->file[inj_file_idx] = NULL;
                return false;
            }
            at->type = token.type;
            inv->line[at_line_index + i]->token[inv->line[at_line_index + i]->token_cnt++] = at;
        }
    }

    /* token pretype evaluation (for macros) */
    for(size_t i = 0; i < entry_cnt; i++)
    {
        if(inv->line[at_line_index + i]->token_cnt == 0)
        {
            continue;
        }

        /* if it has a valid preprocessor directive type then it is a preprocessor directive */
        if(assembler_directive_type_for_str(inv->line[at_line_index + i]->token[0]->str) != kAssemblerPreprocessorDirectiveTypeUnknown)
        {
            inv->line[at_line_index + i]->type = kAssemblerLineTypePreprocessorDirective;
        }
    }

    free(entries);
    return true;

out_failure_file_rm:
    /* preventing evObj issues */
    inv->file[inj_file_idx] = NULL;
out_failure:
    for(size_t i = 0; i < entry_cnt; i++)
    {
        free(entries[i].code);
    }
    free(entries);
    return false;
}

bool assembler_code_preparse(assembler_invocation_t *inv,
                             emex_file_t *input)
{
    if(!assembler_code_inject_file(inv, 0, input))
    {
        diag_fatal(NULL, "couldn't parse file at '%s'\n", input->path);
        return false;
    }

    return true;
}

bool assembler_code_postparse(assembler_invocation_t *inv)
{
    /* token type emitter */
    for(uint64_t li = 0; li < inv->line_cnt; li++)
    {
        if(inv->line[li]->token_cnt == 0 ||
           inv->line[li]->type == kAssemblerLineTypeIgnore)
        {
            /* probably a whitespace or excluded by a macro */
            continue;
        }

        for(uint64_t ti = 0; ti < inv->line[li]->token_cnt; ti++)
        {
            if(!assembler_lexer_classify(inv->line[li]->token[ti]))
            {
                /* callee emitted error */
                return false;
            }
        }
    }

    /* token type evaluation */
    bool section_mode = false;
    for(uint64_t i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->token_cnt == 0 ||
           inv->line[i]->type == kAssemblerLineTypeIgnore)
        {
            /* probably a whitespace or excluded by a macro */
            continue;
        }
        
        if(inv->line[i]->token_cnt >= 2)
        {
            /*
             * checking if last character of token is a ':',
             * because that means that its a label.
             */
            if(inv->line[i]->token[1]->type == kAssemblerTokenTypeColon)
            {
                section_mode = false;

                /*
                 * checking what type of label it is
                 *
                 * note: '_example' for example would be a global
                 *       label, which means it can be called by
                 *       any symbol in the same program, while
                 *       '.example' is a local label which can only
                 *       be called within the same global label's code. 
                 */
                switch(inv->line[i]->token[0]->str[0])
                {
                    case '.':
                        inv->line[i]->type = kAssemblerLineTypeLocalLabel;
                        break;
                    default:
                        inv->line[i]->type = kAssemblerLineTypeGlobalLabel;
                        break;
                }

                if(inv->line[i]->token[0]->type != kAssemblerTokenTypeIdentifier)
                {
                    diag_error(inv->line[i]->token[0], "expected identifier in label definition, but got %s '%s'\n", assembler_lexer_str_for_token_type(inv->line[i]->token[0]->type), inv->line[i]->token[0]->str);
                    return false;
                }

                for(uint64_t j = 3; j < inv->line[i]->token_cnt; j++)
                {
                    diag_error(inv->line[i]->token[j], "unexpected %s '%s' after label definition\n", assembler_lexer_str_for_token_type(inv->line[i]->token[j]->type), inv->line[i]->token[j]->str);
                    return false;
                }

                continue;
            }
        }

        if(inv->line[i]->token_cnt >= 2 && strcmp(inv->line[i]->token[0]->str, "extern") == 0)
        {
            section_mode = true;
            inv->line[i]->type = kAssemblerLineTypeExternLabel;
            continue;
        }
        
        if(inv->line[i]->token_cnt < 3 && strcmp(inv->line[i]->token[0]->str, "section") == 0)
        {
            section_mode = true;
            inv->line[i]->type = kAssemblerLineTypeSection;
            continue;
        }

        /*
         * it is either part of a section or
         * assembly, this is a very important
         * differentiation. 
         */
        inv->line[i]->type = section_mode ? kAssemblerLineTypeSectionData : kAssemblerLineTypeAssembly;
    }

    return true;
}
