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
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <emex64lib/support/diagnostic/legacy.h>
#include <emex64lib/linker/linker.h>
#include <emex64lib/linker/script.h>

bool linker_script_parse(linker_invocation_t *inv,
                         emex_file_t *script_file)
{
    if(!emex_file_open(script_file))
    {
        /* couldn't open the script file */
        return false;
    }

    vfd_t *d = emex_file_dup_vfd(script_file);
    if(d == NULL)
    {
        /* couldn't dup descriptor */
        return false;
    }

    char line[1024];
    int lineno = 0;
    while(vfd_gets(d, line, sizeof(line)))
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
                diag_error(NULL, "%s:%d: expected symbol name after PROVIDE\n", script_file->path, lineno);
                vfd_close(d);
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
                diag_error(NULL, "%s:%d: expected '=' after symbol name\n", script_file->path, lineno);
                free(sym_name);
                vfd_close(d);
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
                diag_error(NULL, "%s:%d: empty expression\n", script_file->path, lineno);
                free(sym_name);
                vfd_close(d);
                return false;
            }

            script_sym_t *new = realloc(inv->script_syms, (inv->script_sym_cnt + 1) * sizeof(script_sym_t));
            if(new == NULL)
            {
                diag_fatal(NULL, "out of memory\n", script_file->path, lineno);
                free(sym_name);
                vfd_close(d);
                return false;
            }
            inv->script_syms = new;
            inv->script_syms[inv->script_sym_cnt].name = sym_name;
            inv->script_syms[inv->script_sym_cnt].expr = strdup(expr_start);
            inv->script_syms[inv->script_sym_cnt].script_path = script_file->path;
            inv->script_sym_cnt++;
            continue;
        }

        diag_error(NULL, "%s:%d: unrecognised linker script directive: '%s'\n", script_file->path, lineno, p);
        vfd_close(d);
        return false;
    }

    vfd_close(d);
    return true;
}

bool linker_script_apply(linker_invocation_t *inv,
                         uint64_t image_end,
                         uint64_t text_start,
                         uint64_t data_start,
                         uint64_t bss_start)
{
    for(size_t i = 0; i < inv->script_sym_cnt; i++)
    {
        const char *expr = inv->script_syms[i].expr;
        uint64_t value = 0;

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
            value = (uint64_t)strtoull(expr, &endptr, 0);
            if(!endptr || *endptr != '\0')
            {
                diag_error(NULL, "unknown expression '%s' in linker script\n", expr);
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
