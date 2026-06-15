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

#include <emex64lib/linker/sym.h>

linker_global_symbol_t *linker_global_symbol_alloc(linker_global_symbol_t *prev,
                                                   const char *name,
                                                   const char *object_path,
                                                   uint64_t addr,
                                                   bool defined)
{
    linker_global_symbol_t *sym = malloc(sizeof(linker_global_symbol_t));
    if(sym == NULL)
    {
        return NULL;
    }

    sym->name = strdup(name);
    if(sym->name == NULL)
    {
        free(sym);
        return NULL;
    }

    sym->object_path = strdup(object_path);
    if(sym->object_path == NULL)
    {
        free(sym->object_path);
        free(sym);
        return NULL;
    }

    sym->addr = addr;
    sym->defined = defined;

    sym->next = NULL;

    if(prev != NULL)
    {
        prev->next = sym;
        sym->prev = prev;
    }

    return sym;
}

void linker_global_symbol_dealloc(linker_global_symbol_t *sym)
{
    if(sym->prev != NULL)
    {
        sym->prev->next = sym->next;
    }

    if(sym->next != NULL)
    {
        sym->next->prev = sym->prev;
    }

    free(sym->name);
    free(sym->object_path);
    free(sym);
}
