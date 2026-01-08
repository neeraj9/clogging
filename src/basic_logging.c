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
#include <string.h>   /* strncpy() */
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
/* safeguard calling init_logging multiple times */
static THREAD_LOCAL int g_is_logging_initialized = 0;

/* store the number of message dropped as a counter for
 * later statistics collection.
 */
static THREAD_LOCAL uint64_t g_basic_num_msg_drops = 0;

int clogging_basic_init(const char *progname, const char *threadname,
                        enum LogLevel level) {
  int rc = 0;

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

  strncpy(g_progname, progname, MAX_PROG_NAME_LEN);
  strncpy(g_threadname, threadname, MAX_PROG_NAME_LEN);
#ifdef _WIN32
  {
    DWORD size = MAX_HOSTNAME_LEN;
    if (!GetComputerNameExA(ComputerNameDnsHostname, g_hostname, &size)) {
      strncpy(g_hostname, "unknown", MAX_HOSTNAME_LEN);
    }
  }
#else
  rc = gethostname(g_hostname, MAX_HOSTNAME_LEN);
  if (rc < 0) {
    strncpy(g_hostname, "unknown", MAX_HOSTNAME_LEN);
  }
#endif
#ifdef _WIN32
  g_pid = (int)GetCurrentProcessId();
#else
  g_pid = (int)getpid();
#endif
  g_level = level;

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

  /* <HEADER> <MESSAGE>
   *	<HEADER> = <TIMESTAMP> <HOSTNAME>
   *	<MESSAGE> = <TAG> <LEVEL> <CONTENT>
   *		<TAG> = <PROGRAM><THREAD>[<PID>]
   *		<LEVEL> = DEBUG | INFO | WARNING | ERROR
   *		<CONTENT> = <FUNCTION/MODULE>: <APPLICATION_MESSAGE>
   */
  rc = fprintf(stderr, "%s %s %s%s[%d] %s %s(%d): %s\n", time_str, g_hostname,
               g_progname, g_threadname, g_pid, level_str, funcname, linenum,
               msg);
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
