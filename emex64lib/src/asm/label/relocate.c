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
#include <assert.h>
#include <emex64lib/asm/label/relocate.h>
#include <emex64lib/asm/invocation.h>

bool assembler_label_relocate_append(assembler_invocation_t *inv,
                                     char *label_str,
                                     bool local,
                                     assembler_token_t *at_link)
{
    assert(inv != NULL && label_str != NULL && at_link != NULL);

    reloc_table_entry_t *rtbe = calloc(1, sizeof(reloc_table_entry_t));
    if(rtbe == NULL)
    {
        return false;
    }

    /* stich link list ^^ */
    if(inv->rtbe == NULL)
    {
        inv->rtbe = rtbe;
    }
    else
    {
        /* finding the head */
        reloc_table_entry_t *tail = inv->rtbe;
        while(tail->next != NULL)
        {
            tail = tail->next;
        }
        tail->next = rtbe;
    }

    rtbe->name = label_str;
    rtbe->byte_pos = vbitwalker_bytes_used(inv->out_vbitwalker);
    rtbe->at_link = at_link;
    rtbe->local = local;

    return true;
}

