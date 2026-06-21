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

#include <emex64lib/support/diagnostic/legacy.h>

#include <emex64lib/asm/code.h>
#include <emex64lib/asm/lexer.h>

typedef struct expand_entry {
    char *source_path;
    char *code;
    size_t len;
    size_t line_num;
} expand_entry_t;

static char *find_header(const char *name,
                         const char *source_dir,
                         const char **inc_dirs,
                         size_t inc_cnt)
{
    char buf[PATH_MAX];

    if(source_dir)
    {
        snprintf(buf, sizeof(buf), "%s/%s", source_dir, name);
        if(access(buf, R_OK) == 0)
        {
            return strdup(buf);
        }
    }

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

static bool expand_file(emex_file_t *file,
                        expand_entry_t **entries,
                        size_t *cnt,
                        size_t *cap,
                        const char **inc_dirs,
                        size_t inc_cnt,
                        int depth)
{
    if(depth > 1024)
    {
        diag_error(NULL, "include recurse limit exceeded near '%s'\n", file->path);
        return false;
    }

    if(!emex_file_map(file))
    {
        diag_error(NULL, "failed to map assembly file '%s'\n", file->path);
        return false;
    }

    size_t len = file->len;
    const char *code = file->content;

    char dir_buf[PATH_MAX];
    snprintf(dir_buf, sizeof(dir_buf), "%s", file->path);
    char *slash = strrchr(dir_buf, '/');
    const char *source_dir = slash ? (slash[0] = '\0', dir_buf) : ".";

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

        if(strncmp(trimmed, "%include%", 9) == 0)
        {
            char *arg = trimmed + 9;
            while(*arg == ' ' || *arg == '\t')
            {
                arg++;
            }

            char hdr_name[PATH_MAX] = {0};
            bool is_system = false;
            if(*arg == '<')
            {
                is_system = true;
                char *end = strchr(arg + 1, '>');
                if(!end)
                {
                    diag_error(NULL, "malformed %%include%% (missing '>') in '%s'\n", file->path);
                    free(line);
                    return false;
                }
                size_t nlen = (size_t)(end - arg - 1);
                memcpy(hdr_name, arg + 1, nlen);
                hdr_name[nlen] = '\0';
            }
            else if(*arg == '"')
            {
                char *end = strchr(arg + 1, '"');
                if(!end)
                {
                    diag_error(NULL, "malformed %%include%% (missing '\"') in '%s'\n", file->path);
                    free(line);
                    return false;
                }
                size_t nlen = (size_t)(end - arg - 1);
                memcpy(hdr_name, arg + 1, nlen);
                hdr_name[nlen] = '\0';
            }
            else
            {
                diag_error(NULL, "malformed %%include%% directive in '%s'\n", file->path);
                free(line);
                return false;
            }

            /* resolve path */
            char *hdr_path = find_header(hdr_name, is_system ? NULL : source_dir, inc_dirs, inc_cnt);
            if(!hdr_path)
            {
                diag_error(NULL, "header '%s' not found (included from '%s')\n", hdr_name, file->path);
                free(line);
                return false;
            }

            free(line);

            emex_file_t *file = emex_file_alloc(hdr_path, in_data_file_policy);
            free(hdr_path);
            if(file == NULL)
            {
                return false;
            }

            /* recurse */
            bool ok = expand_file(file, entries, cnt, cap, inc_dirs, inc_cnt, depth + 1);
            emex_file_dealloc(file);
            if(!ok)
            {
                return false;
            }
            continue;
        }

        if(*cnt >= *cap)
        {
            *cap = (*cap) ? (*cap) * 2 : 64;
            *entries = realloc(*entries, (*cap) * sizeof(expand_entry_t));
        }
        (*entries)[*cnt].source_path = strdup(file->path);
        (*entries)[*cnt].code = line;
        (*entries)[*cnt].len = line_len;
        (*entries)[*cnt].line_num = phys_line;
        (*cnt)++;
    }

    return true;
}

bool assembler_code_preparse(assembler_invocation_t *inv,
                             emex_file_t *input)
{
    /*
     * preparing compiler invocation to
     * map and parse files.
     */
    expand_entry_t *entries = NULL;
    size_t entry_cnt = 0, entry_cap = 0;

    if(!expand_file(input, &entries, &entry_cnt, &entry_cap, (const char **)inv->include_dirs, inv->include_dir_cnt, 0))
    {
        free(entries);
        return false;
    }

    inv->file_cnt = 0;
    inv->file = NULL;

    for(size_t i = 0; i < entry_cnt; i++)
    {
        const char *sp = entries[i].source_path;
        bool found = false;
        for(size_t j = 0; j < inv->file_cnt; j++)
        {
            if(strcmp(inv->file[j], sp) == 0)
            {
                found = true; break;
            }
        }
        if(!found)
        {
            inv->file = realloc(inv->file, (inv->file_cnt + 1) * sizeof(emex_file_t*));
            inv->file[inv->file_cnt++] = strdup(sp);
        }
    }

    inv->line = calloc(entry_cnt + 1, sizeof(assembler_line_t*));
    inv->line_cnt = 0;

    for(size_t i = 0; i < entry_cnt; i++)
    {
        size_t file_idx = 0;
        for(size_t j = 0; j < inv->file_cnt; j++)
        {
            if(strcmp(inv->file[j], entries[i].source_path) == 0)
            {
                file_idx = j;
                break;
            }
        }

        assembler_line_t *al = calloc(1, sizeof(assembler_line_t));
        al->str = entries[i].code;
        al->line_num = entries[i].line_num;
        al->file_idx = file_idx;
        al->inv = inv;
        inv->line[inv->line_cnt++] = al;
        free(entries[i].source_path);
    }
    free(entries);

    /* getting subtokens of each token */
    for(unsigned long i = 0; i < inv->line_cnt; i++)
    {
        /* using cmptok in first pass to get token count */
        for(lextok_token_t token = lextok(inv->line[i]->str); token.token != NULL;)
        {
            /*
             * until this is not null i will not move
             * anywhere else than my safe space which
             * is this while loop :3
             */
            inv->line[i]->token_cnt++;
            token = lextok(NULL);
        }

        /* copy subtokens */
        inv->line[i]->token = calloc(inv->line[i]->token_cnt, sizeof(assembler_token_t*));
        inv->line[i]->token_cnt = 0;

        /*
         * again doing the same dance, over and over
         * and over again, is this a carousell or
         * why am I getting ill rn.
         */
        for(lextok_token_t token = lextok(inv->line[i]->str); token.token != NULL;)
        {
            assembler_token_t *at = calloc(1, sizeof(assembler_token_t));
            at->str = strdup(token.token);
            at->column_num = token.column + 1;
            at->al = inv->line[i];
            if(token.type == kAssemblerTokenTypeInvalid)
            {
                diag_error(at, "Token '%s' is not valid\n", at->str);
                return false;
            }
            else if(token.type == kAssemblerTokenTypeTooLong)
            {
                diag_error(at, "Token is too long, token lenght limit is %d characters\n", LEXTOK_LENGHT_MAX);
                return false;
            }
            at->type = token.type;
            inv->line[i]->token[inv->line[i]->token_cnt++] = at;
            token = lextok(NULL);
        }
    }

    /* token pretype evaluation (for macros) */
    for(unsigned long i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->token_cnt == 0) continue;

        if(strcmp(inv->line[i]->token[0]->str, "%define%") == 0)
        {
            inv->line[i]->type = kAssemblerLineTypeDefinitionDirective;
        }
        else if(strcmp(inv->line[i]->token[0]->str, "%if%") == 0 ||
                strcmp(inv->line[i]->token[0]->str, "%elif%") == 0 ||
                strcmp(inv->line[i]->token[0]->str, "%else%") == 0 ||
                strcmp(inv->line[i]->token[0]->str, "%endif%") == 0 ||
                strcmp(inv->line[i]->token[0]->str, "%ifdef%") == 0 ||
                strcmp(inv->line[i]->token[0]->str, "%ifndef%") == 0)
        {
            inv->line[i]->type = kAssemblerLineTypeConditionDirective;
        }
    }

    return true;
}

bool assembler_code_parse(assembler_invocation_t *inv)
{
    /* token type evaluation */
    bool section_mode = false;
    for(unsigned long i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->token_cnt == 0 ||
           inv->line[i]->type == kAssemblerLineTypeIgnore)
        {
            /* probably a whitespace or excluded by a macro */
            continue;
        }
        
        if(inv->line[i]->token_cnt >= 1)
        {
            /* getting size of subtoken */
            size_t size = strlen(inv->line[i]->token[0]->str);
            if(size == 0)
            {
                /* invalid size */
                continue;
            }

            /*
             * checking if last character of token is a ':',
             * because that means that its a label.
             */
            if(inv->line[i]->token[0]->str[size - 1] == ':')
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
                    case '_':
                        inv->line[i]->type = kAssemblerLineTypeGlobalLabel;
                        break;
                    case '.':
                        inv->line[i]->type = kAssemblerLineTypeLocalLabel;
                        break;
                    default:
                        diag_error(inv->line[i]->token[0], "illegal label definition '%s'\n", inv->line[i]->token[0]->str);
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
