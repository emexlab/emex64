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
#include <emex64lib/support/diagnostic/log.h>
#include <emex64lib/support/file.h>
#include <emex64lib/support/pack.h>
#include <emex64lib/asm/label/label.h>
#include <emex64lib/asm/label/relocate.h>
#include <emex64lib/asm/section.h>
#include <emex64lib/asm/code.h>
#include <emex64lib/asm/expr.h>

static bool __assembler_section_emit_value(assembler_invocation_t *inv,
                                           assembler_token_t **entry,
                                           uint64_t entry_cnt,
                                           int dbs)
{
    if(entry_cnt == 1 && entry[0]->type == kAssemblerTokenTypeString)
    {
        vbitwalker_write_buf(inv->out_vbitwalker, entry[0]->string_literal.buf, entry[0]->string_literal.len);
        return true;
    }

    if(entry_cnt == 1 && entry[0]->type == kAssemblerTokenTypeIdentifier)
    {
        if(dbs != 64)
        {
            diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(entry[0]), "don't put labels inside improper data types, i watch you!");
            return false;
        }
        if(!assembler_label_relocate_append(inv, strdup(entry[0]->str), false, entry[0]))
        {
            diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, AT_TO_DLOC(entry[0]), "out of memory, can't append relocation to relocation table");
            return false;
        }
        vbitwalker_skip(inv->out_vbitwalker, 64);
        return true;
    }

    int64_t value;
    if(!assembler_eval_const(entry, entry_cnt, &value))
    {
        return false;
    }

    if(dbs < 64)
    {
        uint64_t umax = (UINT64_C(1) << dbs) - 1;
        int64_t smin = -(INT64_C(1) << (dbs - 1));
        if(value < smin || (uint64_t)value > umax)
        {
            diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(entry[0]), "value %ld doesn't fit in a %d bits data entry", (long long)value, dbs);
            return false;
        }
    }

    vbitwalker_write(inv->out_vbitwalker, (uint64_t)value, dbs);
    return true;
}

static int __assembler_section_dbs_get(const char *str)
{
    switch(pack_name(str))
    {
        case PACK('d','b'):
            return 8;
        case PACK('d','w'):
            return 16;
        case PACK('d','d'):
            return 32;
        case PACK('d','q'):
            return 64;
        case PACK('d','f'):
            return 0;
        default:
            return 128;
    }
}

bool assembler_section_parse(assembler_invocation_t *inv)
{
    /* only emitting data section into out virtual file descriptor */
    for(uint64_t i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->type != kAssemblerLineTypeSection ||
           strcmp(inv->line[i]->token[1]->str, ".data") != 0)
        {
            continue;
        }

        vbitwalker_align_byte(inv->out_vbitwalker);
        if(inv->data_section_start == UINT64_MAX)
        {
            inv->data_section_start = vbitwalker_bytes_used(inv->out_vbitwalker);
        }

        i++;
        for(; i < inv->line_cnt && (inv->line[i]->type == kAssemblerLineTypeSectionData || inv->line[i]->type == kAssemblerLineTypeIgnore); i++)
        {
            if(inv->line[i]->type == kAssemblerLineTypeIgnore)
            {
                continue;
            }

            if(inv->line[i]->token_cnt < 3)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[inv->line[i]->token_cnt - 1]), "insufficient tokens for entry in .data section");
                return false;
            }

            if(!assembler_label_append(inv->line[i]->token[0]))
            {
                return false;
            }

            int dbs = __assembler_section_dbs_get(inv->line[i]->token[1]->str);
            if(dbs == 0)
            {
                const char *base_file_path = inv->file[inv->line[i]->file_idx]->path;
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

                for(unsigned long a = 2; a < inv->line[i]->token_cnt; a++)
                {
                    if(inv->line[i]->token[a]->type == kAssemblerTokenTypeComma)
                    {
                        continue;
                    }
                    if(inv->line[i]->token[a]->type != kAssemblerTokenTypeString)
                    {
                        diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[a]), "not a file path '%s'", inv->line[i]->token[a]->str);
                        return false;
                    }

                    const char *path_component = inv->line[i]->token[a]->string_literal.buf;

                    char joined[PATH_MAX];
                    int n = snprintf(joined, sizeof(joined), "%s/%s", base_dir, path_component);
                    if(n < 0 || n >= (int)sizeof(joined))
                    {
                        diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[a]), "path too long: %s", path_component);
                        return false;
                    }

                    char resolved[PATH_MAX];
                    if(realpath(joined, resolved) == NULL)
                    {
                        diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[a]), "cannot resolve path '%s'", path_component);
                        return false;
                    }

                    emex_file_t *file = emex_file_alloc(resolved, in_data_file_policy);
                    if(file == NULL || !emex_file_map(file))
                    {
                        diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[a]), "cannot map file at '%s'", path_component);
                        return false;
                    }

                    vbitwalker_write_buf(inv->out_vbitwalker, file->content, file->len);
                    emex_file_dealloc(file);
                }
                continue;
            }
            else if(dbs == 128)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[1]), "invalid data type for .data section entry '%s'", inv->line[i]->token[1]->str);
                return false;
            }

            uint64_t a = 2;
            while(a < inv->line[i]->token_cnt)
            {
                uint64_t start = a;
                while(a < inv->line[i]->token_cnt && inv->line[i]->token[a]->type != kAssemblerTokenTypeComma)
                {
                    a++;
                }
                assembler_token_t **entry = &inv->line[i]->token[start];
                uint64_t entry_cnt = a - start;

                if(a < inv->line[i]->token_cnt)
                {
                    a++;
                }

                if(entry_cnt == 0)
                {
                    diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[start - 1]), "empty value in .data entry");
                    return false;
                }

                if(!__assembler_section_emit_value(inv, entry, entry_cnt, dbs))
                {
                    return false;
                }
            }
        }
        i--;
    }

    vbitwalker_align_byte(inv->out_vbitwalker);
    if(inv->data_section_start != UINT64_MAX)
    {
        inv->data_section_end = vbitwalker_bytes_used(inv->out_vbitwalker);
    }

    /* only emitting bss section into out virtual file descriptor */
    for(uint64_t i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->type != kAssemblerLineTypeSection ||
           strcmp(inv->line[i]->token[1]->str, ".bss") != 0)
        {
            continue;
        }

        vbitwalker_align_byte(inv->out_vbitwalker);
        if(inv->bss_section_start == UINT64_MAX)
        {
            inv->bss_section_start = vbitwalker_bytes_used(inv->out_vbitwalker);
        }

        i++;
        for(; i < inv->line_cnt && (inv->line[i]->type == kAssemblerLineTypeSectionData || inv->line[i]->type == kAssemblerLineTypeIgnore); i++)
        {
            if(inv->line[i]->type == kAssemblerLineTypeIgnore)
            {
                continue;
            }

            if(inv->line[i]->token_cnt < 3)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[inv->line[i]->token_cnt - 1]), "insufficient tokens for entry in .bss section");
                return false;
            }

            if(!assembler_label_append(inv->line[i]->token[0]))
            {
                return false;
            }

            int dbs = __assembler_section_dbs_get(inv->line[i]->token[1]->str);
            if(dbs == 128 || dbs == 0)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[1]), "invalid data type for .bss section entry '%s'", inv->line[i]->token[1]->str);
                return false;
            }

            int64_t count;
            if(!assembler_eval_const(&inv->line[i]->token[2], inv->line[i]->token_cnt - 2, &count))
            {
                return false;
            }
            if(count < 0)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[2]), "negative size in .bss section entry");
                return false;
            }

            inv->out_vbitwalker->byte_pos += (uint64_t)(dbs / 8) * (uint64_t)count;
        }
        i--;
    }

    vbitwalker_align_byte(inv->out_vbitwalker);
    if(inv->bss_section_start != UINT64_MAX)
    {
        uint64_t bss_end = vbitwalker_bytes_used(inv->out_vbitwalker);
        inv->bss_section_size = bss_end > inv->bss_section_start ? bss_end - inv->bss_section_start : 0;
    }

    return true;
}
