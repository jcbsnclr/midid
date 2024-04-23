#pragma once

#include <stddef.h>

#include <err.h>

typedef struct mem_pool_t {
    void *buf;
    size_t cap, ptr;
} mem_pool_t;

result_t mem_init(mem_pool_t *pool, size_t usize);
void mem_free(mem_pool_t *pool);
result_t mem_alloc(mem_pool_t *pool, size_t size, void **out);
result_t mem_alloc_str(mem_pool_t *pool, char *str, char **out);

#define ALLOC(pool, out) mem_alloc((pool), sizeof(**out), (void **)out)
#define TALLOC(pool, out) TRY(ALLOC(pool, out))
#define UALLOC(pool, out) UNWRAP(ALLOC(pool, out))

#define KiB(n) (n * 1024)
#define MiB(n) KiB((n))
