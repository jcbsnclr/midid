#pragma once

#include <mem.h>


#define BUCKETS 32

typedef uint64_t hash_t;

typedef enum obj_kind_t {
    OBJ_OSC,
    OBJ_INST,
    OBJ_ENV,
    OBJ_MAX,
} obj_kind_t;

typedef struct map_bucket_t {
    char *key;
    size_t key_len;
    hash_t hash;
    obj_kind_t kind;
    void *val;
    struct map_bucket_t *next;
} map_bucket_t;

typedef struct map_t {
    mem_pool_t pool;
    map_bucket_t *buckets[BUCKETS];
} map_t;

extern char *obj_kind_str[OBJ_MAX];

result_t map_init(map_t *map);
void map_free(map_t *map);

void *map_get(map_t *map, char *key, size_t len, obj_kind_t kind);
result_t map_insert(map_t *map, char *key, size_t len, void *val, obj_kind_t kind);

void *map_getc(map_t *map, char *key, obj_kind_t kind);
result_t map_insertc(map_t *map, char *key, void *val, obj_kind_t kind);

void map_print(map_t *map);
