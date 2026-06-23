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

#ifndef EMEX64_HASHMAP_HASHMAP_H
#define EMEX64_HASHMAP_HASHMAP_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct hashmap hashmap_t;
typedef struct hm_iter {
    hashmap_t *m;
    size_t i;
} hashmap_iter_t;

hashmap_t *hashmap_alloc(void);
void hashmap_dealloc(hashmap_t *m);

void *hashmap_get(hashmap_t *m, const void *key, size_t klen);
bool hashmap_put(hashmap_t *m, const void *key, size_t klen, void *val);
bool hashmap_del(hashmap_t *m, const void *key, size_t klen);
size_t hashmap_count(const hashmap_t *m);

void *hashmap_gets(hashmap_t *m, const char *k);
bool hashmap_puts(hashmap_t *m, const char *k, void *v);
bool hashmap_dels(hashmap_t *m, const char *k);

hashmap_iter_t hashmap_iter_create(hashmap_t *m);

bool hashmap_next(hashmap_iter_t *it, const void **key, size_t *klen, void **val);

#endif /* EMEX64_HASHMAP_HASHMAP_H */
