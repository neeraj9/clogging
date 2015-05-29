/*
 * Copyright (c) 2015 Neeraj Sharma <neeraj.sharma@alumni.iitg.ernet.in>
 *
 *  This file is part of clogging.
 *
 *  clogging is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  clogging is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with clogging.  If not, see <http://www.gnu.org/licenses/>.
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
