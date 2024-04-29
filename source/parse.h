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

#include <err.h>
#include <audio.h>

// result_t parse_cfg(state_t *st, char *cfg);
typedef struct {
    char *src;
    size_t len, ptr;
} parser_t;

typedef result_t (*parse_extract_t)(mem_pool_t *pool, parser_t *p, char *str, size_t len, void *out);

typedef struct parse_field_t {
    char *key;
    void *out;
    bool taken;
    bool required;
    parse_extract_t parser;
} parse_field_t;

#define FIELD_LAST (parse_field_t){ .key = NULL, .out = NULL, .taken = false, .required = false, .parser = NULL } 

void parser_init(parser_t *p, char *src);

result_t parse_env(state_t *st, parser_t *p);
result_t parse_osc(state_t *st, parser_t *p);
result_t parse_inst(state_t *st, parser_t *p);
result_t parse_chan(state_t *st, parser_t *p);

result_t extract_any(mem_pool_t *pool, parser_t *p, char *str, size_t len, void *out);
result_t extract_wave(mem_pool_t *pool, parser_t *p, char *str, size_t len, void *out);
result_t extract_int(mem_pool_t *pool, parser_t *p, char *str, size_t len, void *out);
result_t extract_byte(mem_pool_t *pool, parser_t *p, char *str, size_t len, void *out);
result_t extract_float(mem_pool_t *pool, parser_t *p, char *str, size_t len, void *out);

result_t parse_fields(state_t *st, parser_t *p, parse_field_t *fields);
