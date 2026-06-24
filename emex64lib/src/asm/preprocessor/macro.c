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

#include <string.h>
#include <emex64lib/asm/preprocessor/macro.h>

assembler_macro_t *assembler_macro_alloc(const char *match,
                                         const char **inject_token,
                                         uint64_t token_cnt)
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

bool assembler_macro_storage_append_macro_char(assembler_macro_storage_t *storage,
                                               const char *match,
                                               const char **token,
                                               uint64_t token_cnt)
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

bool assembler_macro_storage_append_macro(assembler_macro_storage_t *storage,
                                          const char *match,
                                          assembler_token_t **token,
                                          uint64_t token_cnt)
{
    /* checking if it is already defined */
    const char **token_char = calloc(token_cnt, sizeof(const char *));
    if(token_char == NULL)
    {
        return false;
    }

    for(uint64_t i = 0; i < token_cnt; i++)
    {
        token_char[i] = token[i]->str;
    }

    bool success = assembler_macro_storage_append_macro_char(storage, match, token_char, token_cnt);
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
