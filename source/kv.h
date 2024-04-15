#pragma once

#include <err.h>

typedef result_t (*kv_parse_t)(char *str, void *out);

typedef struct kv_field_t {
    char *key;
    void *out;
    kv_parse_t parser;
} kv_field_t;

result_t kv_any(char *str, void *out);
result_t kv_wave(char *str, void *out);
result_t kv_int(char *str, void *out);
result_t kv_byte(char *str, void *out);
result_t kv_float(char *str, void *out);

result_t kv_parse(kv_field_t *fields, char *str);
