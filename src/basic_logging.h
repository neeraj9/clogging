/*
 * Copyright (c) 2026 Neeraj Sharma <neerajsharma.9@outlook.com>
 *
 *  This file is part of clogging.
 *
 *  See LICENSE file for licensing information.
 */

#ifndef CLOGGING_BASIC_LOGGING_H
#define CLOGGING_BASIC_LOGGING_H

#include "logging_common.h"

#include <stdint.h>

#define BASIC_INIT_LOGGING(pn, pnlen, tn, tnlen, level)                                      \
  clogging_basic_init((pn), (pnlen), (tn), (tnlen), (level))
#define BASIC_SET_LOG_LEVEL(level) clogging_basic_set_loglevel(level)
#define BASIC_GET_LOG_LEVEL() clogging_basic_get_loglevel()
#define BASIC_GET_NUM_DROPPED_MESSAGES()                                       \
  clogging_basic_get_num_dropped_messages()

/* Lets follow the ISO C standard of 1999 and use ## __VA_ARGS__ so as
 * to avoid the neccessity of providing even a single argument after
 * format. That is its possible that the user did not provide any
 * variable arguments and the format is the entier message.
 */
#define BASIC_LOG_ERROR(format, ...)                                           \
  clogging_basic_logmsg(__func__, __LINE__, LOG_LEVEL_ERROR, format,           \
                        ##__VA_ARGS__)
#define BASIC_LOG_WARN(format, ...)                                            \
  clogging_basic_logmsg(__func__, __LINE__, LOG_LEVEL_WARN, format,            \
                        ##__VA_ARGS__)
#define BASIC_LOG_INFO(format, ...)                                            \
  clogging_basic_logmsg(__func__, __LINE__, LOG_LEVEL_INFO, format,            \
                        ##__VA_ARGS__)
#define BASIC_LOG_DEBUG(format, ...)                                           \
  clogging_basic_logmsg(__func__, __LINE__, LOG_LEVEL_DEBUG, format,           \
                        ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * init_logging() should be called for each of the threads,
 * including the main thread.
 * The value passed to threadname should be "" (empty) or "-main"
 * for the main thread and "-<threadname>" (where <threadname> identifies
 * the thread) for child threads. As an example the child thread can
 * call threadname = "-worker1" for worker1 or say
 * threadname = "-tcplistener" for a worker who listens for tcp connections.
 */
int clogging_basic_init(const char *progname, uint8_t progname_len,
                        const char *threadname, uint8_t threadname_len,
                        enum LogLevel level);

/*
 * It is a MT safe implementation.
 */
void clogging_basic_set_loglevel(enum LogLevel level);

/* Get the current log level.
 *
 * Irrespective of LOGGING_WITH_THREAD_LOCAL_STORAGE this method
 * will do an atomic read, which is MT safe.
 */
enum LogLevel clogging_basic_get_loglevel(void);

/* This will fail if init_logging() is not invoked earlier.
 * There is an additional cost to validating the initiatized state
 * but its worth the check.
 *
 * This function writes the log to stderr (or standard-error).
 *
 * IMPORTANT: In case of MT (multi threaded) implementation
 * the logmsg() will cause the lock/unlock to happen since the
 * underlying mechanism for logging is fprintf to stderr.
 * This has performance cost when logging is done a lot, so
 * dont use this method if your application does a lot of logging.
 *
 * It is a MT safe implementation.
 *
 */
void clogging_basic_logmsg(const char *funcname, int linenum,
                           enum LogLevel level, const char *format, ...);

/* Get the number of messages dropped due to overload or
 * internal errors.
 */
uint64_t clogging_basic_get_num_dropped_messages(void);

#ifdef __cplusplus
}
#endif

#endif /* CLOGGING_BASIC_LOGGING_H */
