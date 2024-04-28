#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <log.h>
#include <map.h>
#include <mem.h>
#include <parse.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "err.h"

static size_t resolve(char *src, size_t pos) {
    size_t line = 1;

    for (size_t i = 0; i < pos && src[i]; i++)
        if (src[i] == '\n') line++;

    return line;
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
    return c != '\n' && isspace(c);
}

static bool is_not_nl(char c) {
    return c != '\n';
}

// static bool is_not_ws(char c) {
//     return !isspace(c);
// }

static bool is_not_tok_end(char c) {
    return !isspace(c) && c != '#';
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

static bool skip_comment(parser_t *p) {
    bool taken = take_ifc(p, '#');
    take_while(p, is_not_nl);
    return taken;
}

static bool is_eof(parser_t *p) {
    return p->ptr >= p->len;
}

#define PARSE_ERR(k, l, f, ...) ERR_VAL(k, "line %lu: " f, (l), __VA_ARGS__)

#define TRY_IFC(p, c)        \
    if (!take_ifc((p), (c))) \
    return PARSE_ERR(ERR_EXPECTED_CHAR, resolve((p)->src, (p)->ptr), "expected char %c'", c)

#define FAIL_EOF(p)  \
    if (is_eof((p))) \
    return PARSE_ERR(ERR_UNEXPECTED_EOF, resolve(p->src, p->ptr), "unexpected EOF", "")

static result_t take_ident(parser_t *p, size_t *start, size_t *len) {
    *start = p->ptr;
    *len = 0;

    if (p->ptr >= p->len || !take_if(p, is_ident_start)) {
        size_t line = resolve(p->src, p->ptr);
        return PARSE_ERR(ERR_EXPECTED_IDENT, line, "expected identifier", "");
    }

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

    size_t line = resolve(p->src, p->ptr);
    return PARSE_ERR(
        ERR_EXPECTED_LIT, line, "expected literal '%s', found '%.*s'", lit, len, p->src + start);
}

static result_t take_num(parser_t *p, size_t *start, size_t *len) {
    *start = p->ptr;
    *len = 0;

    size_t line = resolve(p->src, *start);
    if (p->ptr >= p->len)
        return PARSE_ERR(ERR_EXPECTED_NUM, line, "expected number, found EOF", "");

    if (p->ptr + 1 < p->len && strncmp(p->src + *start, "->", 2) == 0)
        return PARSE_ERR(ERR_EXPECTED_NUM, line, "expected number, found '->'", "");

    take_ifc(p, '-');
    take_ifc(p, '+');

    if (!take_while(p, is_digit))
        return PARSE_ERR(ERR_EXPECTED_NUM, line, "expected number, found '%c'", p->src[*start]);

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

    size_t name = start;
    size_t name_len = len;

    env_t *env;
    env_stage_t *cur, *last;

    TALLOC(&st->pool, &env);
    TALLOC(&st->pool, &cur);

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

    skip_comment(p);

    log_trace("env %.*s:", name_len, p->src + name);
    env_stage_t *x = env->start;
    for (int i = 0; x; i++) {
        log_line("  [%d] time = %lu, amp = %f", i, x->time, x->amp);
        x = x->next;
    }

    TRY(map_insert(&st->map, p->src + name, name_len, (void *)env, OBJ_ENV));

    map_print(&st->map);

    skip_ws(p);

    return OK_VAL;
}

static result_t parse_pair(parser_t *p, size_t *key, size_t *key_len, size_t *val,
                           size_t *val_len) {
    skip_ws(p);
    TRY(take_ident(p, key, key_len));
    skip_ws(p);
    TRY_IFC(p, '=');
    skip_ws(p);

    *val = p->ptr;
    if (!take_while(p, is_not_tok_end))
        return PARSE_ERR(ERR_EXPECTED_VALUE,
                         resolve(p->src, p->ptr),
                         "key '%.*s' missing value",
                         key_len,
                         p->src + *key);
    *val_len = p->ptr - *val;

    return OK_VAL;
}

result_t parse_fields(state_t *st, parser_t *p, parse_field_t *fields) {
    bool taken = false;

    size_t line = resolve(p->src, p->ptr);

    for (;;) {
        size_t key, key_len, val, val_len;

        result_t res = parse_pair(p, &key, &key_len, &val, &val_len);

        if (res IS_ERR) {
            if (res.kind == ERR_EXPECTED_IDENT) break;

            return res;
        }

        // if (p->ptr >= p->len) break;

        log_warn("key = %.*s, val = %.*s", key_len, p->src + key, val_len, p->src + val);

        bool matches = false;

        for (parse_field_t *cur = fields; cur->key; cur++) {
            if (!cur->taken && strncmp(cur->key, p->src + key, key_len) == 0) {
                matches = true;
                TRY(cur->parser(&st->pool, p, p->src + val, val_len, cur->out));
                cur->taken = true;
            }
        }

        if (!matches)
            return PARSE_ERR(ERR_KEY_INVALID, line, "unknown key '%.*s'", key_len, p->src + key);

        taken = true;
    }

    for (parse_field_t *cur = fields; cur->key; cur++)
        if (cur->required && !cur->taken) {
            return PARSE_ERR(ERR_KEY_REQUIRED, line, "key '%s' required", cur->key);
        }

    if (!taken) return PARSE_ERR(ERR_EXPECTED_PAIR, line, "expected at least 1 field", "");

    return OK_VAL;
}

result_t parse_osc(state_t *st, parser_t *p) {
    (void)st;
    size_t name, name_len;
    TRY(take_name(p, &name, &name_len));

    osc_t *osc;
    TALLOC(&st->pool, &osc);

    osc->base = 0;
    osc->bias = 0.0;
    osc->kind = OSC_SIN;
    osc->vol = 0.0;

    parse_field_t fields[] = {
        {.key = "wave", .out = &osc->kind, .required = true, .parser = extract_wave},
        {.key = "base", .out = &osc->base, .parser = extract_int},
        {NULL}};

    TRY(parse_fields(st, p, fields));

    log_info("WAVE KIND %s", osc_kind_str[osc->kind]);

    skip_comment(p);

    TRY(map_insert(&st->map, p->src + name, name_len, osc, OBJ_OSC));

    return OK_VAL;
}

// extractors
result_t extract_any(mem_pool_t *pool, parser_t *p, char *str, size_t len, void *out) {
    return mem_alloc_str(pool, str, len, (char **)out);
}

#define MIN(x, y) (x < y ? x : y)

result_t extract_wave(mem_pool_t *pool, parser_t *p, char *str, size_t len, void *out) {
    bool matched = false;
    osc_kind_t *o = (osc_kind_t *)out;

    size_t line = resolve(p->src, p->ptr);

    for (size_t i = 0; i < OSC_MAX; i++) {
        if (strncmp(str, osc_kind_str[i], len) == 0) {
            *o = i;
            matched = true;
            break;
        }
    }

    if (!matched)
        return PARSE_ERR(ERR_EXPECTED_WAVE, line, "expected wave, found '%.*s'", len, str);

    return OK_VAL;
}

result_t extract_int(mem_pool_t *pool, parser_t *p, char *str, size_t len, void *out) {
    char *endptr;
    long long i = strtoll(str, &endptr, 10);

    size_t line = resolve(p->src, p->ptr);

    if ((i == LLONG_MAX || i == LLONG_MIN) || endptr == str)
        return PARSE_ERR(ERR_INVALID_INT, line, "invalid integer '%.*s'", len, str);

    *(int *)out = i;

    return OK_VAL;
}
