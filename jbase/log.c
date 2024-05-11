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

//
// log.c: logging facilities
//
// provides utilities for logging messages to stderr. filters messages if their level
// is lower than the filter provided by the `LOG_FILTER` env var at initialisation
//

#include <jbase.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// used to map from jb__llevel_t to a coloured string
static const char *log_level_str[] = {[JB_TRACE] = JB_FG_MAGENTA "TRACE" JB_RESET,
                                      [JB_DEBUG] = JB_FG_CYAN "DEBUG" JB_RESET,
                                      [JB_INFO] = JB_FG_GREEN "INFO " JB_RESET,
                                      [JB_WARN] = JB_FG_YELLOW "WARN " JB_RESET,
                                      [JB_ERROR] = JB_FG_RED "ERROR" JB_RESET};

// global loging filter
static jb_llevel_t filter;

void jb_log_init() {
    char *level;

    if ((level = getenv("LOG_FILTER"))) {
        // if LOG_FILTER env variable has a value, pasrse it and set the filter accordingly
        if (strcmp(level, "trace") == 0)
            filter = JB_TRACE;
        else if (strcmp(level, "debug") == 0)
            filter = JB_DEBUG;
        else if (strcmp(level, "info") == 0)
            filter = JB_INFO;
        else if (strcmp(level, "warn") == 0)
            filter = JB_WARN;
        else if (strcmp(level, "error") == 0)
            filter = JB_ERROR;
        else
            // default to showing most useful information
            filter = JB_INFO;
    }
}

// print a line to stderr, offset with a whitespace and an arrow, and printf-formatted string
static void vlog_line(char *fmt, va_list args) {
    fprintf(stderr, "               ⤷ ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void jb_log_line(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vlog_line(fmt, args);

    va_end(args);
}

void jb_log_inner(jb_llevel_t level, const char *filename, uint32_t line, const char *func,
                  char *fmt, ...) {
    // filter log messages below filter severity
    if (level < filter) return;

    time_t raw_time;
    struct tm *ti;

    // get current time locally
    time(&raw_time);
    ti = localtime(&raw_time);

    // print metadata to stderr
    fprintf(stderr,
            JB_FG_BLACK_BRIGHT "(%0*d:%0*d:%0*d) " JB_RESET "%s " JB_BOLD "[" JB_FG_CYAN_BRIGHT
                               "%s " JB_RESET JB_BOLD "%s:%d]" JB_RESET JB_FG_WHITE,
            2,
            ti->tm_hour,
            2,
            ti->tm_min,
            2,
            ti->tm_sec,
            log_level_str[level],
            func,
            filename,
            line);

    fputc('\n', stderr);

    // if a format string is provided, then print a line to stderr with printf-formatted message
    if (fmt) {
        va_list args;
        va_start(args, fmt);

        vlog_line(fmt, args);

        va_end(args);
    }
}
