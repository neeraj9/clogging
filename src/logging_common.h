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

#ifndef LOGGING_COMMON_H
#define LOGGING_COMMON_H

#include "config.h"

#ifdef LOGGING_WITH_THREAD_LOCAL_STORAGE
#define LOGGING_OPTIONAL_TLS  __thread
#else
#define LOGGING_OPTIONAL_TLS
#endif

/* If VERBOSE is NOT defined then the DEBUG logs are compiled out
 * and cannot be switched on dynamically. This is done to ensure that
 * production system have DEBUG logs compiled out and save critical
 * cycles which otherwise would be wasted in function call to logmsg()
 * and then returning when log level is below DEBUG.
 */
#define VERBOSE

/* Maximum size of message in bytes which can be logged. Note that this
 * do not include the prefix size where timestamp and other details
 * might be present.
 */
#define MAX_LOG_MSG_LEN 256

/* DONT change the values because there is a lookup
 * implemented in specific logging implementation
 * based on these values. See logging_common.c for
 * a sample usage in the get_log_level_as_cstring() implementation.
 *
 * Additionally note that the sever the error the lower the value
 * should be. This is used in specific implementation of logging.
 * See basic_logging.c logmsg() as an implementation sample.
 */
enum LogLevel {
	LOG_LEVEL_ERROR = 0,
	LOG_LEVEL_WARN = 1,
	LOG_LEVEL_INFO = 2,
	LOG_LEVEL_DEBUG = 3
};

/* The default log level is INFO and must be considered so in
 * all implementations of specific logging. See basic_logging.c for
 * usage of this.
 */
#define DEFAULT_LOG_LEVEL LOG_LEVEL_INFO

/* Lets follow the ISO C standard of 1999 and use ## __VA_ARGS__ so as
 * to avoid the neccessity of providing even a single argument after
 * format. That is its possible that the user did not provide any
 * variable arguments and the format is the entier message.
 */
#define LOG_ERROR(format, ...) \
	logmsg(__func__, __LINE__, LOG_LEVEL_ERROR, format, ## __VA_ARGS__)
#define LOG_WARN(format, ...) \
	logmsg(__func__, __LINE__, LOG_LEVEL_WARN, format, ## __VA_ARGS__)
#define LOG_INFO(format, ...) \
	logmsg(__func__, __LINE__, LOG_LEVEL_INFO, format, ## __VA_ARGS__)

#if defined(VERBOSE)
#define LOG_DEBUG(format, ...) \
	logmsg(__func__, __LINE__, LOG_LEVEL_DEBUG, format, ## __VA_ARGS__)
#else
/* If not verbose then dont even compile the DEBUG logs, which are typically
 * cpu intensive function calls in the critical path.
 * If you want dynamicity then use INFO instead and set the log level to
 * WARN, which can be changed to INFO dynamically anytime.
 */
#define LOG_DEBUG(format, ...)
#endif

/* Get the string representation of logging level.
 *
 * When LOGGING_WITH_THREAD_LOCAL_STORAGE is defined then this is a
 * MT safe function in all respects.
 *
 * When LOGGING_WITH_THREAD_LOCAL_STORAGE is NOT defined then
 * the init_logging() should call this method during initialization
 * as a workaround to make it MT safe initialization. See the implementation
 * of this function for more details.
 */
const char *get_log_level_as_cstring(enum LogLevel level);

#endif /* LOGGING_COMMON_H */
