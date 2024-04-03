#pragma once

#include <stdint.h>

typedef enum log_level_t {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

void log_inner(log_level_t level, const char *filename, uint32_t line, const char *func, char *fmt, ...);
void log_line(char *fmt, ...);

#define log_trace(...) log_inner(LOG_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_debug(...) log_inner(LOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_info(...)  log_inner(LOG_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_warn(...)  log_inner(LOG_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_error(...) log_inner(LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
