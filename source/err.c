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
