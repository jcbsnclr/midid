#include <err.h>
#include <jack/types.h>
#include <log.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

result_t err_impl(err_kind_t kind, size_t line, const char *file, const char *func, char *fmt,
                  ...) {
    result_t err;

    err.kind = kind;
    err.line = line;
    err.file = file;
    err.func = func;

    va_list args1, args2;
    va_start(args1, fmt);
    va_copy(args2, args1);

    int len = vsnprintf(NULL, 0, fmt, args1);
    err.msg = malloc(len + 1);
    vsprintf(err.msg, fmt, args2);
    err.msg[len] = '\0';

    va_end(args1);
    return err;
}

void report_result(result_t info) {
    log_inner(LOG_ERROR, info.file, info.line, info.func, "%s", info.msg);
    free(info.msg);
}
