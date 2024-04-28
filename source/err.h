#pragma once

#include <jack/jack.h>

typedef
    enum {
        OK,
        ERR_JACK,
        ERR_LIBC,
        ERR_UNKNOWN_WAVE,
        ERR_MEM_POOL,
        ERR_OOM,

        ERR_EXPECTED_NAME,
        ERR_EXPECTED_CHAR,
        ERR_EXPECTED_NUM,
        ERR_EXPECTED_LIT,
        ERR_EXPECTED_IDENT,
        ERR_EXPECTED_INT,
        ERR_EXPECTED_BYTE,
        ERR_EXPECTED_PAIR,
        ERR_EXPECTED_VALUE,

        ERR_INVALID_CHAN,
        ERR_INVALID_INT,
        ERR_INVALID_NUM,

        ERR_KEY_INVALID,

        ERR_UNEXPECTED_EOF,

        ERR_KEY_REQUIRED,
        ERR_EXPECTED_WAVE,
        ERR_PARSER_EOF,
        ERR_PARSER_FAILED,
    } err_kind_t;

typedef struct result_t {
    err_kind_t kind;
    char *msg;

    size_t line;
    const char *file;
    const char *func;
} result_t;

#define OK_VAL ((result_t){ .kind = OK })
#define ERR_VAL(k, ...) err_impl((k), __LINE__, __FILE__, __func__, __VA_ARGS__)

#define JACK_ERR(status) ((result_t){ .kind = ERR_JACK, .err_jack = (status)})
#define LIBC_ERR(err) ((result_t){ .kind = ERR_LIBC, .err_libc = (err)})

#define IS_OK .kind == OK
#define IS_ERR .kind != OK

#define UNWRAP(res) {result_t result = res; if ((result) IS_ERR) \
    {report_result(result); exit(1);}}

#define TRY(res) {result_t result = (res); if (result IS_ERR) return result;}

void report_result(result_t info);
result_t err_impl(err_kind_t kind, size_t line, const char *file, const char *func, char *fmt, ...);

