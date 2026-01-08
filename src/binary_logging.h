/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#ifndef CLOGGING_BINARY_LOGGING_H
#define CLOGGING_BINARY_LOGGING_H

#include "logging_common.h"

#include <stdint.h>

/* UTF-8 ENCODING NOTICE:
 * When compiled with CLOGGING_USE_UTF8_STRINGS, all string parameters
 * (progname, threadname, format strings, etc.) MUST be valid UTF-8.
 * 
 * - On Windows, use clogging_utf8_from_wide() to convert from UTF-16
 * - On Unix/Linux, most systems already use UTF-8
 * - Invalid UTF-8 will cause warnings in debug builds
 * 
 * Even without CLOGGING_USE_UTF8_STRINGS, UTF-8 strings work correctly,
 * but validation and conversion utilities won't be available.
 */

#ifdef __cplusplus
extern "C" {
#endif

enum VarArgType {
  BINARY_LOG_VAR_ARG_INTEGER = 0,
  BINARY_LOG_VAR_ARG_DOUBLE = 1,
  BINARY_LOG_VAR_ARG_POINTER = 2,
  BINARY_LOG_VAR_ARG_STRING = 3
};

/*
 * clogging_binary_init() should be called for each of the threads,
 * including the main thread.
 * The value passed to threadname should be "" (empty) or "-main"
 * for the main thread and "-<threadname>" (where <threadname> identifies
 * the thread) for child threads. As an example the child thread can
 * call threadname = "-worker1" for worker1 or say
 * threadname = "-tcplistener" for a worker who listens for tcp connections.
 *
 * Note that if handle represents a blocking fd/handle then the call to
 * every clogging_binary_logmsg() can block, so open in nonblocking mode
 * but then its possible that partial data is written to handle. This is
 * worse in some ways, so take your pick. Personally, I would risk the
 * non-blocking mode and handle partial writes at the receiver.
 *
 * progname is of maximum length of progname_len bytes including null terminator.
 * threadname is of maximum length of threadname_len bytes including null terminator.
 */
int clogging_binary_init(const char *progname, uint8_t progname_len,
                        const char *threadname, uint8_t threadname_len,
                         enum LogLevel level, clogging_handle_t handle);

/* Backward compatibility macro for old int-based API.
 * Converts int fd to clogging_handle_t automatically.
 */
#define clogging_binary_init_compat(progname, progname_len, threadname, threadname_len, level, fd) \
  clogging_binary_init((progname), (progname_len), (threadname), (threadname_len), (level), \
                       clogging_create_handle_from_fd(fd))

/*
 * It is a MT safe implementation.
 */
void clogging_binary_set_loglevel(enum LogLevel level);

/* Get the current log level.
 *
 * It is MT safe.
 */
enum LogLevel clogging_binary_get_loglevel(void);

/* This will fail if clogging_binary_init() is not invoked earlier.
 * There is an additional cost to validating the initiatized state
 * but its worth the check.
 *
 * The output is in binary format as follows:
 *
 * <length> <timestamp> <hostname> <progname>
 * <threadname> <pid> <loglevel> <file> <func> <linenum>
 * [<arg1>, <arg2>, ...]
 *
 * Note: Multi-byte fields are encoded in big-endian format.
 *
 * It is a MT safe implementation.
 *
 */
void clogging_binary_logmsg(const char *filename, const char *funcname,
                            int linenum, enum LogLevel level,
                            const char *format, ...);

/* Get the number of messages dropped due to overload or
 * internal errors.
 */
uint64_t clogging_binary_get_num_dropped_messages(void);

#ifdef __cplusplus
}
#endif

#endif /* CLOGGING_BINARY_LOGGING_H */
