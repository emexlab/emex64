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

#include <string.h>
#include <EmexToolchain/asm/preprocessor/macro.h>

assembler_macro_t *assembler_macro_alloc(const char *match,
                                         const char **inject_token,
                                         UInt64 token_cnt)
{
    assembler_macro_t *macro = malloc(sizeof(assembler_macro_t));
    if(macro == NULL)
    {
        return NULL;
    }

    macro->inject_token = inject_token;
    macro->inject_token_cnt = token_cnt;

    return macro;
}

void assembler_macro_dealloc(assembler_macro_t *macro)
{
    free(macro->inject_token);
    free(macro);
}

assembler_macro_storage_t *assembler_macro_storage_alloc()
{
    assembler_macro_storage_t *storage = malloc(sizeof(assembler_macro_storage_t));
    if(storage == NULL)
    {
        return NULL;
    }

    storage->macro_map = hashmap_alloc();
    if(storage->macro_map == NULL)
    {
        free(storage);
        return NULL;
    }

    return storage;
}

void assembler_macro_storage_dealloc(assembler_macro_storage_t *storage)
{
    const void *key; size_t klen; assembler_macro_t *val;
    for(hashmap_iter_t it = hashmap_iter_create(storage->macro_map); hashmap_next(&it, &key, &klen, (void**)&val);)
    {
        assembler_macro_dealloc(val);
    }
    hashmap_dealloc(storage->macro_map);
    free(storage);
}

assembler_macro_t *assembler_macro_storage_lookup(assembler_macro_storage_t *storage,
                                                  const char *match)
{
    return (assembler_macro_t*)hashmap_gets(storage->macro_map, match);
}

Boolean assembler_macro_storage_append_macro_char(assembler_macro_storage_t *storage,
                                               const char *match,
                                               const char **token,
                                               UInt64 token_cnt)
{
    /* checking if it is already defined */
    assembler_macro_t *found = assembler_macro_storage_lookup(storage, match);
    if(found != NULL)
    {
        /* inject information */
        free(found->inject_token);
        found->inject_token = token;
        found->inject_token_cnt = token_cnt;
        return true;
    }

    /* need new macro */
    assembler_macro_t *macro = assembler_macro_alloc(match, token, token_cnt);
    if(macro == NULL)
    {
        return false;
    }

    if(!hashmap_puts(storage->macro_map, match, macro))
    {
        assembler_macro_dealloc(macro);
        return false;
    }

    return true;
}

Boolean assembler_macro_storage_append_macro(assembler_macro_storage_t *storage,
                                          const char *match,
                                          assembler_token_t **token,
                                          UInt64 token_cnt)
{
    /* checking if it is already defined */
    const char **token_char = calloc(token_cnt, sizeof(const char *));
    if(token_char == NULL)
    {
        return false;
    }

    for(UInt64 i = 0; i < token_cnt; i++)
    {
        token_char[i] = token[i]->str;
    }

    Boolean success = assembler_macro_storage_append_macro_char(storage, match, token_char, token_cnt);
    if(!success)
    {
        free(token_char);
    }
    return success;
}

void assembler_macro_storage_remove_macro(assembler_macro_storage_t *storage,
                                          const char *match)
{
    assembler_macro_t *found = assembler_macro_storage_lookup(storage, match);
    if(found != NULL)
    {
        assembler_macro_dealloc(found);
        hashmap_dels(storage->macro_map, match);
    }
}
