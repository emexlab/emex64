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
#include <errno.h>
#include <string.h>
#include <EmexToolchain/Support/diagnostic/log.h>
#include <EmexToolchain/ETLinker/linker.h>
#include <EmexToolchain/ETLinker/script.h>

Boolean linker_script_parse(linker_invocation_t *inv,
                            emex_file_t *script_file)
{
    if(!emex_file_open(script_file))
    {
        /* couldn't open the script file */
        return false;
    }

    EFAUTOREL EFFileHandleRef d = emex_file_dup_vfd(script_file);
    if(d == NULL)
    {
        /* couldn't dup descriptor */
        return false;
    }

    char line[1024];
    int lineno = 0;
    while(EFFileHandleGets(d, line, sizeof(line)))
    {
        lineno++;
        char *comment = strchr(line, '#');
        if(comment)
        {
            *comment = '\0';
        }
        char *end = line + strlen(line);
        while(end > line && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' '  || end[-1] == '\t'))
        {
            *--end = '\0';
        }

        char *p = line;
        while(*p == ' ' || *p == '\t')
        {
            p++;
        }
        if(!*p)
        {
            continue;
        }

        if(strncmp(p, "PROVIDE", 7) == 0 && (p[7] == ' ' || p[7] == '\t'))
        {
            p += 7;
            while(*p == ' ' || *p == '\t')
            {
                p++;
            }

            /* symbol name */
            char *name_start = p;
            while(*p && *p != '=' && *p != ' ' && *p != '\t')
            {
                p++;
            }
            size_t name_len = (size_t)(p - name_start);
            if(name_len == 0)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "%s:%d: expected symbol name after PROVIDE", script_file->path, lineno);
                return false;
            }
            char *sym_name = malloc(name_len + 1);
            memcpy(sym_name, name_start, name_len);
            sym_name[name_len] = '\0';

            while(*p == ' ' || *p == '\t')
            {
                p++;
            }
            if(*p != '=')
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "%s:%d: expected '=' after symbol name", script_file->path, lineno);
                free(sym_name);
                return false;
            }
            p++;
            while(*p == ' ' || *p == '\t')
            {
                p++;
            }

            char *expr_start = p;
            char *semi = strchr(p, ';');
            if(semi)
            {
                *semi = '\0';
            }
            end = p + strlen(p);
            while(end > p && (end[-1] == ' ' || end[-1] == '\t'))
            {
                *--end = '\0';
            }

            if(!*expr_start)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "%s:%d: empty expression", script_file->path, lineno);
                free(sym_name);
                return false;
            }

            script_sym_t *new = realloc(inv->script_syms, (inv->script_sym_cnt + 1) * sizeof(script_sym_t));
            if(new == NULL)
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityFatal, NULL, "out of memory", script_file->path, lineno);
                free(sym_name);
                return false;
            }
            inv->script_syms = new;
            inv->script_syms[inv->script_sym_cnt].name = sym_name;
            inv->script_syms[inv->script_sym_cnt].expr = strdup(expr_start);
            inv->script_syms[inv->script_sym_cnt].script_path = script_file->path;
            inv->script_sym_cnt++;
            continue;
        }

        diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "%s:%d: unrecognised linker script directive: '%s'", script_file->path, lineno, p);
        return false;
    }
    return true;
}

Boolean linker_script_apply(linker_invocation_t *inv,
                            UInt64 image_end,
                            UInt64 text_start,
                            UInt64 data_start,
                            UInt64 bss_start)
{
    for(size_t i = 0; i < inv->script_sym_cnt; i++)
    {
        const char *expr = inv->script_syms[i].expr;
        UInt64 value = 0;

        if(strcmp(expr, "IMAGE_END") == 0)
        {
            value = image_end;
        }
        else if(strcmp(expr, "IMAGE_START") == 0)
        {
            value = 0;
        }
        else if(strcmp(expr, "TEXT_START") == 0)
        {
            value = text_start;
        }
        else if(strcmp(expr, "DATA_START") == 0)
        {
            value = data_start;
        }
        else if(strcmp(expr, "BSS_START") == 0)
        {
            value = bss_start;
        }
        else
        {
            /* parse hex / decimal number */
            char *endptr = NULL;
            value = (UInt64)strtoull(expr, &endptr, 0);
            if(!endptr || *endptr != '\0')
            {
                diagnostic_report(inv->consumer, kDiagnosticSeverityError, NULL, "unknown expression '%s' in linker script", expr);
                return false;
            }
        }

        if(!linker_symbol_append_definition(inv, inv->script_syms[i].name, inv->script_syms[i].script_path, value))
        {
            return false;
        }
    }
    return true;
}
