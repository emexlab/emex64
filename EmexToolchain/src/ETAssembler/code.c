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
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETAssembler/preprocessor/directive.h>
#include <EmexToolchain/ETAssembler/code.h>
#include <EmexToolchain/ETAssembler/lexer.h>

typedef struct expand_entry {
    char *code;
    EFSize len;
    EFSize line_num;
} expand_entry_t;

static Boolean __assembler_code_fastline(EFFileRef file,
                                         expand_entry_t **entries,
                                         EFSize *cnt,
                                         EFSize *cap)
{
    EFAUTOREL EFFileHandleRef fileHandle = EFFileCopyFileHandle(EFGetAllocator(file), file);
    EFAUTOREL EFMappingRef mapping = EFFileHandleCopyMapping(EFGetAllocator(file), fileHandle);
    if(mapping == NULL)
    {
        return false;
    }

    EFSize size = (EFSize)EFMappingGetSize(mapping);
    const char *code = (const char*)EFMappingGetAddress(mapping);

    EFSize start = 0;
    EFSize phys_line = 0;
    for(EFSize i = 0; i <= size; i++)
    {
        if(code[i] != '\n' && i != size)
        {
            continue;
        }

        phys_line++;

        EFSize line_len = i - start;
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

    if(source_dir)
    {
        char buf[PATH_MAX];
        SInt32 n = snprintf(buf, sizeof(buf), "%s/%s", source_dir, name);
        if(n < 0 || (EFSize)n >= sizeof(buf))
        {
            return NULL;
        }
        if(access(buf, R_OK) == 0)
        {
            return strdup(buf);
        }
        return NULL;
    }
    return NULL;
}

char *assembler_code_find_system_header(const char *name,
                                        EFArrayRef includeSearchPaths)
{
    char buf[PATH_MAX];
    EFIndex includeSearchPathCount = EFArrayGetCount(includeSearchPaths);
    for(EFIndex index = 0; index < includeSearchPathCount; index++)
    {
        snprintf(buf, sizeof(buf), "%s/%s", EFStringGetCStringPtr(EFArrayGetValueAtIndex(includeSearchPaths, index), kEFStringEncodingUTF8), name);
        if(access(buf, R_OK) == 0)
        {
            return strdup(buf);
        }
    }
    return NULL;
}

static inline Boolean __assembler_splice_line(ETAssemblerInvocationRef inv,
                                              UInt64 idx,
                                              EFSize count)
{
    /* bounds check */
    if(idx >= inv->line_cnt)
    {
        return false;
    }

    /* realloc */
    EFSize new_cnt = inv->line_cnt - 1 + count;
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
    for(UInt64 i = 0; i < inv->line[idx]->token_cnt; i++)
    {
        if(inv->line[idx]->token[i]->type == kETAssemblerTokenTypeString)
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
    for(EFSize i = 0; i < count; i++)
    {
        inv->line[idx + i] = NULL;
    }

    inv->line_cnt = new_cnt;
    return true;
}

Boolean assembler_code_inject_file(ETAssemblerInvocationRef inv,
                                   UInt64 at_line_index,
                                   EFFileRef inj_file)
{
    /* getting code */
    expand_entry_t *entries = NULL;
    EFSize entry_cnt = 0, entry_cap = 0;

    if(!__assembler_code_fastline(inj_file, &entries, &entry_cnt, &entry_cap))
    {
        goto out_failure;
    }

    /* injecting file into array */
    EFIndex inj_file_idx = EFArrayGetCount(inv->files);
    if(!EFArrayAppendValue(inv->files, inj_file))
    {
        goto out_failure;
    }

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
            at->real_len = strlen(at->str);
            at->al = inv->line[at_line_index + i];
            if(token.type == kETAssemblerTokenTypeInvalid)
            {
                ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(at), EFSTR("token '%s' is not valid"), at->str);
                EFArrayRemoveValueAtIndex(inv->files, inj_file_idx);
                return false;
            }
            else if(token.type == kETAssemblerTokenTypeTooLong)
            {
                ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(at), EFSTR("token is too long, token lenght limit is %d characters"), LEXTOK_LENGHT_MAX);
                EFArrayRemoveValueAtIndex(inv->files, inj_file_idx);
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
            inv->line[at_line_index + i]->type = kETAssemblerLineTypePreprocessorDirective;
        }
    }

    free(entries);
    return true;

out_failure_file_rm:
    /* preventing evObj issues */
    EFArrayRemoveValueAtIndex(inv->files, inj_file_idx);
out_failure:
    for(size_t i = 0; i < entry_cnt; i++)
    {
        free(entries[i].code);
    }
    free(entries);
    return false;
}

Boolean assembler_code_preparse(ETAssemblerInvocationRef inv,
                                EFFileRef input)
{
    if(!assembler_code_inject_file(inv, 0, input))
    {
        ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityFatal, NULL, EFSTR("couldn't parse file at '%@'"), EFURLGetPath(EFFileGetURL(input)));
        return false;
    }

    return true;
}

Boolean assembler_code_postparse(ETAssemblerInvocationRef inv)
{
    /* token type emitter */
    for(UInt64 li = 0; li < inv->line_cnt; li++)
    {
        if(inv->line[li]->token_cnt == 0 ||
           inv->line[li]->type == kETAssemblerLineTypeIgnore)
        {
            /* probably a whitespace or excluded by a macro */
            continue;
        }

        for(UInt64 ti = 0; ti < inv->line[li]->token_cnt; ti++)
        {
            if(!assembler_lexer_classify(inv->line[li]->token[ti]))
            {
                /* callee emitted error */
                return false;
            }
        }
    }

    /* token type evaluation */
    Boolean section_mode = false;
    for(UInt64 i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->token_cnt == 0 ||
           inv->line[i]->type == kETAssemblerLineTypeIgnore)
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
            if(inv->line[i]->token[1]->type == kETAssemblerTokenTypeColon)
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
                        inv->line[i]->type = kETAssemblerLineTypeLabel;
                        break;
                    default:
                        inv->line[i]->type = kETAssemblerLineTypeSymbol;
                        break;
                }

                /* post validity check */
                Boolean valid = true;
                if(inv->line[i]->token[0]->type != kETAssemblerTokenTypeIdentifier)
                {
                    ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[0]), EFSTR("expected identifier in label definition, but got %s '%s'"), assembler_lexer_str_for_token_type(inv->line[i]->token[0]->type), inv->line[i]->token[0]->str);
                    valid = false;
                }

                for(UInt64 j = 2; j < inv->line[i]->token_cnt; j++)
                {
                    ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[j]), EFSTR("unexpected %s '%s' after label definition"), assembler_lexer_str_for_token_type(inv->line[i]->token[j]->type), inv->line[i]->token[j]->str);
                    valid = false;
                }
                if(!valid)
                {
                    return false;
                }

                continue;
            }
        }

        if(inv->line[i]->token_cnt >= 1 && inv->line[i]->token[0]->type == kETAssemblerTokenTypeKeyword)
        {
            if(inv->line[i]->token_cnt == 1)
            {
                ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[0]), EFSTR("expected identifier after %s '%s'"), assembler_lexer_str_for_token_type(inv->line[i]->token[0]->type), inv->line[i]->token[0]->str);
                return false;
            }

            switch(inv->line[i]->token[0]->keyword.v)
            {
                case kETAssemblerKeywordSection:
                    section_mode = true;
                    inv->line[i]->type = kETAssemblerLineTypeSection;
                    break;
                case kETAssemblerKeywordExtern:
                    inv->line[i]->type = kETAssemblerLineTypeExternSymbol;
                    break;
                default:
                    break;
            }

            Boolean valid = true;

            if(inv->line[i]->token[1]->type != kETAssemblerTokenTypeIdentifier)
            {
                ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[1]), EFSTR("expected identifier in keyword construct, but got %s '%s'"), assembler_lexer_str_for_token_type(inv->line[i]->token[1]->type), inv->line[i]->token[1]->str);
                valid = false;
            }

            for(UInt64 j = 2; j < inv->line[i]->token_cnt; j++)
            {
                /* idk keyword construct, keyword definition ahhhhhh */
                ETAssemblerDiagnosticConsumerReport(inv->diagnosticConsumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[j]), EFSTR("unexpected %s '%s' after keyword construct"), assembler_lexer_str_for_token_type(inv->line[i]->token[j]->type), inv->line[i]->token[j]->str);
                valid = false;
            }

            if(!valid)
            {
                return false;
            }

            continue;
        }

        /*
         * it is either part of a section or
         * assembly, this is a very important
         * differentiation.
         */
        inv->line[i]->type = section_mode ? kETAssemblerLineTypeSectionData : kETAssemblerLineTypeAssembly;
    }

    return true;
}
