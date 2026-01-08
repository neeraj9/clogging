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

#include "basic_logging.h"

#include <stdarg.h>   /* va_start() and friends */
#include <stdio.h>    /* fprintf() and friends */
#include <time.h>     /* time() */

#ifdef _WIN32
/* Use the WIN32_LEAN_AND_MEAN macro before including windows.h. This tells the
Windows header to exclude less commonly used APIs, including the old winsock.h:
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>  /* GetCurrentProcessId(), GetComputerNameExA() */
#else
#include <sys/types.h>
#include <unistd.h>   /* getpid(), gethostname() */
#endif

#define MAX_PROG_NAME_LEN 40
#define MAX_HOSTNAME_LEN 20

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The use of global static is tricky and prone to issues in MT (multi-threaded)
 * environments, but then the clogging_basic_init should be called ONLY once.
 * In this case there is no MT issue since
 * each of the thread will have its own copy of the file static variable.
 * The side effect is to call init_logging() separately for each of the
 * threads. This differs from the fact when thread local storage is not
 * used, when only the main thread should call init_logging() and only
 * once.
 *
 */
#ifdef _WIN32
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif

static THREAD_LOCAL char g_progname[MAX_PROG_NAME_LEN] = {0};
static THREAD_LOCAL char g_threadname[MAX_PROG_NAME_LEN] = {0};
static THREAD_LOCAL char g_hostname[MAX_HOSTNAME_LEN] = {0};
static THREAD_LOCAL int g_pid = 0;
static THREAD_LOCAL enum LogLevel g_level = DEFAULT_LOG_LEVEL;
/* Logging options */
static THREAD_LOCAL clogging_log_options_t g_log_options = {
  .color = 0,
  .json = 0,
  .prefix_fields_flag = CLOGGING_PREFIX_DEFAULT
};
/* safeguard calling init_logging multiple times */
static THREAD_LOCAL int g_is_logging_initialized = 0;

/* store the number of message dropped as a counter for
 * later statistics collection.
 */
static THREAD_LOCAL uint64_t g_basic_num_msg_drops = 0;

int clogging_basic_init(const char *progname, uint8_t progname_len,
                        const char *threadname, uint8_t threadname_len,
                        enum LogLevel level, const clogging_log_options_t *opts) {
  if (g_is_logging_initialized > 0) {
    fprintf(stderr, "logging is already initialized or in the"
                    " process of initialization.\n");
    return -1;
  }

  /* first thing to do is block any other thread in running logging
   * initialization (if that is done, though its bad and should be
   * done in the main thread ONLY.
   */
  g_is_logging_initialized = 1;

  /* Intentionally call the method so that any function static variables
   * in get_log_level_as_cstring() are correctly initialized.
   * Note that this should be done in any logging implementation (like
   * many other things), although the argument and the return values
   * are unimportant.
   */
  (void)get_log_level_as_cstring(LOG_LEVEL_ERROR);

  clogging_strtcpy(g_progname, progname, progname_len);
  clogging_strtcpy(g_threadname, threadname, threadname_len);
#ifdef _WIN32
  {
    DWORD size = MAX_HOSTNAME_LEN;
    if (!GetComputerNameExA(ComputerNameDnsHostname, g_hostname, &size)) {
      clogging_strtcpy(g_hostname, "unknown", MAX_HOSTNAME_LEN);
    }
  }
#else
  int rc = gethostname(g_hostname, MAX_HOSTNAME_LEN);
  if (rc < 0) {
    clogging_strtcpy(g_hostname, "unknown", MAX_HOSTNAME_LEN);
  }
#endif
#ifdef _WIN32
  g_pid = (int)GetCurrentProcessId();
#else
  g_pid = (int)getpid();
#endif
  g_level = level;

  /* Store logging options */
  if (opts != NULL) {
    g_log_options = *opts;
  } else {
    /* Use defaults */
    g_log_options.color = 0;
    g_log_options.json = 0;
    g_log_options.prefix_fields_flag = CLOGGING_PREFIX_DEFAULT;
  }

  return 0;
}

void clogging_basic_set_loglevel(enum LogLevel level) { g_level = level; }

enum LogLevel clogging_basic_get_loglevel(void) { return g_level; }

void clogging_basic_logmsg(const char *funcname, int linenum,
                           enum LogLevel level, const char *format, ...) {
  /* ISO 8601 date and time format with sec */
#define TIME_STR_LEN 26
  const int time_str_len = TIME_STR_LEN;
  char time_str[TIME_STR_LEN];
#undef TIME_STR_LEN
  time_t now;
  int len = 0;
  int rc = 0;
  const char *level_str = 0;
  char msg[MAX_LOG_MSG_LEN];
  va_list ap;

  /* ignore logs which are filtered out */
  if (level > g_level) {
    return;
  }

  if (g_is_logging_initialized <= 0) {
    fprintf(stderr, "logging is not initialized yet\n");
    return;
  }

  time(&now);
  len = time_to_cstr(&now, time_str, time_str_len);
  if (len < 0) {
    /* huh! I'd like to crash at this point but
     * lets just log the message, which is a must.
     */
    ++g_basic_num_msg_drops;
    return;
  }

  level_str = get_log_level_as_cstring(level);

  va_start(ap, format);
  rc = vsnprintf(msg, MAX_LOG_MSG_LEN, format, ap);
  if (rc < 0) {
    /* cannot recover from this one, so lets just ignore it
     * for now rather than logging it somewhere.
     */
    ++g_basic_num_msg_drops;
    return;
  }
  va_end(ap);

  /* JSON format output if enabled */
  if (g_log_options.json) {
    if (g_log_options.prefix_fields_flag == CLOGGING_PREFIX_DEFAULT) {
      /* optimization for default setting */
      rc = fprintf(stderr,
                   "{\"timestamp\":\"%s\", \"hostname\":\"%s\", \"progname\":\"%s\", \"threadname\":\"%s\", \"pid\":%d, \"level\":\"%s\", \"funcname\":\"%s\", \"linenum\":%d, \"message\":\"%s\"}\n",
                   time_str, g_hostname, g_progname, g_threadname, g_pid, level_str, funcname, linenum, msg);
    } else {
      /* Build JSON object: {"timestamp":"...", "hostname":"...", ...} */
      char json_line[1024] = {0};
      int json_pos = 0;
      
      /* Start JSON object */
      json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, "{");
      
      /* Add timestamp if enabled */
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_TIMESTAMP) {
        json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, "\"timestamp\":\"%s\"", time_str);
      }
      
      /* Add hostname if enabled */
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_HOSTNAME) {
        if (json_pos > 1) json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, ",");
        json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, "\"hostname\":\"%s\"", g_hostname);
      }
      
      /* Add program name if enabled */
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_PROGNAME) {
        if (json_pos > 1) json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, ",");
        json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, "\"progname\":\"%s\"", g_progname);
      }
      
      /* Add thread name if enabled (along with PID or as separate field) */
      if ((g_log_options.prefix_fields_flag & CLOGGING_PREFIX_PID) ||
          (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_PROGNAME)) {
        if (json_pos > 1) json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, ",");
        json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, "\"threadname\":\"%s\"", g_threadname);
      }
      
      /* Add PID if enabled */
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_PID) {
        if (json_pos > 1) json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, ",");
        json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, "\"pid\":%d", g_pid);
      }
      
      /* Add log level if enabled */
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_LOGLEVEL) {
        if (json_pos > 1) json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, ",");
        json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, "\"level\":\"%s\"", level_str);
      }
      
      /* Add function name if enabled */
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_FUNCNAME) {
        if (json_pos > 1) json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, ",");
        json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, "\"funcname\":\"%s\"", funcname);
      }
      
      /* Add line number if enabled */
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_LINENUM) {
        if (json_pos > 1) json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, ",");
        json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, "\"linenum\":%d", linenum);
      }
      
      /* Add message */
      if (json_pos > 1) json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, ",");
      json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, "\"message\":\"%s\"", msg);
      
      /* Close JSON object */
      json_pos += snprintf(json_line + json_pos, sizeof(json_line) - json_pos, "}");
      
      rc = fprintf(stderr, "%s\n", json_line);
    }
  } else {
    /* <HEADER> <MESSAGE>
     *	<HEADER> = <TIMESTAMP> <HOSTNAME>
     *	<MESSAGE> = <TAG> <LEVEL> <CONTENT>
     *		<TAG> = <PROGRAM><THREAD>[<PID>]
     *		<LEVEL> = DEBUG | INFO | WARNING | ERROR
     *		<CONTENT> = <FUNCTION/MODULE>: <APPLICATION_MESSAGE>
     */
    if (g_log_options.prefix_fields_flag == CLOGGING_PREFIX_DEFAULT) {
      /* optimization for default setting */
      rc = fprintf(stderr, "%s %s %s%s[%d] %s %s(%d): %s\n", time_str, g_hostname,
                   g_progname, g_threadname, g_pid, level_str, funcname, linenum,
                   msg);
    } else {
      /* Build prefix based on prefix_fields_flag */
      char prefix[512] = {0};
      int prefix_len = 0;
      
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_TIMESTAMP) {
        prefix_len += snprintf(prefix + prefix_len, sizeof(prefix) - prefix_len, "%s ", time_str);
      }
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_HOSTNAME) {
        prefix_len += snprintf(prefix + prefix_len, sizeof(prefix) - prefix_len, "%s ", g_hostname);
      }
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_PROGNAME) {
        prefix_len += snprintf(prefix + prefix_len, sizeof(prefix) - prefix_len, "%s", g_progname);
      }
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_PID) {
        prefix_len += snprintf(prefix + prefix_len, sizeof(prefix) - prefix_len, "%s[%d]", g_threadname, g_pid);
      } else if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_PROGNAME) {
        prefix_len += snprintf(prefix + prefix_len, sizeof(prefix) - prefix_len, "%s", g_threadname);
      }
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_LOGLEVEL) {
        prefix_len += snprintf(prefix + prefix_len, sizeof(prefix) - prefix_len, " %s", level_str);
      }
      
      char content_prefix[128] = {0};
      int content_len = 0;
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_FUNCNAME) {
        content_len += snprintf(content_prefix + content_len, sizeof(content_prefix) - content_len, "%s", funcname);
      }
      if (g_log_options.prefix_fields_flag & CLOGGING_PREFIX_LINENUM) {
        content_len += snprintf(content_prefix + content_len, sizeof(content_prefix) - content_len, "(%d)", linenum);
      }
      
      if (prefix_len > 0) {
        if (content_len > 0) {
          rc = fprintf(stderr, "%s %s: %s\n", prefix, content_prefix, msg);
        } else {
          rc = fprintf(stderr, "%s %s\n", prefix, msg);
        }
      } else {
        if (content_len > 0) {
          rc = fprintf(stderr, "%s: %s\n", content_prefix, msg);
        } else {
          rc = fprintf(stderr, "%s\n", msg);
        }
      }
    }
  }
  
  /* ignore the error if it's there */
  if (rc < 0) {
    ++g_basic_num_msg_drops;
  }
  /* Note that when less number of characters are written, even then
   * we dont keep track of that.
   */
}

uint64_t clogging_basic_get_num_dropped_messages(void) {
  return g_basic_num_msg_drops;
}

#ifdef __cplusplus
}
#endif
