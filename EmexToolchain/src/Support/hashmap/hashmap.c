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

#include <stdbool.h>
#include <string.h>
#include <EmexToolchain/Support/hashmap/hashmap.h>

typedef struct hashmap_bucket {
    UInt64 hash;
    void *key;
    size_t klen;
    void *val;
} hashmap_bucket_t;

typedef struct hashmap {
    hashmap_bucket_t *buckets;
    size_t mask;
    size_t count;
} hashmap_t;

#define HASHMAP_INIT_CAP    16

static UInt64 hashmap_hash(const void *key,
                             size_t len)
{
    const UInt8 *p = (const UInt8 *)key;
    UInt64 h = 0xcbf29ce484222325ULL;
    for(size_t i = 0; i < len; i++)
    {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

hashmap_t *hashmap_alloc()
{
    hashmap_t *m = (hashmap_t*)malloc(sizeof *m);
    if(!m)
    {
        return NULL;
    }
    m->buckets = (hashmap_bucket_t *)calloc(HASHMAP_INIT_CAP, sizeof *m->buckets);
    if(!m->buckets)
    {
        free(m);
        return NULL;
    }
    m->mask  = HASHMAP_INIT_CAP - 1;
    m->count = 0;
    return m;
}

void hashmap_dealloc(hashmap_t *m)
{
    if(!m)
    {
        return;
    }
    for (size_t i = 0; i <= m->mask; i++)
    {
        free(m->buckets[i].key);
    }
    free(m->buckets);
    free(m);
}

size_t hashmap_count(const hashmap_t *m)
{
    return m->count;
}

void *hashmap_gets(hashmap_t *m,
                   const char *k)
{
    return hashmap_get(m, k, strlen(k));
}

Boolean hashmap_puts(hashmap_t *m,
                  const char *k,
                  void *v)
{
    return hashmap_put(m, k, strlen(k), v);
}

Boolean hashmap_dels(hashmap_t *m,
                  const char *k)
{
    return hashmap_del(m, k, strlen(k));
}

static Boolean hashmap_resize(hashmap_t *m,
                           size_t newcap)
{
    hashmap_bucket_t *nb = (hashmap_bucket_t*)calloc(newcap, sizeof *nb);
    if(!nb)
    {
        return false;
    }
    size_t nmask = newcap - 1;
    for(size_t k = 0; k <= m->mask; k++)
    {
        hashmap_bucket_t b = m->buckets[k];
        if(!b.key)
        {
            continue;
        }
        size_t i = (size_t)b.hash & nmask;
        while(nb[i].key)
        {
            i = (i + 1) & nmask;
        }
        nb[i] = b;
    }
    free(m->buckets);
    m->buckets = nb;
    m->mask = nmask;
    return true;
}

void *hashmap_get(hashmap_t *m,
                  const void *key,
                  size_t klen)
{
    UInt64 h = hashmap_hash(key, klen);
    size_t i = (size_t)h & m->mask;
    for(;;)
    {
        hashmap_bucket_t *b = &m->buckets[i];
        if(!b->key)
        {
            return NULL;
        }
        if(b->hash == h && b->klen == klen && memcmp(b->key, key, klen) == 0)
        {
            return b->val;
        }
        i = (i + 1) & m->mask;
    }
}

Boolean hashmap_put(hashmap_t *m,
                 const void *key,
                 size_t klen,
                 void *val)
{
    if((m->count + 1) * 4 >= (m->mask + 1) * 3)
    {
        hashmap_resize(m, (m->mask + 1) * 2);
    }
 
    UInt64 h = hashmap_hash(key, klen);
    size_t i = (size_t)h & m->mask;
    for(;;)
    {
        hashmap_bucket_t *b = &m->buckets[i];
        if(!b->key)
        {
            void *kc = malloc(klen ? klen : 1);
            if(!kc)
            {
                return false;
            }
            memcpy(kc, key, klen);
            b->hash = h; b->key = kc; b->klen = klen; b->val = val;
            m->count++;
            return true;
        }
        if(b->hash == h && b->klen == klen && memcmp(b->key, key, klen) == 0)
        {
            b->val = val;
            return false;
        }
        i = (i + 1) & m->mask;
    }
}

Boolean hashmap_del(hashmap_t *m,
                 const void *key,
                 size_t klen)
{
    UInt64 h = hashmap_hash(key, klen);
    size_t i = (size_t)h & m->mask;
    for(;;)
    {
        hashmap_bucket_t *b = &m->buckets[i];
        if(!b->key)
        {
            return false;
        }
        if(b->hash == h && b->klen == klen && memcmp(b->key, key, klen) == 0)
        {
            break;
        }
        i = (i + 1) & m->mask;
    }

    free(m->buckets[i].key);
    size_t hole = i;
    size_t j = i;
    for(;;)
    {
        m->buckets[hole].key = NULL;
        for(;;)
        {
            j = (j + 1) & m->mask;
            if(!m->buckets[j].key)
            {
                m->count--;
                return true;
            }
            size_t home = (size_t)m->buckets[j].hash & m->mask;
            Boolean can_move = (hole < j) ? (home <= hole || home > j) : (home <= hole && home > j);
            if(can_move)
            {
                break;
            }
        }
        m->buckets[hole] = m->buckets[j];
        hole = j;
    }
}

hashmap_iter_t hashmap_iter_create(hashmap_t *m)
{
    hashmap_iter_t it = { m, 0 };
    return it;
}

Boolean hashmap_next(hashmap_iter_t *it,
                  const void **key,
                  size_t *klen,
                  void **val)
{
    hashmap_t *m = it->m;
    while(it->i <= m->mask)
    {
        hashmap_bucket_t *b = &m->buckets[it->i++];
        if(b->key)
        {
            if(key)
            {
                *key  = b->key;
            }
            if(klen)
            {
                *klen = b->klen;
            }
            if(val)
            {
                *val  = b->val;
            }
            return true;
        }
    }
    return false;
}
