#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>

typedef enum {
  INFO,
  ERROR,
  DEBUG
} log_level_t;

#ifndef LOG_LEVEL
#define LOG_LEVEL INFO
#endif

void info(const char *fmt, ...);
void error(const char *fmt, ...);
void debug(const char *fmt, ...);

static void write_log(const char *prefix, const char *fmt, va_list args);

#endif