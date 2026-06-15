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

#include <emex64lib/linker/linker.h>

linker_invocation_t *linker_invocation_alloc(void)
{
    linker_invocation_t *inv = malloc(sizeof(linker_invocation_t));
    if(inv == NULL)
    {
        return NULL;
    }

    inv->sym = NULL;

    return inv;
}

void linker_invocation_dealloc(linker_invocation_t *inv)
{
    linker_global_symbol_t *sym = inv->sym;
    while(sym != NULL)
    {
        linker_global_symbol_t *next = sym->next;
        free(sym->name);
        free(sym->object_path);
        free(sym);
        sym = next;
    }

    free(inv);
}
