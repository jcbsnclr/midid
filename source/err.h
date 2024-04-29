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

#define IS_OK .kind == OK
#define IS_ERR .kind != OK

#define UNWRAP(res) {result_t result = res; if ((result) IS_ERR) \
    {report_result(result); exit(1);}}

#define TRY(res) {result_t result = (res); if (result IS_ERR) return result;}

void report_result(result_t info);
result_t err_impl(err_kind_t kind, size_t line, const char *file, const char *func, char *fmt, ...);

