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
