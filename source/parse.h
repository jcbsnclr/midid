#pragma once

#include <err.h>
#include <audio.h>

// result_t parse_cfg(state_t *st, char *cfg);
typedef struct {
    char *src;
    size_t len, ptr;
} parser_t;

void parser_init(parser_t *p, char *src);

result_t parse_env(state_t *st, parser_t *p);
result_t parse_osc(state_t *st, parser_t *p);
result_t parse_inst(state_t *st, parser_t *p);
result_t parse_chan(state_t *st, parser_t *p);
