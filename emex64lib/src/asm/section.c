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
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>

#include <emex64lib/support/parser.h>
#include <emex64lib/support/bitwalker.h>
#include <emex64lib/support/diagnostic/legacy.h>
#include <emex64lib/support/file.h>

#include <emex64lib/asm/section.h>
#include <emex64lib/asm/code.h>

static inline unsigned long align_up(unsigned long v, unsigned long a)
{
    return (v + a - 1) & ~(a - 1);
}

bool assembler_section_parse(assembler_invocation_t *inv)
{
    /* iterating for section token type and creating data section */
    for(unsigned long i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->type == kAssemblerLineTypeSection)
        {
            if(strcmp(inv->line[i]->token[1]->str, ".data") == 0)
            {
                /* so relocation never breaks */
                fdwalker_align_byte(inv->fdwalker);

                /* record data section start for ELF emit */
                if(inv->data_section_start == UINT64_MAX)
                {
                    inv->data_section_start = fdwalker_bytes_used(inv->fdwalker);
                }

                /* iterating till section data is over */
                i++;
                for(; i < inv->line_cnt && (inv->line[i]->type == kAssemblerLineTypeSectionData || inv->line[i]->type == kAssemblerLineTypeIgnore); i++)
                {
                    if(inv->line[i]->type == kAssemblerLineTypeIgnore)
                    {
                        continue;
                    }

                    /* checking count */
                    if(inv->line[i]->token_cnt < 3)
                    {
                        diag_error(inv->line[i]->token[inv->line[i]->token_cnt - 1], "insufficient tokens for entry in .data section\n");
                        return false;
                    }

                    /* inserting address as label */
                    inv->label[inv->label_cnt].name = strdup(inv->line[i]->token[0]->str);
                    inv->label[inv->label_cnt].at_link = inv->line[i]->token[0];
                    inv->label[inv->label_cnt].defined = true;
                    inv->label[inv->label_cnt++].addr = fdwalker_bytes_used(inv->fdwalker);

                    /* checking if its known */
                    int dbs = 8;
                    if(strcmp(inv->line[i]->token[1]->str, "dw") == 0)
                    {
                        dbs = 16;
                    }
                    else if(strcmp(inv->line[i]->token[1]->str, "dd") == 0)
                    {
                        dbs = 32;
                    }
                    else if(strcmp(inv->line[i]->token[1]->str, "dq") == 0)
                    {
                        dbs = 64;
                    }
                    else if(strcmp(inv->line[i]->token[1]->str, "df") == 0)
                    {
                        const char *base_file_path = inv->file[inv->line[i]->file_idx];

                        /* need directory path */
                        char base_dir[PATH_MAX];
                        const char *last_slash = strrchr(base_file_path, '/');
                        if(!last_slash)
                        {
                            strcpy(base_dir, ".");
                        }
                        else
                        {
                            size_t len = last_slash - base_file_path;
                            if(len == 0)
                            {
                                strcpy(base_dir, "/");
                            }
                            else
                            {
                                memcpy(base_dir, base_file_path, len);
                                base_dir[len] = '\0';
                            }
                        }

                        /* iterating through the chain */
                        for(unsigned long a = 2; a < inv->line[i]->token_cnt; a++)
                        {
                            /* using low level type parser */
                            parser_return_t pr = parse_value_from_string(inv->line[i]->token[a]->str);
                            if(pr.type == emexParserValueTypeOverflow)
                            {
                                diag_error(inv->line[i]->token[a], "integer literal '%s' overflows 64bit lenght\n", inv->line[i]->token[a]->str);
                                return false;
                            }

                            /* checking type */
                            if(pr.type == emexParserValueTypeBuffer)
                            {
                                char *path_component = malloc(pr.len + 1);
                                strncpy(path_component, (char*)pr.value, pr.len);
                                path_component[pr.len] = '\0';

                                char joined[PATH_MAX];
                                int n = snprintf(joined, sizeof(joined), "%s/%s", base_dir, path_component);
                                if(n < 0 || n >= (int)sizeof(joined))
                                {
                                    diag_error(inv->line[i]->token[a], "path too long: %s\n", path_component);
                                    free(path_component);
                                    return false;
                                }

                                char resolved[PATH_MAX];
                                if(realpath(joined, resolved) == NULL)
                                {
                                    diag_error(inv->line[i]->token[a], "cannot resolve path '%s'\n", path_component);
                                    free(path_component);
                                    return false;
                                }

                                emex_file_t *file = emex_file_alloc(resolved, section_data_file_policy);
                                if(file == NULL || !emex_file_map(file))
                                {
                                    diag_error(inv->line[i]->token[a], "cannot map file at '%s'\n", path_component);
                                    free(path_component);
                                    return false;
                                }

                                fdwalker_write_buf(inv->fdwalker, file->content, file->len);

                                emex_file_dealloc(file);
                                free(path_component);
                                continue;
                            }

                            diag_error(inv->line[i]->token[1], "not a file path '%s'\n", inv->line[i]->token[1]->str);
                            return false;
                        }

                        continue;
                    }
                    else if(strcmp(inv->line[i]->token[1]->str, "db") != 0)
                    {
                        diag_error(inv->line[i]->token[1], "invalid data type for .data section entry '%s'\n", inv->line[i]->token[1]->str);
                        return false;
                    }

                    /* iterating through the chain */
                    for(unsigned long a = 2; a < inv->line[i]->token_cnt; a++)
                    {
                        /* using low level type parser */
                        parser_return_t pr = parse_value_from_string(inv->line[i]->token[a]->str);

                        /* checking type */
                        if(pr.type == emexParserValueTypeBuffer)
                        {
                            /* its a buffer so we copy the buffer into section */
                            char *buffer = (char*)pr.value;
                            fdwalker_write_buf(inv->fdwalker, buffer, pr.len);
                        }
                        else if(pr.type == emexParserValueTypeString)
                        {
                            if(dbs != 64)
                            {
                                diag_error(inv->line[i]->token[a], "don't put labels inside improper data types, i watch you!\n");
                                return false;
                            }

                            /* using finally the relocation table to its full extend */
                            reloc_table_entry_t *rtbe = inv->rtbe;
                            while(rtbe != NULL &&
                                  rtbe->next != NULL)
                            {
                                rtbe = rtbe->next;
                            }

                            if(rtbe == NULL)
                            {
                                rtbe = calloc(1, sizeof(reloc_table_entry_t));
                                inv->rtbe = rtbe;
                            }
                            else
                            {
                                rtbe->next = calloc(1, sizeof(reloc_table_entry_t));
                                rtbe = rtbe->next;
                            }

                            rtbe->name = strdup(inv->line[i]->token[a]->str);
                            rtbe->at_link = inv->line[i]->token[a];
                            rtbe->byte_pos = inv->fdwalker->byte_pos;
                            fdwalker_skip(inv->fdwalker, 64);
                        }
                        else
                        {
                            /* storing value */
                            fdwalker_write(inv->fdwalker, pr.value, dbs);
                        }
                    }
                }
                i--;
            }
        }
    }

    /* record data section end */
    if(inv->data_section_start != UINT64_MAX)
    {
        inv->data_section_end = fdwalker_bytes_used(inv->fdwalker);
    }

    if(inv->options.page_align)
    {
        inv->fdwalker->byte_pos = align_up(inv->fdwalker->byte_pos, 0x2000);
        inv->fdwalker->bit_idx = 0;
    }

    /* iterating for section token type and creating bss section */
    for(unsigned long i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->type == kAssemblerLineTypeSection)
        {
            if(strcmp(inv->line[i]->token[1]->str, ".bss") == 0)
            {
                /* so relocation never breaks */
                fdwalker_align_byte(inv->fdwalker);

                /* record bss start */
                if(inv->bss_section_start == UINT64_MAX)
                {
                    inv->bss_section_start = fdwalker_bytes_used(inv->fdwalker);
                }
                
                /* finding variable type */
                i++;
                for(; i < inv->line_cnt && (inv->line[i]->type == kAssemblerLineTypeSectionData || inv->line[i]->type == kAssemblerLineTypeIgnore); i++)
                {
                    if(inv->line[i]->type == kAssemblerLineTypeIgnore)
                    {
                        continue;
                    }

                    /* checking count */
                    if(inv->line[i]->token_cnt != 3)
                    {
                        diag_error(inv->line[i]->token[inv->line[i]->token_cnt - 1], "insufficient or too many tokens for entry in .bss section\n");
                        return false;
                    }

                    /* insert label into label array */
                    inv->label[inv->label_cnt].name = strdup(inv->line[i]->token[0]->str);
                    inv->label[inv->label_cnt].at_link = inv->line[i]->token[0];
                    inv->label[inv->label_cnt].defined = true;
                    inv->label[inv->label_cnt++].addr = fdwalker_bytes_used(inv->fdwalker);

                    /* find out size */
                    int dbs = 8;
                    if(strcmp(inv->line[i]->token[1]->str, "dw") == 0)
                    {
                        dbs = 16;
                    }
                    else if(strcmp(inv->line[i]->token[1]->str, "dd") == 0)
                    {
                        dbs = 32;
                    }
                    else if(strcmp(inv->line[i]->token[1]->str, "dq") == 0)
                    {
                        dbs = 64;
                    }
                    else if(strcmp(inv->line[i]->token[1]->str, "db") != 0)
                    {
                        diag_error(inv->line[i]->token[1], "invalid data type for .bss section entry '%s'\n", inv->line[i]->token[1]->str);
                        return false;
                    }

                    /* offset image address by value */
                    parser_return_t pr = parse_value_from_string(inv->line[i]->token[2]->str);

                    /* checking if the type makes sense */
                    /* imagine you read that comment and you realise that you had the same idea before */
                    if(pr.type != emexParserValueTypeNumber)
                    {
                        diag_error(inv->line[i]->token[2], "invalid size for .bss section entry '%s'\n", inv->line[i]->token[2]->str);
                        return false;
                    }

                    inv->fdwalker->byte_pos += (dbs / 8) * pr.value;
                }
                i--;
            }
        }
    }

    /* record bss section size */
    if(inv->bss_section_start != UINT64_MAX)
    {
        uint64_t bss_end = fdwalker_bytes_used(inv->fdwalker);
        inv->bss_section_size = bss_end > inv->bss_section_start ? bss_end - inv->bss_section_start : 0;
    }

    if(inv->options.page_align)
    {
        inv->fdwalker->byte_pos = align_up(inv->fdwalker->byte_pos, 0x2000);
        inv->fdwalker->bit_idx = 0;
    }

    return true;
}
