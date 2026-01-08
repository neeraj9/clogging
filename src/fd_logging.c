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

#include "fd_logging.h"

#include <errno.h>    /* errno */
#include <stdarg.h>   /* va_start() and friends */
#include <stdio.h>    /* fprintf() and friends */
#include <string.h>   /* strerror_r() */
#include <sys/stat.h> /* fstat() */
#include <sys/types.h>
#include <time.h>   /* time() */

#ifdef _WIN32
/* Windows doesn't have ssize_t, define it */
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#include <winsock2.h> /* getsockname() for socket detection */
#endif /* _WIN32 */

#ifdef _WIN32
/* Use the WIN32_LEAN_AND_MEAN macro before including windows.h. This tells the
Windows header to exclude less commonly used APIs, including the old winsock.h.
If we dont do this, we may get redefinition errors for types like AF_IPX, because
we are using winsock2.h above.
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>  /* GetCurrentProcessId(), GetComputerNameExA() */
#else
#include <sys/types.h>
#include <sys/socket.h> /* getsockname() */
#include <unistd.h>   /* getpid(), gethostname() */
#endif

#define MAX_PROG_NAME_LEN 40
#define MAX_HOSTNAME_LEN 20

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The use of global static is tricky and prone to issues in MT (multi-threaded)
 * environments, but then the clogging_fd_init should be called ONLY once.
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

static THREAD_LOCAL char g_fd_progname[MAX_PROG_NAME_LEN] = {0};
static THREAD_LOCAL char g_fd_threadname[MAX_PROG_NAME_LEN] = {0};
static THREAD_LOCAL char g_fd_hostname[MAX_HOSTNAME_LEN] = {0};
static THREAD_LOCAL int g_fd_pid = 0;
static THREAD_LOCAL enum LogLevel g_fd_level = DEFAULT_LOG_LEVEL;
#ifdef _WIN32
static THREAD_LOCAL clogging_handle_t g_fd_handle = {CLOGGING_HANDLE_TYPE_CRT, {.crt_fd = 2}};
#else
static THREAD_LOCAL clogging_handle_t g_fd_handle = 2; /* stderr fd is default as 2 */
#endif
static THREAD_LOCAL int g_fd_prefix_length = 0; /* 1 when prefix length to log
                                               entry */
/* safeguard calling init_logging multiple times */
static THREAD_LOCAL int g_fd_is_logging_initialized = 0;

#define TOTAL_MSG_BYTES 1024
/* optimization by having only one instance per thread instead of
 * stack allocation all the time.
 */
static THREAD_LOCAL char g_fd_total_message[TOTAL_MSG_BYTES];

/* account for partial write */
static THREAD_LOCAL uint8_t g_fd_previous_message_offset = 0;
static THREAD_LOCAL uint8_t g_fd_previous_message_bytes = 0;
static THREAD_LOCAL char g_fd_previous_message[TOTAL_MSG_BYTES];

/* store the number of message dropped as a counter for
 * later statistics collection.
 */
static THREAD_LOCAL uint64_t g_fd_num_msg_drops = 0;

int clogging_fd_init(const char *progname, uint8_t progname_len,
                     const char *threadname, uint8_t threadname_len,
                     enum LogLevel level, clogging_handle_t handle) {
  int rc = 0;

  if (g_fd_is_logging_initialized > 0) {
    fprintf(stderr, "logging is already initialized or in the"
                    " process of initialization.\n");
    return -1;
  }

  /* first thing to do is block any other thread in running logging
   * initialization (if that is done, though its bad and should be
   * done in the main thread ONLY.
   */
  g_fd_is_logging_initialized = 1;

  /* Intentionally call the method so that any function static variables
   * in get_log_level_as_cstring() are correctly initialized.
   * Note that this should be done in any logging implementation (like
   * many other things), although the argument and the return values
   * are unimportant.
   */
  (void)get_log_level_as_cstring(LOG_LEVEL_ERROR);

  clogging_strtcpy(g_fd_progname, progname, MAX_PROG_NAME_LEN);
  clogging_strtcpy(g_fd_threadname, threadname, MAX_PROG_NAME_LEN);
#ifdef _WIN32
  {
    DWORD size = MAX_HOSTNAME_LEN;
    if (!GetComputerNameExA(ComputerNameDnsHostname, g_fd_hostname, &size)) {
      clogging_strtcpy(g_fd_hostname, "unknown", MAX_HOSTNAME_LEN);
    }
  }
#else
  rc = gethostname(g_fd_hostname, MAX_HOSTNAME_LEN);
  if (rc < 0) {
    clogging_strtcpy(g_fd_hostname, "unknown", MAX_HOSTNAME_LEN);
  }
#endif
#ifdef _WIN32
  g_fd_pid = (int)GetCurrentProcessId();
#else
  g_fd_pid = (int)getpid();
#endif
  g_fd_level = level;
  g_fd_handle = handle;

  /* determine the type of handle and set prefix length accordingly */
  if (clogging_handle_is_socket(handle) == 1) {
    g_fd_prefix_length = 1;
  } else if (clogging_handle_is_pipe(handle) == 1) {
    g_fd_prefix_length = 1;
  } else {
    g_fd_prefix_length = 0;
  }

  return 0;
}

void clogging_fd_set_loglevel(enum LogLevel level) { g_fd_level = level; }

enum LogLevel clogging_fd_get_loglevel(void) { return g_fd_level; }

void clogging_fd_logmsg(const char *funcname, int linenum, enum LogLevel level,
                        const char *format, ...) {
  /* ISO 8601 date and time format with sec */
#define TIME_STR_LEN 26
  const int time_str_len = TIME_STR_LEN;
  char time_str[TIME_STR_LEN];
#undef TIME_STR_LEN
  time_t now;
  int remaining_bytes = 0;
  int len = 0;
  int rc = 0;
  const char *level_str = 0;
  char msg[MAX_LOG_MSG_LEN];
  va_list ap;
  ssize_t bytes_sent = 0;
  int msg_offset = 0;

  /* ignore logs which are filtered out */
  if (level > g_fd_level) {
    return;
  }

  if (g_fd_is_logging_initialized <= 0) {
    fprintf(stderr, "logging is not initialized yet\n");
    ++g_fd_num_msg_drops;
    return;
  }

  remaining_bytes =
      (g_fd_previous_message_bytes - g_fd_previous_message_offset);
  if (len > 0) {
    len = (int)clogging_handle_write(g_fd_handle, &g_fd_previous_message[g_fd_previous_message_offset],
                                      remaining_bytes);
    if (len <= 0) {
      /* cannot write a thing, so drop the current message
       */
      ++g_fd_num_msg_drops;
      return;
    }
    if (len < remaining_bytes) {
      g_fd_previous_message_offset += len;
      /* since this time as well it was partial write
       * so new message can anyway not be written.
       * There is no other option other than dropping the
       * current message.
       */
      ++g_fd_num_msg_drops;
      return;
    } else {
      /* previous message is written completely */
      g_fd_previous_message_bytes = 0;
      g_fd_previous_message_offset = 0;
    }
  }

  time(&now);
  len = time_to_cstr(&now, time_str, time_str_len);
  if (len < 0) {
    /* huh! I'd like to crash at this point but
     * lets just log the message, which is a must.
     */
    ++g_fd_num_msg_drops;
    return;
  }

  level_str = get_log_level_as_cstring(level);

  va_start(ap, format);
  rc = vsnprintf(msg, MAX_LOG_MSG_LEN, format, ap);
  if (rc < 0) {
    /* cannot recover from this one, so lets just ignore it
     * for now rather than logging it somewhere.
     */
    ++g_fd_num_msg_drops;
    return;
  }
  va_end(ap);

  if (g_fd_prefix_length) {
    /* add a length field when the
     * fd is not a regular file.
     */
    msg_offset = 2;
  }

  /* <HEADER> <MESSAGE>
   *	<HEADER> = <TIMESTAMP> <HOSTNAME>
   *	<MESSAGE> = <TAG> <LEVEL> <CONTENT>
   *		<TAG> = <PROGRAM><THREAD>[<PID>]
   *		<LEVEL> = DEBUG | INFO | WARNING | ERROR
   *		<CONTENT> = <FUNCTION/MODULE>: <APPLICATION_MESSAGE>
   */
  /* leave the first two bytes for size */
  len = snprintf(&g_fd_total_message[msg_offset], TOTAL_MSG_BYTES - msg_offset,
                 "%s %s %s%s[%d] %s %s(%d): %s\n", time_str, g_fd_hostname,
                 g_fd_progname, g_fd_threadname, g_fd_pid, level_str, funcname,
                 linenum, msg);
  if (len < 0) {
    /* there is nothing much we can do, so return.  */
    ++g_fd_num_msg_drops;
    return;
  }
  /* Note that the null character at the end is not part of the len */
  /* encode the length in big-endian format */
  if (g_fd_prefix_length) {
    g_fd_total_message[0] = (len >> 8) & 0x00ff;
    g_fd_total_message[1] = (len & 0x00ff);
  }
  bytes_sent = clogging_handle_write(g_fd_handle, g_fd_total_message, len + msg_offset);
#if VERBOSE
  if (bytes_sent < 0) {
    int err = errno;
    char errmsg[256];
    strerror_r(err, errmsg, sizeof(errmsg));
    fprintf(stderr, "%s%s: write() failed, e=%d, errmsg=[%s]\n", g_fd_progname,
            g_fd_threadname, err, errmsg);
    ++g_fd_num_msg_drops;
  } else if (bytes_sent != (len + msg_offset)) {
    fprintf(stderr, "%s%s: could write only %d out of %d bytes\n",
      g_fd_progname, g_fd_threadname, bytes_sent, len + msg_offset);
  } else {
    fprintf(stderr, "%s%s: success\n", g_fd_progname, g_fd_threadname);
  }
#else
  if (bytes_sent < 0 || bytes_sent != (len + msg_offset)) {
    /* cannot write a thing, so drop the current message
     */
    ++g_fd_num_msg_drops;
    return;
  }
#endif /* VERBOSE */
}

uint64_t clogging_fd_get_num_dropped_messages(void) {
  return g_fd_num_msg_drops;
}

#ifdef __cplusplus
}
#endif
