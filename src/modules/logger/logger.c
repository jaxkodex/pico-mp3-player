#include "logger.h"

#include <stdio.h>

void info(const char *fmt, ...)
{
  if (LOG_LEVEL < INFO) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  write_log("INFO", fmt, args);
  va_end(args);
}

void error(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  write_log("ERROR", fmt, args);
  va_end(args);
}

void debug(const char *fmt, ...)
{
  if (LOG_LEVEL < DEBUG) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  write_log("DEBUG", fmt, args);
  va_end(args);
}

static void write_log(const char *prefix, const char *fmt, va_list args)
{
  printf("%s: ", prefix);
  vprintf(fmt, args);
  printf("\n");
}