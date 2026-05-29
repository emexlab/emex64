/*
 * MIT License
 *
 * Copyright (c) 2024 emexlab
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

#include <emex64lib/support/parser.h>
#include <emex64lib/support/bitwalker.h>

#include <emex64lib/asm/section.h>
#include <emex64lib/asm/code.h>
#include <emex64lib/asm/diag.h>

static inline unsigned long align_up(unsigned long v, unsigned long a)
{
    return (v + a - 1) & ~(a - 1);
}

void assembler_section_parse(assembler_invocation_t *inv)
{
    /* iterating for section token type and creating data section */
    for(unsigned long i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i].type == ASSEMBLER_LINE_TYPE_SECTION)
        {
            if(strcmp(inv->line[i].token[1].str, ".data") == 0)
            {
                /* iterating till section data is over */
                i++;
                for(; i < inv->line_cnt && inv->line[i].type == ASSEMBLER_LINE_TYPE_SECTION_DATA; i++)
                {
                    /* checking count */
                    if(inv->line[i].token_cnt < 3)
                    {
                        diag_error(&(inv->line[i].token[inv->line[i].token_cnt - 1]), "insufficient tokens for entry in .data section\n");
                    }

                    /* inserting address as label */
                    inv->label[inv->label_cnt].name = strdup(inv->line[i].token[0].str);
                    inv->label[inv->label_cnt++].addr = fdwalker_bytes_used(inv->fdwalker);

                    /* checking if its known */
                    int dbs = 8;
                    if(strcmp(inv->line[i].token[1].str, "dw") == 0)
                    {
                        dbs = 16;
                    }
                    else if(strcmp(inv->line[i].token[1].str, "dd") == 0)
                    {
                        dbs = 32;
                    }
                    else if(strcmp(inv->line[i].token[1].str, "dq") == 0)
                    {
                        dbs = 64;
                    }
                    else if(strcmp(inv->line[i].token[1].str, "db") != 0)
                    {
                        diag_error(&(inv->line[i].token[1]), "invalid data type for .data section entry \"%s\"\n", inv->line[i].token[1].str);
                    }

                    /* iterating through the chain */
                    for(unsigned long a = 2; a < inv->line[i].token_cnt; a++)
                    {
                        /* using low level type parser */
                        parser_return_t pr = parse_value_from_string(inv->line[i].token[a].str);

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
                                diag_error(&(inv->line[i].token[a]), "don't put labels inside improper data types, i watch you!\n");
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

                            rtbe->name = strdup(inv->line[i].token[a].str);
                            rtbe->ctlink = &(inv->line[i].token[a]);
                            rtbe->byte_pos = inv->fdwalker->byte_pos;
                            rtbe->bit_idx = inv->fdwalker->bit_idx;
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

    if(inv->options.page_align)
    {
        inv->fdwalker->byte_pos = align_up(inv->fdwalker->byte_pos, 0x2000);
        inv->fdwalker->bit_idx = 0;
    }

    /* iterating for section token type and creating bss section */
    for(unsigned long i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i].type == ASSEMBLER_LINE_TYPE_SECTION)
        {
            if(strcmp(inv->line[i].token[1].str, ".bss") == 0)
            {
                /* finding variable type */
                i++;
                for(; i < inv->line_cnt && inv->line[i].type == ASSEMBLER_LINE_TYPE_SECTION_DATA; i++)
                {
                    /* checking count */
                    if(inv->line[i].token_cnt != 3)
                    {
                        diag_error(&(inv->line[i].token[inv->line[i].token_cnt - 1]), "insufficient or too many tokens for entry in .bss section\n");
                    }

                    /* insert label into label array */
                    inv->label[inv->label_cnt].name = strdup(inv->line[i].token[0].str);
                    inv->label[inv->label_cnt++].addr = fdwalker_bytes_used(inv->fdwalker);

                    /* find out size */
                    int dbs = 8;
                    if(strcmp(inv->line[i].token[1].str, "dw") == 0)
                    {
                        dbs = 16;
                    }
                    else if(strcmp(inv->line[i].token[1].str, "dd") == 0)
                    {
                        dbs = 32;
                    }
                    else if(strcmp(inv->line[i].token[1].str, "dq") == 0)
                    {
                        dbs = 64;
                    }
                    else if(strcmp(inv->line[i].token[1].str, "db") != 0)
                    {
                        diag_error(&(inv->line[i].token[1]), "invalid data type for .bss section entry \"%s\"\n", inv->line[i].token[1].str);
                    }

                    /* offset image address by value */
                    parser_return_t pr = parse_value_from_string(inv->line[i].token[2].str);

                    /* checking if the type makes sense */
                    /* imagine you read that comment and you realise that you had the same idea before */
                    if(pr.type != emexParserValueTypeNumber)
                    {
                        diag_error(&(inv->line[i].token[2]), "invalid size for .bss section entry \"%s\"\n", inv->line[i].token[2].str);
                    }

                    inv->fdwalker->byte_pos += (dbs / 8) * pr.value;
                }
                i--;
            }
        }
    }

    if(inv->options.page_align)
    {
        inv->fdwalker->byte_pos = align_up(inv->fdwalker->byte_pos, 0x2000);
        inv->fdwalker->bit_idx = 0;
    }
}