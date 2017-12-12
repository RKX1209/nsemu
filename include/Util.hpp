#ifndef _UTIL_HPP
#define _UTIL_HPP

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#define LINE_BREAK 15

enum RunLevel{
  RUN_LEVEL_RELEASE = 0,
  RUN_LEVEL_DEBUG,
};

static RunLevel curlevel = RUN_LEVEL_DEBUG;

static void util_print(RunLevel level, const char *format, ...) {
  if (curlevel >= level) {
    va_list va;
    va_start (va, format);
    vprintf (format, va);
    va_end (va);
  }
}

#define debug_print(format, ...) util_print (RUN_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define ns_print(format, ...) util_print (RUN_LEVEL_RELEASE, format, ##__VA_ARGS__)
#define ns_abort(format, ...) \
    ns_print ("%s: ", __func__); \
    ns_print (format, ##__VA_ARGS__); \
    abort()

inline void bindump(uint8_t *ptr, size_t size) {
  int i = 0;
  while (i < size) {
    debug_print ("%02x", ptr[i]);
    if ((i + 1) % LINE_BREAK == 0)
      debug_print ("\n");
    else
      debug_print (" ");
    i++;
  }
  if (i % LINE_BREAK != 0)
    debug_print ("\n");
}

inline int32_t host_order32(const char *b) {
  return ((b[3]) << 24) | ((b[2]) << 16) | ((b[1]) << 8) | (b[0]);
}

#endif