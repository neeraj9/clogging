/*
 * Copyright (c) 2015 Neeraj Sharma <neeraj.sharma@alumni.iitg.ernet.in>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef BASIC_LOGGING_H
#define BASIC_LOGGING_H

/* must be the first thing to include before anything else */
#include "logging_common.h"

/* When LOGGING_WITH_THREAD_LOCAL_STORAGE is NOT defined.
 * For MT (multi threaded) applications the init_logging()
 * should be called from the main thread, although sufficient
 * protection is provided in the code even if that is not
 * the case.
 * The value passed to threadname should be "" (empty string).
 *
 * When LOGGING_WITH_THREAD_LOCAL_STORAGE is defined
 * then init_logging() should be called for each of the threads,
 * including the main thread.
 * The value passed to threadname should be "" (empty) or "-main"
 * for the main thread and "-<threadname>" (where <threadname> identifies
 * the thread) for child threads. As an example the child thread can
 * call threadname = "-worker1" for worker1 or say
 * threadname = "-tcplistener" for a worker who listens for tcp connections.
 */
int init_logging(const char *progname, const char *threadname,
		 enum LogLevel level);

/* When LOGGING_WITH_THREAD_LOCAL_STORAGE is NOT defined.
 * For MT programs its recommended to set log level from the
 * main thread, although sufficient protection is in place.
 * Note that there is no lock but integer assignment is supposed
 * to be atomic.
 *
 * When LOGGING_WITH_THREAD_LOCAL_STORAGE is defined.
 * There is a MT safe implementation.
 */
void set_loglevel(enum LogLevel level);

/* Get the current log level.
 *
 * Irrespective of LOGGING_WITH_THREAD_LOCAL_STORAGE this method
 * will do an atomic read, which is MT safe.
 */
enum LogLevel get_loglevel(void);

/* This will fail if init_logging() is not invoked earlier.
 * There is an additional cost to validating the initiatized state
 * but its worth the check.
 *
 * IMPORTANT: In case of MT (multi threaded) implementation
 * the logmsg() will cause the lock/unlock to happen since the
 * underlying mechanism for logging is fprintf to stderr.
 * This has performance cost when logging is done a lot, so
 * dont use this method if your application does a lot of logging.
 *
 * When LOGGING_WITH_THREAD_LOCAL_STORAGE is not defined.
 * There are additional actions done to make it MT safe without
 * using mutexes.
 *
 * When LOGGING_WITH_THREAD_LOCAL_STORAGE is defined.
 * It is a MT safe implementation.
 *
 */
void logmsg(const char *funcname,
	    int linenum, enum LogLevel level, const char *format, ...);

#endif /* BASIC_LOGGING_H */
