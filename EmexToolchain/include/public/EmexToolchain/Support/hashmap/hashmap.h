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

#ifndef EMEX64_HASHMAP_HASHMAP_H
#define EMEX64_HASHMAP_HASHMAP_H

#include <stddef.h>
#include <stdlib.h>
#include <EmexFoundation/EmexFoundation.h>

typedef struct hashmap hashmap_t;
typedef struct hm_iter {
    hashmap_t *m;
    EFSize i;
} hashmap_iter_t;

hashmap_t *hashmap_alloc(void);
void hashmap_dealloc(hashmap_t *m);

void *hashmap_get(hashmap_t *m, const void *key, EFSize klen);
Boolean hashmap_put(hashmap_t *m, const void *key, EFSize klen, void *val);
Boolean hashmap_del(hashmap_t *m, const void *key, EFSize klen);
EFSize hashmap_count(const hashmap_t *m);

void *hashmap_gets(hashmap_t *m, const char *k);
Boolean hashmap_puts(hashmap_t *m, const char *k, void *v);
Boolean hashmap_dels(hashmap_t *m, const char *k);

hashmap_iter_t hashmap_iter_create(hashmap_t *m);

Boolean hashmap_next(hashmap_iter_t *it, const void **key, EFSize *klen, void **val);

#endif /* EMEX64_HASHMAP_HASHMAP_H */
