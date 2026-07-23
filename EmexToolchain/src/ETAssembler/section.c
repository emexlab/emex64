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
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/Support/parser.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/Support/pack.h>
#include <EmexToolchain/ETAssembler/label/label.h>
#include <EmexToolchain/ETAssembler/label/relocate.h>
#include <EmexToolchain/ETAssembler/section.h>
#include <EmexToolchain/ETAssembler/code.h>
#include <EmexToolchain/ETAssembler/expr.h>

static Boolean __assembler_section_emit_value(ETAssemblerInvocationRef inv,
                                              assembler_token_t **entry,
                                              UInt64 entry_cnt,
                                              int dbs)
{
    if(entry_cnt == 1 && entry[0]->type == kETAssemblerTokenTypeString)
    {
        EFBitWalkerWriteBuffer(inv->out_vbitwalker, entry[0]->string_literal.buf, (EFIndex)entry[0]->string_literal.len);
        return true;
    }

    if(entry_cnt == 1 && entry[0]->type == kETAssemblerTokenTypeIdentifier)
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
        EFBitWalkerSkip(inv->out_vbitwalker, 64);
        return true;
    }

    SInt64 value;
    if(!assembler_eval_const(entry, entry_cnt, &value))
    {
        return false;
    }

    if(dbs < 64)
    {
        UInt64 umax = (UINT64_C(1) << dbs) - 1;
        SInt64 smin = -(INT64_C(1) << (dbs - 1));
        if(value < smin || (UInt64)value > umax)
        {
            diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(entry[0]), "value %ld doesn't fit in a %d bits data entry", (long long)value, dbs);
            return false;
        }
    }

    EFBitWalkerWrite(inv->out_vbitwalker, (UInt64)value, dbs);
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

Boolean assembler_section_parse(ETAssemblerInvocationRef inv)
{
    /* only emitting data section into out virtual file descriptor */
    for(UInt64 i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->type != kETAssemblerLineTypeSection ||
           strcmp(inv->line[i]->token[1]->str, ".data") != 0)
        {
            continue;
        }

        EFBitWalkerAlignByte(inv->out_vbitwalker);
        if(inv->data_section_start == UINT64_MAX)
        {
            inv->data_section_start = EFBitWalkerBytesUsed(inv->out_vbitwalker);
        }

        i++;
        for(; i < inv->line_cnt && (inv->line[i]->type == kETAssemblerLineTypeSectionData || inv->line[i]->type == kETAssemblerLineTypeIgnore); i++)
        {
            if(inv->line[i]->type == kETAssemblerLineTypeIgnore)
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
                EFURLRef url = EFFileGetURL(EFArrayGetValueAtIndex(inv->files, inv->line[i]->file_idx));
                EFAUTOREL EFStringRef pathStr = EFURLCopyPath(EFGetAllocator(url), url);

                for(unsigned long a = 2; a < inv->line[i]->token_cnt; a++)
                {
                    if(inv->line[i]->token[a]->type == kETAssemblerTokenTypeComma)
                    {
                        continue;
                    }
                    if(inv->line[i]->token[a]->type != kETAssemblerTokenTypeString)
                    {
                        diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[a]), "not a file path '%s'", inv->line[i]->token[a]->str);
                        return false;
                    }

                    const char *path_component = inv->line[i]->token[a]->string_literal.buf;

                    EFAUTOREL EFFileRef file = NULL;
                    EFAUTOREL EFStringRef pathComponentStr = EFStringCreateWithCString(kEFAllocatorDefault, path_component, kEFStringEncodingUTF8);
                    if(EFStringHasPrefix(pathComponentStr, EFSTR("https://")) || EFStringHasPrefix(pathComponentStr, EFSTR("http://")) || EFStringHasPrefix(pathComponentStr, EFSTR("/")))
                    {
                        file = EFFileCreateWithPath(EFGetAllocator(url), EFFilePolicyInData, pathComponentStr);
                    }
                    else
                    {
                        EFAUTOREL EFURLRef newUrl = EFURLCreateURLByReplacingLastPathComponent(kEFAllocatorDefault, url, pathComponentStr);
                        file = EFFileCreateWithURL(EFGetAllocator(url), EFFilePolicyInData, newUrl);
                    }
                    EFAUTOREL EFFileHandleRef fileHandle = EFFileCopyFileHandle(EFGetAllocator(url), file);
                    EFAUTOREL EFMappingRef mapping = EFFileHandleCopyMapping(EFGetAllocator(url), fileHandle);
                    if(file == NULL || mapping == NULL)
                    {
                        diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[a]), "cannot map file at '%s'", path_component);
                        return false;
                    }

                    void *address = EFMappingGetAddress(mapping);
                    EFSize size = EFMappingGetSize(mapping);

                    EFBitWalkerWriteBuffer(inv->out_vbitwalker, address, (EFIndex)size);
                }
                continue;
            }
            else if(dbs == 128)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[1]), "invalid data type for .data section entry '%s'", inv->line[i]->token[1]->str);
                return false;
            }

            UInt64 a = 2;
            while(a < inv->line[i]->token_cnt)
            {
                UInt64 start = a;
                while(a < inv->line[i]->token_cnt && inv->line[i]->token[a]->type != kETAssemblerTokenTypeComma)
                {
                    a++;
                }
                assembler_token_t **entry = &inv->line[i]->token[start];
                UInt64 entry_cnt = a - start;

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

    EFBitWalkerAlignByte(inv->out_vbitwalker);
    if(inv->data_section_start != UINT64_MAX)
    {
        inv->data_section_end = (UInt64)EFBitWalkerBytesUsed(inv->out_vbitwalker);
    }

    /* only emitting bss section into out virtual file descriptor */
    for(UInt64 i = 0; i < inv->line_cnt; i++)
    {
        if(inv->line[i]->type != kETAssemblerLineTypeSection ||
           strcmp(inv->line[i]->token[1]->str, ".bss") != 0)
        {
            continue;
        }

        EFBitWalkerAlignByte(inv->out_vbitwalker);
        if(inv->bss_section_start == UINT64_MAX)
        {
            inv->bss_section_start = (UInt64)EFBitWalkerBytesUsed(inv->out_vbitwalker);
        }

        i++;
        for(; i < inv->line_cnt && (inv->line[i]->type == kETAssemblerLineTypeSectionData || inv->line[i]->type == kETAssemblerLineTypeIgnore); i++)
        {
            if(inv->line[i]->type == kETAssemblerLineTypeIgnore)
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

            SInt64 count;
            if(!assembler_eval_const(&inv->line[i]->token[2], inv->line[i]->token_cnt - 2, &count))
            {
                return false;
            }
            if(count < 0)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, AT_TO_DLOC(inv->line[i]->token[2]), "negative size in .bss section entry");
                return false;
            }

            EFBitWalkerPosition position = EFBitWalkerGetPosition(inv->out_vbitwalker);
            position.bytePos += (UInt64)(dbs / 8) * (UInt64)count;
            EFBitWalkerSetPosition(inv->out_vbitwalker, position);
        }
        i--;
    }

    EFBitWalkerAlignByte(inv->out_vbitwalker);
    if(inv->bss_section_start != UINT64_MAX)
    {
        UInt64 bss_end = (UInt64)EFBitWalkerBytesUsed(inv->out_vbitwalker);
        inv->bss_section_size = bss_end > inv->bss_section_start ? bss_end - inv->bss_section_start : 0;
    }

    return true;
}
