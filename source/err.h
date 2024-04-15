#pragma once

#include <jack/jack.h>

typedef struct result_t {
    enum {
        OK,
        ERR_JACK,
        ERR_LIBC,
        ERR_CMDLINE,
        ERR_KV,
        ERR_UNKNOWN_WAVE,
    } kind;

    union {
        jack_status_t err_jack;  
        int err_libc;
        char *arg;
        char *wave_name;

        struct {
            enum {
                ERR_KV_EMPTY,
                ERR_KV_NO_VAL,
                ERR_KV_UNKNOWN_KEY,
            } kind;

            char *expr;
        } kv;
    };
} result_t;

#define OK_VAL ((result_t){ .kind = OK })

#define JACK_ERR(status) ((result_t){ .kind = ERR_JACK, .err_jack = (status)})
#define LIBC_ERR(err) ((result_t){ .kind = ERR_LIBC, .err_libc = (err)})

#define IS_OK .kind == OK
#define IS_ERR .kind != OK

void report_result(result_t info);

#define UNWRAP(res) {result_t result = res; if ((result) IS_ERR) \
    {log_error("fatal error"); report_result(result); exit(1);}}

#define TRY(res) {result_t result = (res); if (result IS_ERR) return result;}
