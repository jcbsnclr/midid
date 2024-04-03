#include <log.h>
#include <ansi.h>

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

static const char *log_level_name[] = {
	[LOG_TRACE] = ANSI_FG_MAGENTA "TRACE" ANSI_RESET,
	[LOG_DEBUG] = ANSI_FG_CYAN    "DEBUG" ANSI_RESET,
	[LOG_INFO]  = ANSI_FG_GREEN   "INFO " ANSI_RESET,
	[LOG_WARN]  = ANSI_FG_YELLOW  "WARN " ANSI_RESET,
	[LOG_ERROR] = ANSI_FG_RED     "ERROR" ANSI_RESET
};

typedef struct logger_t {
    FILE *outp;
    log_level_t filter;
} logger_t;

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

void log_inner(log_level_t level, const char *filename, uint32_t line, const char *func, char *fmt, ...) {
    (void)log_level_name;

    time_t raw_time;
    struct tm *ti;

    time(&raw_time);
    ti = localtime(&raw_time);

    fprintf(
        stderr,
        ANSI_FG_BLACK_BRIGHT "(%0*d:%0*d:%0*d) " ANSI_RESET 
        "%s " ANSI_BOLD "[" ANSI_FG_CYAN_BRIGHT 
        "%s " ANSI_RESET ANSI_BOLD 
        "%s:%d]" ANSI_RESET ANSI_FG_WHITE,
        2, ti->tm_hour, 2, ti->tm_min, 2, ti->tm_sec,
        log_level_name[level],
        func, filename, line
    );

    fputc('\n', stderr);
    if (fmt) {
        va_list args;
        va_start(args, fmt);

        vlog_line(fmt, args);

        va_end(args);
    }
}
