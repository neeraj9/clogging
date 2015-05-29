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

#ifndef LOGGING_COMMON_H
#define LOGGING_COMMON_H

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
