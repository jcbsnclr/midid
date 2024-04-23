#include <assert.h>
#include <ctype.h>
#include <log.h>
#include <map.h>
#include <mem.h>
#include <parse.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "err.h"

// maximum number of envelope stages in an envelope (arbitrary)
#define ENV_STAGES 32

#define PARSER_ERR(k)                                \
    (result_t) {                                     \
        .kind = ERR_PARSER, .parser = {.kind = (k) } \
    }

typedef bool (*ccond_t)(char);

static bool is_ident_start(char c) {
    return isalpha(c) || c == '_';
}

static bool is_ident_body(char c) {
    return is_ident_start(c) || isdigit(c);
}

static bool is_digit(char c) {
    return isdigit(c);
}

static bool is_ws(char c) {
    return isspace(c);
}

static bool take_if(parser_t *p, ccond_t cond) {
    if (p->ptr >= p->len) return false;

    if (cond(p->src[p->ptr])) {
        p->ptr++;
        return true;
    }

    return false;
}

static bool take_ifc(parser_t *p, char c) {
    if (p->ptr >= p->len) return false;

    if (p->src[p->ptr] == c) {
        p->ptr++;
        return true;
    }

    return false;
}

static bool take_while(parser_t *p, ccond_t cond) {
    bool taken = false;

    while (take_if(p, cond)) taken = true;

    return taken;
}

static void skip_ws(parser_t *p) {
    take_while(p, is_ws);
}

static bool is_eof(parser_t *p) {
    return p->ptr >= p->len;
}

#define PARSE_ERR(k, w, s, ...)                                                             \
    (result_t) {                                                                            \
        .kind = ERR_PARSER, .parser = {.kind = (k), .where = (w), .src = (s), __VA_ARGS__ } \
    }

#define TRY_IFC(p, c) \
    if (!take_ifc((p), (c))) return PARSE_ERR(ERR_EXPECTED_CHAR, p->ptr, p->src, .expected = (c))

#define FAIL_EOF(p) \
    if (is_eof((p))) return PARSE_ERR(ERR_UNEXPECTED_EOF, p->ptr, p->src)

static result_t take_ident(parser_t *p, size_t *start, size_t *len) {
    *start = p->ptr;
    *len = 0;

    if (p->ptr >= p->len || !take_if(p, is_ident_start))
        return PARSE_ERR(ERR_EXPECTED_IDENT, *start, p->src);

    take_while(p, is_ident_body);

    *len = p->ptr - *start;

    return OK_VAL;
}

static result_t take_name(parser_t *p, size_t *start, size_t *len) {
    skip_ws(p);

    TRY(take_ident(p, start, len));

    TRY_IFC(p, ':');

    return OK_VAL;
}

static result_t take_lit(parser_t *p, char *lit) {
    size_t start = p->ptr;
    size_t len = strlen(lit);

    if (strncmp(p->src + p->ptr, lit, len) == 0) {
        p->ptr += len;
        return OK_VAL;
    }

    return PARSE_ERR(ERR_EXPECTED_LIT, start, p->src, .exp_lit = lit);
}

static result_t take_num(parser_t *p, size_t *start, size_t *len) {
    *start = p->ptr;
    *len = 0;

    if (p->ptr >= p->len) return PARSE_ERR(ERR_EXPECTED_NUM, p->ptr, p->src);

    if (p->ptr + 1 < p->len && strncmp(p->src + *start, "->", 2) == 0)
        return PARSE_ERR(ERR_EXPECTED_NUM, p->ptr, p->src);

    take_ifc(p, '-');
    take_ifc(p, '+');

    if (!take_while(p, is_digit)) return PARSE_ERR(ERR_EXPECTED_NUM, p->ptr, p->src);

    if (take_ifc(p, '.')) take_while(p, is_digit);

    *len = p->ptr - *start;

    return OK_VAL;
}

static result_t parse_env_stage(parser_t *p, size_t *start, env_stage_t *env) {
    skip_ws(p);
    *start = p->ptr;

    FAIL_EOF(p);

    if (take_lit(p, "SUST") IS_OK) {
        env->time = ENV_SUSTAIN;
        env->amp = 0.0;
        return OK_VAL;
    }

    size_t n1, l1;
    size_t n2, l2;

    TRY(take_num(p, &n1, &l1));
    TRY_IFC(p, 's');
    TRY(take_num(p, &n2, &l2));

    float time;
    assert(sscanf(p->src + *start, "%fs%f", &time, &env->amp) == 2);

    env->time = time * SECS(1);

    return OK_VAL;
}

void parser_init(parser_t *p, char *src) {
    p->len = strlen(src);
    p->ptr = 0;
    p->src = src;
}

result_t parse_env(state_t *st, parser_t *p) {
    size_t start = 0;
    size_t len = 0;
    TRY(take_name(p, &start, &len));

    log_debug("len = %lu", len);

    size_t name = start;
    size_t name_len = len;

    env_t *env;
    env_stage_t *cur, *last;

    TALLOC(&st->pool, &env);
    TALLOC(&st->pool, &cur);

    log_error("%p", env);

    last = cur;

    env->start = cur;
    env->done = NULL;

    for (;;) {
        cur->next = NULL;

        TRY(parse_env_stage(p, &start, cur));

        if (cur != last) last->next = cur;

        if (last->time == ENV_SUSTAIN) env->done = cur;

        last = cur;
        TALLOC(&st->pool, &cur);

        // log_info("time = %lu, %f", cur->time, cur->amp);

        skip_ws(p);

        if (take_lit(p, "->") IS_ERR) break;
    }

    env_stage_t *x = env->start;
    while (x) {
        log_info("time = %lu, %f", x->time, x->amp);
        x = x->next;
    }

    TRY(map_insert(&st->map, p->src + name, name_len, (void *)env, OBJ_ENV));

    map_print(&st->map);

    return OK_VAL;
}
