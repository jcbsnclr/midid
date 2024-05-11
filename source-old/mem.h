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
result_t mem_alloc_str(mem_pool_t *pool, char *str, size_t len, char **out);

#define ALLOC(pool, out) mem_alloc((pool), sizeof(**out), (void **)out)
#define TALLOC(pool, out) TRY(ALLOC(pool, out))
#define UALLOC(pool, out) UNWRAP(ALLOC(pool, out))

#define KiB(n) (n * 1024)
#define MiB(n) KiB((n))
