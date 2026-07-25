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
#include <string.h>
#include <EmexFoundation/EmexFoundation.h>
#include <EmexToolchain/ETLinker/sym.h>

linker_symbol_t *linker_symbol_alloc(const char *name,
                                     const char *object_path,
                                     UInt64 addr,
                                     Boolean defined)
{
    linker_symbol_t *sym = malloc(sizeof(linker_symbol_t));
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
        free(sym->name);
        free(sym);
        return NULL;
    }

    sym->addr = addr;
    sym->defined = defined;

    sym->next = NULL;

    return sym;
}

void linker_symbol_dealloc(linker_symbol_t *sym)
{
    free(sym->name);
    free(sym->object_path);
    free(sym);
}
