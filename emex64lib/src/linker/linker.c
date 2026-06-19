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

#include <stdlib.h>
#include <string.h>

#include <emex64lib/support/diag.h>

#include <emex64lib/linker/linker.h>

uint8_t ident[EI_NIDENT] = { ELF_MAGIC_0, ELF_MAGIC_1, ELF_MAGIC_2, ELF_MAGIC_3, ELF_CLASS64, ELF_DATA2LSB, EV_CURRENT };

linker_invocation_t *linker_invocation_alloc(linker_options_t options)
{
    linker_invocation_t *inv = malloc(sizeof(linker_invocation_t));
    if(inv == NULL)
    {
        return NULL;
    }

    inv->sym = NULL;
    inv->obj = NULL;
    inv->script_syms = NULL;
    inv->script_sym_cnt = 0;

    /* a lot of problems to solve for today :3 */
    inv->out_text_off = BOOT_HEADER_SIZE;
    inv->out_data_off = 0;
    inv->out_bss_off = 0;

    inv->options = options;

    return inv;
}

void linker_invocation_dealloc(linker_invocation_t *inv)
{
    linker_symbol_t *sym = inv->sym;
    while(sym != NULL)
    {
        linker_symbol_t *next = sym->next;
        linker_symbol_dealloc(sym);
        sym = next;
    }

    linker_object_t *obj = inv->obj;
    while(obj != NULL)
    {
        linker_object_t *next = obj->next;
        linker_object_dealloc(obj);
        obj = next;
    }

    for(size_t i = 0; i < inv->script_sym_cnt; i++)
    {
        free(inv->script_syms[i].name);
        free(inv->script_syms[i].expr);
    }
    free(inv->script_syms);

    free(inv);
}

bool linker_symbol_append_definition(linker_invocation_t *inv,
                                     const char *name,
                                     const char *object_path,
                                     uint64_t addr)
{
    linker_symbol_t *sym = linker_symbol_lookup(inv, name);
    if(sym == NULL)
    {
        sym = linker_symbol_alloc(name, object_path, addr, true);
        if(sym == NULL)
        {
            return false;
        }
    }
    if(sym->defined && sym->addr != addr)
    {
        diag_error(NULL, "duplicate symbol '%s' in '%s'\n", name, object_path);
        diag_note(NULL, "symbol '%s' also exists in '%s'\n", name, sym->object_path);
        return false;
    }

    if(inv->sym == NULL)
    {
        inv->sym = sym;
    }
    else
    {
        sym->next = inv->sym;
        inv->sym = sym;
    }

    return true;
}

linker_symbol_t *linker_symbol_lookup(linker_invocation_t *inv,
                                      const char *name)
{
    linker_symbol_t *sym = inv->sym;
    while(sym != NULL)
    {
        if(strcmp(sym->name, name) == 0)
        {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}
