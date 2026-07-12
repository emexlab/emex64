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

#include <stdlib.h>
#include <assert.h>
#include <EmexToolchain/ETAssembler/label/relocate.h>
#include <EmexToolchain/ETAssembler/invocation.h>

Boolean assembler_label_relocate_append(assembler_invocation_t *inv,
                                        char *label_str,
                                        Boolean local,
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
