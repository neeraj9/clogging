/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "logging_common.h"

#include <stdio.h> /* snprintf() */
#include <time.h>  /* gmtime_r() */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define THREAD_LOCAL __declspec(thread)
/* Windows doesn't have gmtime_r, provide a wrapper */
#ifndef HAVE_GMTIME_R
static inline struct tm *gmtime_r(const time_t *timep, struct tm *result) {
  struct tm *ret = gmtime(timep);
  if (ret && result) {
    *result = *ret;
    return result;
  }
  return NULL;
}
#endif
#else
#define THREAD_LOCAL __thread
#endif

/*
 * Get the string representation of logging level.
 */
const char *get_log_level_as_cstring(enum LogLevel level) {
  /*
   * A better alternative would be to use thread local storage,
   * via the __thread gnu directive. This will add a cost to
   * it though, which is inherently due to the way thread local
   * storage is relaized by the gnu c compiler.
   * In case this function is called infrequently, which will
   * ultimately depend on calls to logging api then use the
   * thread local approach.
   *
   */

  /* The mapping is based on the enumeration values as in
   * logging_common.h, so update this mapping table to
   * match that.
   */
  static THREAD_LOCAL const char *level_to_str[] = {
      "ERROR", /* 0 */
      "WARN",  /* 1 */
      "INFO",  /* 2 */
      "DEBUG"  /* 3 */
  };

  return level_to_str[level];
}

int time_to_cstr(time_t *t, char *timestr, int maxlen) {
  struct tm tms;

  gmtime_r(t, &tms);
  return snprintf(timestr, maxlen, "%04d-%02d-%02dT%02d:%02d:%02d+00:00",
           tms.tm_year + 1900, tms.tm_mon + 1, tms.tm_mday, tms.tm_hour,
           tms.tm_min, tms.tm_sec);
}

#ifdef __cplusplus
}
#endif
