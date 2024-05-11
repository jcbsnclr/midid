#include <ansi.h>
#include <log.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *log_level_str[] = {[LOG_TRACE] = ANSI_FG_MAGENTA "TRACE" ANSI_RESET,
                                      [LOG_DEBUG] = ANSI_FG_CYAN "DEBUG" ANSI_RESET,
                                      [LOG_INFO] = ANSI_FG_GREEN "INFO " ANSI_RESET,
                                      [LOG_WARN] = ANSI_FG_YELLOW "WARN " ANSI_RESET,
                                      [LOG_ERROR] = ANSI_FG_RED "ERROR" ANSI_RESET};

static log_level_t filter;

void log_init() {
    char *level;

    if ((level = getenv("LOG_FILTER"))) {
        if (strcmp(level, "trace") == 0)
            filter = LOG_TRACE;
        else if (strcmp(level, "debug") == 0)
            filter = LOG_DEBUG;
        else if (strcmp(level, "info") == 0)
            filter = LOG_INFO;
        else if (strcmp(level, "warn") == 0)
            filter = LOG_WARN;
        else if (strcmp(level, "error") == 0)
            filter = LOG_ERROR;
        else
            filter = LOG_ERROR;
    }
}

static void vlog_line(char *fmt, va_list args) {
    fprintf(stderr, "               ⤷ ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void log_line(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vlog_line(fmt, args);

    va_end(args);
}

void log_inner(log_level_t level, const char *filename, uint32_t line, const char *func, char *fmt,
               ...) {
    if (level < filter) return;

    time_t raw_time;
    struct tm *ti;

    time(&raw_time);
    ti = localtime(&raw_time);

    fprintf(stderr,
            ANSI_FG_BLACK_BRIGHT "(%0*d:%0*d:%0*d) " ANSI_RESET "%s " ANSI_BOLD
                                 "[" ANSI_FG_CYAN_BRIGHT "%s " ANSI_RESET ANSI_BOLD
                                 "%s:%d]" ANSI_RESET ANSI_FG_WHITE,
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
    if (fmt) {
        va_list args;
        va_start(args, fmt);

        vlog_line(fmt, args);

        va_end(args);
    }
}
