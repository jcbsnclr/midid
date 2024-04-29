/*
 * midid - software MIDI synthesiser, utilising JACK
 * Copyright (C) 2024  Jacob Sinclair <jcbsnclr@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <err.h>
#include <map.h>
#include <mem.h>
#include <string.h>

#include "log.h"

#define MAP_POOL_SIZE MiB(8)

#define FNV_OFFSET_BASIS 0xcbf29ce484222325
#define FNV_PRIME 0x100000001b3

char *obj_kind_str[OBJ_MAX] = {[OBJ_OSC] = "osc", [OBJ_INST] = "inst", [OBJ_ENV] = "env"};

static hash_t fnv1a_buf(char *buf, size_t len) {
    hash_t hash = FNV_OFFSET_BASIS;

    for (size_t i = 0; i < len; i++) {
        hash ^= buf[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

#define MIN(x, y) (x < y ? x : y)

static map_bucket_t *find_bucket(map_t *map, char *key, size_t len, obj_kind_t kind) {
    hash_t hash = fnv1a_buf(key, len);
    map_bucket_t *cur = map->buckets[hash % BUCKETS];

    while (cur) {
        if (cur->hash == hash && cur->kind == kind &&
            strncmp(cur->key, key, MIN(len, cur->key_len)) == 0)
            break;
        cur = cur->next;
    }

    return cur;
}

void *map_get(map_t *map, char *key, size_t len, obj_kind_t kind) {
    map_bucket_t *bucket = find_bucket(map, key, len, kind);

    if (bucket) return bucket->val;

    return bucket;
}

result_t map_insert(map_t *map, char *key, size_t len, void *val, obj_kind_t kind) {
    hash_t hash = fnv1a_buf(key, len);

    map_bucket_t *bucket = find_bucket(map, key, len, kind);
    char *keyp;
    mem_alloc_str(&map->pool, key, len, &keyp);

    if (!bucket) {
        TALLOC(&map->pool, &bucket);

        bucket->hash = hash;
        bucket->key = keyp;
        bucket->key_len = len;
        bucket->kind = kind;
        bucket->next = map->buckets[hash % BUCKETS];

        map->buckets[hash % BUCKETS] = bucket;
    }

    bucket->val = val;

    return OK_VAL;
}

void map_print(map_t *map) {
    log_trace("map:");
    for (size_t i = 0; i < BUCKETS; i++) {
        map_bucket_t *b = map->buckets[i];

        while (b) {
            log_line(" * key = %.*s, kind = %s, val = %p",
                     b->key_len,
                     b->key,
                     obj_kind_str[b->kind],
                     b->val);

            b = b->next;
        }
    }
}

result_t map_init(map_t *map) {
    TRY(mem_init(&map->pool, MAP_POOL_SIZE));
    memset(map->buckets, 0, sizeof(map_bucket_t *) * BUCKETS);

    return OK_VAL;
}

void map_free(map_t *map) {
    mem_free(&map->pool);
    memset(map->buckets, 0, sizeof(map_bucket_t *) * BUCKETS);
}
