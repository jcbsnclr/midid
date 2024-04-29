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

#include <assert.h>
#include <mem.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN 32

result_t mem_init(mem_pool_t *pool, size_t size) {
    void *buf = calloc(size, 1);

    if (!buf) return (result_t){.kind = ERR_MEM_POOL};

    pool->buf = buf;
    pool->cap = size;
    pool->ptr = 0;

    return OK_VAL;
}

void mem_free(mem_pool_t *pool) {
    assert(pool->buf);
    free(pool->buf);
    pool->buf = NULL;
}

result_t mem_alloc(mem_pool_t *pool, size_t size, void **out) {
    assert(pool->buf);

    if (pool->ptr + size >= pool->cap) return (result_t){.kind = ERR_OOM};

    size_t chunks = size / ALIGN;
    if (size % ALIGN != 0) chunks += 1;

    *out = (uint8_t *)pool->buf + pool->ptr;

    pool->ptr += chunks * ALIGN;

    return OK_VAL;
}

result_t mem_alloc_str(mem_pool_t *pool, char *str, size_t len, char **out) {
    assert(pool->buf);

    if (pool->ptr + len >= pool->cap) return (result_t){.kind = ERR_OOM};

    char *buf;
    TRY(mem_alloc(pool, len, (void **)&buf));

    strncpy(buf, str, len);

    *out = buf;

    return OK_VAL;
}
